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
#include <inttypes.h>
#include "stlink.h"
#include "stlink-libusb.h"

enum {
    USB_DEBUGLEVEL_NONE     = 0,
    USB_DEBUGLEVEL_ERROR    = 1,
    USB_DEBUGLEVEL_WARNING  = 2,
};

static inline void dump_data(uint8_t *buf, size_t len)
{
    for (int i = 0; i < len; i += 16) {
        for (int j = i; j < i + 16 && j < len; j++) {
            printf("%02" PRIX8 " ", buf[j]);
        }
        printf("\n");
    }
}

static inline int swim_poll(stlink *stl, uint32_t addr, uint16_t len, uint8_t x)
{
    int ret;
    uint32_t busy;
    do {
        ret = stlink_swim_get_busy(stl, addr, len, x, &busy);
    } while ((busy & 0xff) && ret == 0);
    return ret;
}

#define CHECK_SWIM(x) \
    ret = x; \
    if (ret != 0) \
        return; \
    ret = swim_poll(stl, 0x0, 0x0100, 0x00); \
    if (ret != 0) \
        return

#define CHECK_SWIM0(x) \
    ret = x; \
    if (ret != 0) \
        return; \
    ret = swim_poll(stl, 0x0, 0x00, 0x0); \
    if (ret != 0) \
        return

#define CHECK_SWIMxy(x, addr, len, x1) \
    ret = x; \
    if (ret != 0) \
        return; \
    ret = swim_poll(stl, addr, len, x1); \
    if (ret != 0) \
        return

#define SWIM_READ(addr, len, x1, buf) \
    ret = stlink_swim_do_0b(stl, addr, len, x1); \
    if (ret != 0) \
        return; \
    ret = swim_poll(stl, addr, len, x1); \
    if (ret != 0) \
        return; \
    ret = stlink_swim_read_flash(stl, addr, len, x1, buf); \
    if (ret != 0) \
        return

static void swim(stlink *stl)
{
    uint16_t size = 0;
    int ret = stlink_swim_get_size(stl, &size);
    if (ret != 0)
        return;
    printf("size = 0x%" PRIx16 "\n", size);
    ret = stlink_swim_get_02(stl);
    if (ret != 0)
        return;
    CHECK_SWIM(stlink_swim_do_07(stl));
    CHECK_SWIM(stlink_swim_do_07(stl));
    CHECK_SWIM(stlink_swim_do_08(stl, 0x0, 0x0100, 0x00));
    CHECK_SWIM(stlink_swim_do_07(stl));
    CHECK_SWIM(stlink_swim_do_04(stl));
    CHECK_SWIM0(stlink_swim_do_03(stl, 0x00));
    CHECK_SWIM0(stlink_swim_do_05(stl));
    CHECK_SWIMxy(stlink_swim_do_0a(stl, 0x7f80, 1, 0xa0), 0x7f80, 1, 0xa0);
    CHECK_SWIMxy(stlink_swim_do_08(stl, 0x7f80, 1, 0xa0), 0x7f80, 1, 0xa0);

    uint8_t *buf = malloc(size);

    SWIM_READ(0x7f99, 1, 0xa0, buf);
    dump_data(buf, 1);

    CHECK_SWIMxy(stlink_swim_do_06(stl, 0x7f99, 1, 0xa0), 0x7f99, 1, 0xa0);
    CHECK_SWIMxy(stlink_swim_do_0a(stl, 0x7f80, 1, 0xb0), 0x7f80, 1, 0xb0);
    CHECK_SWIMxy(stlink_swim_do_03(stl, 0x01), 0x7f80, 0x0101, 0xb0);
    CHECK_SWIMxy(stlink_swim_do_0a(stl, 0x7f80, 1, 0xb4), 0x7f80, 1, 0xb4);

#if 0
    // ??? boot ROM
    SWIM_READ(0x67f0, 6, 0x00, buf);
    dump_data(buf, 6);

    // ??? reserved (between GPIO and periph. reg. and boot ROM)
    SWIM_READ(0x5808, 1, 0x00, buf);
    dump_data(buf, 1);

    // ??? option bytes
    SWIM_READ(0x488e, 2, 0x00, buf);
    dump_data(buf, 2);

    // Read-out protection (ROP)
    SWIM_READ(0x4800, 1, 0x00, buf);
    dump_data(buf, 1);

    // User boot code (UBC) (OPT1)
    SWIM_READ(0x4801, 1, 0x00, buf);
    dump_data(buf, 1);

    // User boot code (UBC) (NOPT1)
    SWIM_READ(0x4802, 1, 0x00, buf);
    dump_data(buf, 1);
#endif

    // Flash program memory
    uint32_t flash_start = 0x8000;
    uint16_t flash_size = 32 * 1024;
    uint32_t flash_end = flash_start + flash_size;
    for (uint32_t addr = flash_start; addr < flash_end; addr += size) {
        uint16_t len = (addr + size > flash_end) ? (flash_end - addr) : size;
        SWIM_READ(addr, len, 0x00, buf);
        dump_data(buf, len);
    }

    free(buf);
}

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
    /*if (mode != -1 && mode != STLINK_DEV_DEBUG_MODE) {
        stlink_enter_swd_mode(stl);
        mode = stlink_get_current_mode(stl);
        printf("new mode = %02x\n", mode);
    }*/
    if (mode != -1 && mode != STLINK_DEV_SWIM_MODE) {
        stlink_swim_enter(stl);
        mode = stlink_get_current_mode(stl);
        printf("new mode = %02x\n", mode);
    }
    if (mode == STLINK_DEV_SWIM_MODE) {
        swim(stl);
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
