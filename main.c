/*
 * Test app for ST-Link
 *
 * Copyright (c) 2011 Andreas Färber
 *
 * Licensed under the GNU General Public License (GPL) version 2, or
 * at your option, any later version.
 */

#include <stdlib.h>
#include <stdio.h>
#include "stlink.h"
#include "stlink-libusb.h"

enum {
    USB_DEBUGLEVEL_NONE     = 0,
    USB_DEBUGLEVEL_ERROR    = 1,
    USB_DEBUGLEVEL_WARNING  = 2,
};

static void connect(libusb_context *usb_context)
{
    printf("Opening ST-Link device...\n");
    stlink *stl = stlink_open(usb_context);
    if (stl == NULL) {
        return;
    }
    stlink_get_version(stl);
    int mode = stlink_get_current_mode(stl);
    printf("mode = %02x\n", mode);
    if (mode == STLINK_DEV_DFU_MODE) {
        stlink_exit_dfu_mode(stl);
        mode = stlink_get_current_mode(stl);
        printf("new mode = %02x\n", mode);
    }
    if (mode != -1 && mode != STLINK_DEV_DEBUG_MODE) {
        stlink_enter_swd_mode(stl);
        mode = stlink_get_current_mode(stl);
        printf("new mode = %02x\n", mode);
    }

    stlink_close(stl);
    printf("done.\n");
}

int main(void)
{
    int ret;

    libusb_context *usb_context;
    ret = libusb_init(&usb_context);
    if (ret != 0) {
        fprintf(stderr, "USB init failed, exiting.\n");
        return -1;
    }
    //libusb_set_debug(usb_context, USB_DEBUGLEVEL_WARNING);

    connect(usb_context);

    libusb_exit(usb_context);
    return 0;
}
