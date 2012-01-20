/*
 * Copyright (c) 2011 Andreas Färber <andreas.faerber@web.de>
 *
 * Licensed under the GNU Lesser General Public License (LGPL) version 2.1,
 * or (at your option) any later version.
 *
 * USB mass storage spec:
 * http://www.usb.org/developers/devclass_docs/usbmassbulk_10.pdf
 *
 * USB mass storage error handling based on libusb example code:
 * Copyright (c) 2009-2011 Pete Batard <pbatard@gmail.com>
 * Based on lsusb, copyright (c) 2007 Daniel Drake <dsd@gentoo.org>
 * With contributions to Mass Storage test by Alan Stern.
 */

#include "stlink-libusb.h"

#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "bswap.h"
#include "stlink.h"


#define STLINK_TIMEOUT_MS 1000 // 1 s

// Command Block Wrapper (CBW)
typedef struct CommandBlockWrapper {
    uint32_t    dCBWSignature;
    uint32_t    dCBWTag;
    uint32_t    dCBWDataTransferLength;
    uint8_t     bmCBWFlags;
    uint8_t     bCBWLUN;
    uint8_t     bCBWCBLength;
    uint8_t     CBWCB[16];
} __attribute__((packed)) USBCommandBlockWrapper;

#define USB_CBW_SIGNATURE 0x43425355

// Command Status Wrapper (CSW)
typedef struct CommandStatusWrapper {
    uint32_t    dCSWSignature;
    uint32_t    dCSWTag;
    uint32_t    dCSWDataResidue;
    uint8_t     bCSWStatus;
} __attribute__((packed)) USBCommandStatusWrapper;

#define USB_CSW_SIGNATURE 0x53425355

enum {
    USB_CSW_STATUS_COMMAND_PASSED   = 0x00,
    USB_CSW_STATUS_COMMAND_FAILED   = 0x01,
    USB_CSW_STATUS_PHASE_ERROR      = 0x02,
};

// ST-Link device
struct STLink {
    libusb_device_handle *handle;
    uint8_t endpoint_in;
    uint8_t endpoint_out;
};

stlink *stlink_open(libusb_context *usb_context)
{
    stlink *stl = malloc(sizeof(stlink));
    stl->handle = libusb_open_device_with_vid_pid(usb_context, USB_VID_ST, USB_PID_STLINK);
    if (stl->handle == NULL) {
        free(stl);
        return NULL;
    }

    libusb_device *dev = libusb_get_device(stl->handle);
    struct libusb_config_descriptor *conf_desc;
    int ret = libusb_get_config_descriptor(dev, 0, &conf_desc);
    if (ret != LIBUSB_SUCCESS) {
        libusb_close(stl->handle);
        free(stl);
        return NULL;
    }
    for (int i = 0; i < conf_desc->bNumInterfaces; i++) {
        printf("interface %d\n", i);
        for (int j = 0; j < conf_desc->interface[i].num_altsetting; j++) {
            for (int k = 0; k < conf_desc->interface[i].altsetting[j].bNumEndpoints; k++) {
                const struct libusb_endpoint_descriptor *endpoint;
                endpoint = &conf_desc->interface[i].altsetting[j].endpoint[k];
                if (endpoint->bEndpointAddress & LIBUSB_ENDPOINT_IN) {
                    stl->endpoint_in = endpoint->bEndpointAddress;
                    printf("Found IN endpoint\n");
                } else {
                    stl->endpoint_out = endpoint->bEndpointAddress;
                    printf("Found OUT endpoint\n");
                }
            }
        }
    }
    libusb_free_config_descriptor(conf_desc);

    ret = libusb_kernel_driver_active(stl->handle, 0);
    if (ret == 1) {
        printf("kernel driver active\n");
        ret = libusb_detach_kernel_driver(stl->handle, 0);
        fprintf(stderr, "detaching kernel driver failed: %d\n", ret);
    } else if (ret == 0) {
        //printf("kernel driver not active\n");
    } else {
        fprintf(stderr, "libusb_kernel_driver_active = %d\n", ret);
    }
    ret = libusb_claim_interface(stl->handle, 0);
    if (ret != LIBUSB_SUCCESS) {
        fprintf(stderr, "claiming interface failed: %d\n", ret);
        libusb_close(stl->handle);
        free(stl);
        return NULL;
    }

    return stl;
}

void stlink_close(stlink *stl)
{
    if (stl == NULL)
        return;

    libusb_release_interface(stl->handle, 0);
    libusb_close(stl->handle);
    free(stl);
}

#define RETRY_MAX 5

static uint32_t
send_usb_mass_storage_command(libusb_device_handle *handle, uint8_t endpoint,
                              uint8_t *cdb, uint8_t cdb_length,
                              uint8_t lun, uint8_t flags, uint32_t data_transfer_length)
{
    static uint32_t tag;

    USBCommandBlockWrapper cbw;
    memset(&cbw, 0, sizeof(USBCommandBlockWrapper));
    cbw.dCBWSignature = cpu_to_le32(USB_CBW_SIGNATURE);
    if (tag == 0)
        tag = 1;
    cbw.dCBWTag = cpu_to_le32(tag);
    int curTag = tag++;
    cbw.dCBWDataTransferLength = cpu_to_le32(data_transfer_length);
    cbw.bmCBWFlags = flags;
    cbw.bCBWLUN = lun;
    cbw.bCBWCBLength = cdb_length;
    memcpy(cbw.CBWCB, cdb, cdb_length);
    int transferred;
    int ret;
    int try = 0;
    do {
        ret = libusb_bulk_transfer(handle, endpoint, (unsigned char *)&cbw, sizeof(cbw),
                                   &transferred, STLINK_TIMEOUT_MS);
        if (ret == LIBUSB_ERROR_PIPE) {
            libusb_clear_halt(handle, endpoint);
        }
        try++;
    } while ((ret == LIBUSB_ERROR_PIPE) && (try < RETRY_MAX));
    if (ret != LIBUSB_SUCCESS) {
        fprintf(stderr, "%s: sending failed: %d\n", __func__, ret);
        return 0;
    }
    return curTag;
}

static int
get_usb_mass_storage_status(libusb_device_handle *handle, uint8_t endpoint, uint32_t *tag)
{
    USBCommandStatusWrapper csw;
    int transferred;
    int ret;
    int try = 0;
    do {
        ret = libusb_bulk_transfer(handle, endpoint, (unsigned char *)&csw, sizeof(csw),
                                   &transferred, STLINK_TIMEOUT_MS);
        if (ret == LIBUSB_ERROR_PIPE) {
            libusb_clear_halt(handle, endpoint);
        }
        try++;
    } while ((ret == LIBUSB_ERROR_PIPE) && (try < RETRY_MAX));
    if (ret != LIBUSB_SUCCESS) {
        fprintf(stderr, "%s: receiving failed: %d\n", __func__, ret);
        return -1;
    }
    if (transferred != sizeof(csw)) {
        fprintf(stderr, "%s: received unexpected amount: %d\n", __func__, transferred);
        return -1;
    }
    csw.dCSWSignature = le32_to_cpu(csw.dCSWSignature);
    csw.dCSWTag = le32_to_cpu(csw.dCSWTag);
    csw.dCSWDataResidue = le32_to_cpu(csw.dCSWDataResidue);
    if (csw.dCSWSignature != USB_CSW_SIGNATURE) {
        fprintf(stderr, "%s: received wrong signature: %04" PRIX32 "\n",
                __func__, csw.dCSWSignature);
        return -1;
    }
    //printf("%s: residue = 0x%" PRIx32 "\n", __func__);
    *tag = csw.dCSWTag;
    return csw.bCSWStatus;
}

#define REQUEST_SENSE 0x03
#define REQUEST_SENSE_LENGTH 18

static void
get_sense(libusb_device_handle *handle, uint8_t endpoint_in, uint8_t endpoint_out)
{
    uint8_t cdb[16];
    memset(cdb, 0, sizeof(cdb));
    cdb[0] = REQUEST_SENSE;
    cdb[4] = REQUEST_SENSE_LENGTH;
    uint32_t tag = send_usb_mass_storage_command(handle, endpoint_out, cdb, sizeof(cdb), 0,
                                                 LIBUSB_ENDPOINT_IN, REQUEST_SENSE_LENGTH);
    if (tag == 0) {
        fprintf(stderr, "%s: sending REQUEST SENSE failed\n", __func__);
        return;
    }
    unsigned char sense[REQUEST_SENSE_LENGTH];
    int transferred;
    int ret;
    int try = 0;
    do {
        ret = libusb_bulk_transfer(handle, endpoint_in, sense, sizeof(sense),
                                   &transferred, STLINK_TIMEOUT_MS);
        if (ret == LIBUSB_ERROR_PIPE) {
            libusb_clear_halt(handle, endpoint_in);
        }
        try++;
    } while ((ret == LIBUSB_ERROR_PIPE) && (try < RETRY_MAX));
    if (ret != LIBUSB_SUCCESS) {
        fprintf(stderr, "%s: receiving failed: %d\n", __func__, ret);
        return;
    }
    if (transferred != sizeof(sense)) {
        fprintf(stderr, "%s: received unexpected amount: %d\n", __func__, transferred);
    }
    uint32_t received_tag;
    int status = get_usb_mass_storage_status(handle, endpoint_in, &received_tag);
    if (status != USB_CSW_STATUS_COMMAND_PASSED) {
        fprintf(stderr, "%s: receiving failed with status: %02x\n", __func__, status);
        return;
    }
    if (sense[0] != 0x70 && sense[0] != 0x71) {
        fprintf(stderr, "No sense data\n");
    } else {
        fprintf(stderr, "Sense KCQ: %02X %02X %02X\n", sense[2] & 0x0f, sense[12], sense[13]);
    }
}

int stlink_send_command(stlink *stl, uint8_t *cdb, uint8_t cdb_length,
                        uint8_t *buffer, int transfer_length, bool inbound)
{
    printf("%s: CDB:", __func__);
    for (int i = 0; i < cdb_length; i++) {
        printf(" %02" PRIX8, cdb[i]);
    }
    printf("\n");
    uint8_t lun = 0;
    uint32_t tag = send_usb_mass_storage_command(stl->handle, stl->endpoint_out,
                                                 cdb, cdb_length, lun,
                                                 LIBUSB_ENDPOINT_IN, transfer_length);
    if (tag == 0) {
        fprintf(stderr, "%s: sending failed\n", __func__);
        return -1;
    }
    int transferred;
    if (transfer_length > 0) {
        int ret;
        int try = 0;
        do {
            ret = libusb_bulk_transfer(stl->handle,
                                       (!inbound) ? stl->endpoint_out : stl->endpoint_in,
                                       buffer, transfer_length,
                                       &transferred, STLINK_TIMEOUT_MS);
            if (ret == LIBUSB_ERROR_PIPE) {
                libusb_clear_halt(stl->handle, stl->endpoint_in);
            }
            try++;
        } while ((ret == LIBUSB_ERROR_PIPE) && (try < RETRY_MAX));
        if (ret != LIBUSB_SUCCESS) {
            fprintf(stderr, "%s: transferring failed: %d\n", __func__, ret);
            return -1;
        }
        if (transferred != transfer_length) {
            fprintf(stderr, "%s: transferred unexpected amount: %d\n", __func__, transferred);
        }
    }
    uint32_t received_tag;
    int status = get_usb_mass_storage_status(stl->handle, stl->endpoint_in, &received_tag);
    if (status < 0) {
        fprintf(stderr, "%s: receiving status failed: %d\n", __func__, status);
        return -1;
    }
    if (status != USB_CSW_STATUS_COMMAND_PASSED) {
        fprintf(stderr, "%s: receiving status: %02x\n", __func__, status);
    }
    if (status == USB_CSW_STATUS_COMMAND_FAILED) {
        get_sense(stl->handle, stl->endpoint_in, stl->endpoint_out);
        return -1;
    }
    if (received_tag != tag) {
        fprintf(stderr, "%s: received tag %08" PRIx32 " but expected %08" PRIx32 "\n",
                __func__, received_tag, tag);
        //return -1;
    }
    if (transfer_length > 0 && transferred != transfer_length) {
        return -1;
    }
    return 0;
}
