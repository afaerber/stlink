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

static inline int swim_poll(stlink *stl)
{
    int ret;
    uint32_t busy;
    do {
        ret = stlink_swim_get_busy(stl, &busy);
    } while ((busy & 0xff) && ret == 0);
    return ret;
}

#define CHECK_SWIM(x) \
    ret = x; \
    if (ret != 0) \
        return; \
    ret = swim_poll(stl); \
    if (ret != 0) \
        return

#define SWIM_READ(addr, len, buf) \
    ret = stlink_swim_begin_read(stl, addr, len); \
    if (ret != 0) \
        return; \
    ret = swim_poll(stl); \
    if (ret != 0) \
        return; \
    ret = stlink_swim_read(stl, len, buf); \
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
    CHECK_SWIM(stlink_swim_do_07(stl, 0x0, 0x0100, 0x00));

    CHECK_SWIM(stlink_swim_do_07(stl, 0x0, 0x0100, 0x00));
    CHECK_SWIM(stlink_swim_do_08(stl, 0x0, 0x0100, 0x00));
    CHECK_SWIM(stlink_swim_do_07(stl, 0x0, 0x0100, 0x00));
    CHECK_SWIM(stlink_swim_do_04(stl, 0x0, 0x0100, 0x00)); // causes demo to stop blinking
    CHECK_SWIM(stlink_swim_do_03(stl, 0x00));
    CHECK_SWIM(stlink_swim_do_05(stl, 0x0, 0, 0x00));

    uint8_t *buf = malloc(size);

    // 0xa0
    buf[0] = STM8_SWIM_CSR_SAFE_MASK |
             STM8_SWIM_CSR_SWIM_DM;
    CHECK_SWIM(stlink_swim_write(stl, STM8_SWIM_CSR, 1, buf));
    CHECK_SWIM(stlink_swim_do_08(stl, 0x7f80, 1, 0xa0));

    SWIM_READ(STM8_DM_CSR2, 1, buf);
    dump_data(buf, 1);

    CHECK_SWIM(stlink_swim_do_06(stl, 0x7f99, 1, 0xa0));
    // 0xb0
    buf[0] = STM8_SWIM_CSR_SAFE_MASK |
             STM8_SWIM_CSR_SWIM_DM |
             STM8_SWIM_CSR_HS;
    CHECK_SWIM(stlink_swim_write(stl, STM8_SWIM_CSR, 1, buf));
    CHECK_SWIM(stlink_swim_do_03(stl, 0x01));
    // 0xb4
    buf[0] = STM8_SWIM_CSR_SAFE_MASK |
             STM8_SWIM_CSR_SWIM_DM |
             STM8_SWIM_CSR_HS |
             STM8_SWIM_CSR_RST;
    CHECK_SWIM(stlink_swim_write(stl, STM8_SWIM_CSR, 1, buf));

    buf[0] = 0x00;
    CHECK_SWIM(stlink_swim_write(stl, STM8S105_CLK_CKDIVR, 1, buf));

#if 0
    // ??? boot ROM
    SWIM_READ(0x67f0, 6, buf);
    dump_data(buf, 6);

    // ??? reserved (between GPIO and periph. reg. and boot ROM)
    SWIM_READ(0x5808, 1, buf);
    dump_data(buf, 1);

    // ??? option bytes
    SWIM_READ(0x488e, 2, buf);
    dump_data(buf, 2);

    // Read-out protection (ROP)
    SWIM_READ(STM8S105_OPT0, 1, buf);
    dump_data(buf, 1);

    // User boot code (UBC)
    SWIM_READ(STM8S105_OPT1, 1, buf);
    dump_data(buf, 1);
    SWIM_READ(STM8S105_NOPT1, 1, buf);
    dump_data(buf, 1);
#endif

    // Flash program memory
    uint32_t flash_start = 0x8000;
    uint16_t flash_size = 32 * 1024;
    uint32_t flash_end = flash_start + flash_size;
    for (uint32_t addr = flash_start; addr < flash_end; addr += size) {
        uint16_t len = (addr + size > flash_end) ? (flash_end - addr) : size;
        SWIM_READ(addr, len, buf);
        dump_data(buf, len);
    }

    SWIM_READ(STM8_SWIM_CSR, 1, buf);
    dump_data(buf, 1);

    // 0xb6
    buf[0] = STM8_SWIM_CSR_SAFE_MASK |
             STM8_SWIM_CSR_SWIM_DM |
             STM8_SWIM_CSR_HS |
             STM8_SWIM_CSR_RST |
             STM8_SWIM_CSR_HSIT;
    CHECK_SWIM(stlink_swim_write(stl, STM8_SWIM_CSR, 1, buf));
    CHECK_SWIM(stlink_swim_do_05(stl, 0x7f80, 1, 0xb6));
    // demo resumes blinking
    CHECK_SWIM(stlink_swim_do_03(stl, 0x00));
    CHECK_SWIM(stlink_swim_do_07(stl, 0x7f80, 1, 0xb6));
    // demo stops blinking

    // ---

    CHECK_SWIM(stlink_swim_do_07(stl, 0x7f80, 1, 0xb6));
    CHECK_SWIM(stlink_swim_do_08(stl, 0x7f80, 1, 0xb6));
    // demo resumes blinking
    CHECK_SWIM(stlink_swim_do_07(stl, 0x7f80, 1, 0xb6));
    // demo stops blinking
    CHECK_SWIM(stlink_swim_do_04(stl, 0x7f80, 1, 0xb6));
    CHECK_SWIM(stlink_swim_do_03(stl, 0x00));
    CHECK_SWIM(stlink_swim_do_05(stl, 0x7f80, 1, 0xb6));

    buf[0] = 0xa0;
    CHECK_SWIM(stlink_swim_write(stl, STM8_SWIM_CSR, 1, buf));
    CHECK_SWIM(stlink_swim_do_08(stl, 0x7f80, 1, 0xa0));

    SWIM_READ(STM8_DM_CSR2, 1, buf);
    dump_data(buf, 1);

    CHECK_SWIM(stlink_swim_do_06(stl, 0x7f99, 1, 0xa0));
    buf[0] = 0xb0;
    CHECK_SWIM(stlink_swim_write(stl, STM8_SWIM_CSR, 1, buf));
    CHECK_SWIM(stlink_swim_do_03(stl, 0x01));
    buf[0] = 0xb4;
    CHECK_SWIM(stlink_swim_write(stl, STM8_SWIM_CSR, 1, buf));

    buf[0] = 0x00;
    CHECK_SWIM(stlink_swim_write(stl, STM8S105_CLK_CKDIVR, 1, buf));

#if 0
    // ??? boot ROM
    SWIM_READ(0x67f0, 6, buf);
    dump_data(buf, 6);

    // ??? reserved (between GPIO and periph. reg. and boot ROM)
    SWIM_READ(0x5808, 1, buf);
    dump_data(buf, 1);

    // ??? option bytes
    SWIM_READ(0x488e, 2, buf);
    dump_data(buf, 2);

    // Read-out protection (ROP)
    SWIM_READ(STM8S105_OPT0, 1, buf);
    dump_data(buf, 1);

    // User boot code (UBC)
    SWIM_READ(STM8S105_OPT1, 1, buf);
    dump_data(buf, 1);
    SWIM_READ(STM8S105_NOPT1, 1, buf);
    dump_data(buf, 1);
#endif

    uint32_t eeprom_start = 0x4000;
    uint16_t eeprom_size = 1024;
    uint32_t eeprom_end = eeprom_start + eeprom_size;
    for (uint32_t addr = eeprom_start; addr < eeprom_end; addr += size) {
        uint16_t len = (addr + size > eeprom_end) ? (eeprom_end - addr) : size;
        SWIM_READ(addr, len, buf);
        dump_data(buf, len);
    }

    SWIM_READ(STM8_SWIM_CSR, 1, buf);
    dump_data(buf, 1);

    buf[0] = 0xb6;
    CHECK_SWIM(stlink_swim_write(stl, STM8_SWIM_CSR, 1, buf));
    CHECK_SWIM(stlink_swim_do_05(stl, 0x7f80, 1, 0xb6));
    // demo resumes blinking
    CHECK_SWIM(stlink_swim_do_03(stl, 0x00));
    CHECK_SWIM(stlink_swim_do_07(stl, 0x7f80, 1, 0xb6));
    // demo stops blinking

    // ---

    CHECK_SWIM(stlink_swim_do_07(stl, 0x7f80, 1, 0xb6));
    CHECK_SWIM(stlink_swim_do_08(stl, 0x7f80, 1, 0xb6));
    // demo resumes blinking
    CHECK_SWIM(stlink_swim_do_07(stl, 0x7f80, 1, 0xb6));
    // demo stops blinking
    CHECK_SWIM(stlink_swim_do_04(stl, 0x7f80, 1, 0xb6));
    CHECK_SWIM(stlink_swim_do_03(stl, 0x00));
    CHECK_SWIM(stlink_swim_do_05(stl, 0x7f80, 1, 0xb6));

    buf[0] = 0xa0;
    CHECK_SWIM(stlink_swim_write(stl, STM8_SWIM_CSR, 1, buf));
    CHECK_SWIM(stlink_swim_do_08(stl, 0x7f80, 1, 0xa0));

    SWIM_READ(STM8_DM_CSR2, 1, buf);
    dump_data(buf, 1);

    CHECK_SWIM(stlink_swim_do_06(stl, 0x7f99, 1, 0xa0));
    buf[0] = 0xb0;
    CHECK_SWIM(stlink_swim_write(stl, STM8_SWIM_CSR, 1, buf));
    CHECK_SWIM(stlink_swim_do_03(stl, 0x01));
    buf[0] = 0xb4;
    CHECK_SWIM(stlink_swim_write(stl, STM8_SWIM_CSR, 1, buf));

    buf[0] = 0x00;
    CHECK_SWIM(stlink_swim_write(stl, STM8S105_CLK_CKDIVR, 1, buf));

#if 0
    // ??? boot ROM
    SWIM_READ(0x67f0, 6, buf);
    dump_data(buf, 6);

    // ??? reserved (between GPIO and periph. reg. and boot ROM)
    SWIM_READ(0x5808, 1, buf);
    dump_data(buf, 1);

    // ??? option bytes
    SWIM_READ(0x488e, 2, buf);
    dump_data(buf, 2);

    // Read-out protection (ROP)
    SWIM_READ(STM8S105_OPT0, 1, buf);
    dump_data(buf, 1);

    // User boot code (UBC)
    SWIM_READ(STM8S105_OPT1, 1, buf);
    dump_data(buf, 1);
    SWIM_READ(STM8S105_NOPT1, 1, buf);
    dump_data(buf, 1);
#endif

    // Option bytes
    for (uint32_t addr = 0x4800; addr <= 0x480e; addr++) {
        SWIM_READ(addr, 1, buf);
        dump_data(buf, 1);
    }
    SWIM_READ(STM8S105_OPTBL, 1, buf);
    dump_data(buf, 1);

    SWIM_READ(STM8_SWIM_CSR, 1, buf);
    dump_data(buf, 1);

    buf[0] = 0xb6;
    CHECK_SWIM(stlink_swim_write(stl, STM8_SWIM_CSR, 1, buf));
    CHECK_SWIM(stlink_swim_do_05(stl, 0x7f80, 1, 0xb6));
    // demo resumes blinking
    CHECK_SWIM(stlink_swim_do_03(stl, 0x00));
    CHECK_SWIM(stlink_swim_do_07(stl, 0x7f80, 1, 0xb6));
    // demo stops blinking

    free(buf);
}

__attribute__((unused))
static void swim_flash(stlink *stl)
{
    int ret;
    uint8_t *buf; //XXX

    CHECK_SWIM(stlink_swim_do_07(stl, 0x7f80, 1, 0xb6));
    CHECK_SWIM(stlink_swim_do_08(stl, 0x7f80, 1, 0xb6));
    CHECK_SWIM(stlink_swim_do_07(stl, 0x7f80, 1, 0xb6));
    CHECK_SWIM(stlink_swim_do_04(stl, 0x7f80, 1, 0xb6));
    CHECK_SWIM(stlink_swim_do_03(stl, 0x00));
    CHECK_SWIM(stlink_swim_do_05(stl, 0x7f80, 1, 0xb6));

    buf[0] = 0xa0;
    CHECK_SWIM(stlink_swim_write(stl, STM8_SWIM_CSR, 1, buf));
    CHECK_SWIM(stlink_swim_do_08(stl, 0x7f80, 1, 0xa0));

    SWIM_READ(STM8_DM_CSR2, 1, buf);
    dump_data(buf, 1);

    CHECK_SWIM(stlink_swim_do_06(stl, 0x7f99, 1, 0xa0));

    buf[0] = 0xb0;
    CHECK_SWIM(stlink_swim_write(stl, STM8_SWIM_CSR, 1, buf));
    CHECK_SWIM(stlink_swim_do_03(stl, 0x01));
    buf[0] = 0xb4;
    CHECK_SWIM(stlink_swim_write(stl, STM8_SWIM_CSR, 1, buf));

    // ??? GPIO and periph. reg.
    buf[0] = 0x00;
    CHECK_SWIM(stlink_swim_write(stl, 0x50c6, 1, buf));

#if 0
    // ??? boot ROM
    SWIM_READ(0x67f0, 6, buf);
    dump_data(buf, 6);

    // ??? reserved (between GPIO and periph. reg. and boot ROM)
    SWIM_READ(0x5808, 1, buf);
    dump_data(buf, 1);

    // ??? option bytes
    SWIM_READ(0x488e, 2, buf);
    dump_data(buf, 2);

    // Read-out protection (ROP)
    SWIM_READ(0x4800, 1, buf);
    dump_data(buf, 1);

    // User boot code (UBC) (OPT1)
    SWIM_READ(0x4801, 1, buf);
    dump_data(buf, 1);

    // User boot code (UBC) (NOPT1)
    SWIM_READ(0x4802, 1, buf);
    dump_data(buf, 1);
#endif

    // Unlock program memory (FLASH_PUKR)
    buf[0] = 0x56;
    CHECK_SWIM(stlink_swim_write(stl, 0x5062, 1, buf));
    buf[0] = 0xae;
    CHECK_SWIM(stlink_swim_write(stl, 0x5062, 1, buf));
    // Unlock data EEPROM and option bytes (FLASH_DUKR)
    buf[0] = 0xae;
    CHECK_SWIM(stlink_swim_write(stl, 0x5064, 1, buf));
    buf[0] = 0x56;
    CHECK_SWIM(stlink_swim_write(stl, 0x5064, 1, buf));
    // FLASH_IAPSR
    SWIM_READ(0x505f, 1, buf);
    dump_data(buf, 1);

    // -> RAM
    // XXX buf
    CHECK_SWIM(stlink_swim_write(stl, 0x0030, 0x200, buf));
    // XXX buf
    CHECK_SWIM(stlink_swim_write(stl, 0x000f, 20, buf));

    buf[0] = 0x00;
    CHECK_SWIM(stlink_swim_write(stl, 0x0330, 1, buf));
    buf[0] = 0x00;
    CHECK_SWIM(stlink_swim_write(stl, 0x0532, 1, buf));
    buf[0] = 0x00;
    CHECK_SWIM(stlink_swim_write(stl, 0x012f, 1, buf));
    buf[0] = 0x00;
    CHECK_SWIM(stlink_swim_write(stl, 0x0331, 1, buf));
    buf[0] = 0x00;
    CHECK_SWIM(stlink_swim_write(stl, 0x50c6, 1, buf));
    buf[0] = 0x00;
    buf[1] = 0x00;
    buf[2] = 0x30;
    CHECK_SWIM(stlink_swim_write(stl, 0x7f01, 3, buf));
    buf[0] = 0xe8;
    CHECK_SWIM(stlink_swim_write(stl, 0x7f0a, 1, buf));

    SWIM_READ(0x7f99, 1, buf);
    dump_data(buf, 1);
    buf[0] = 0x09;
    CHECK_SWIM(stlink_swim_write(stl, STM8_DM_CSR2, 1, buf));
    buf[0] = 0x01;
    CHECK_SWIM(stlink_swim_write(stl, STM8_DM_CSR2, 1, buf));

    // XXX buf
    CHECK_SWIM(stlink_swim_write(stl, 0x012f, 0x0202, buf));
    // XXX buf
    CHECK_SWIM(stlink_swim_write(stl, 0x0331, 0x0202, buf));
    SWIM_READ(0x012f, 1, buf);
    dump_data(buf, 1);
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
        stlink_swim_exit(stl);
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
