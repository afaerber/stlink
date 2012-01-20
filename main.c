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
#include "stm8.h"

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
    // 01 during read
    // 04 if missing prologue
    uint32_t busy;
    do {
        ret = stlink_swim_get_busy(stl, &busy);
    } while ((busy & 0xff) && ret == 0);
    return ret;
}

#define CHECK_SWIM(x) \
    ret = x; \
    if (ret != 0) \
        return -1; \
    ret = swim_poll(stl); \
    if (ret != 0) \
        return -1

#define SWIM_READ(addr, len, buf) \
    ret = stlink_swim_begin_read(stl, addr, len); \
    if (ret != 0) \
        return -1; \
    ret = swim_poll(stl); \
    if (ret != 0) \
        return -1; \
    ret = stlink_swim_read(stl, len, buf); \
    if (ret != 0) \
        return -1

static int swim_prologue(stlink *stl)
{
    int ret;
    uint8_t buf[2];

    CHECK_SWIM(stlink_swim_do_07(stl));
    CHECK_SWIM(stlink_swim_do_08(stl));
    CHECK_SWIM(stlink_swim_do_07(stl));
    CHECK_SWIM(stlink_swim_do_04(stl)); // causes demo to stop blinking
    CHECK_SWIM(stlink_swim_do_03(stl, 0x00));
    CHECK_SWIM(stlink_swim_do_05(stl));

    // 0xa0
    buf[0] = STM8_SWIM_CSR_SAFE_MASK |
             STM8_SWIM_CSR_SWIM_DM;
    CHECK_SWIM(stlink_swim_write(stl, STM8_SWIM_CSR, 1, buf));
    CHECK_SWIM(stlink_swim_do_08(stl));

    SWIM_READ(STM8_DM_CSR2, 1, buf);
    dump_data(buf, 1);

    CHECK_SWIM(stlink_swim_do_06(stl));
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

    return 0;
}

static int swim_epilogue(stlink *stl)
{
    int ret;
    uint8_t buf[2];

    SWIM_READ(STM8_SWIM_CSR, 1, buf);
    dump_data(buf, 1);

    // 0xb6
    buf[0] = STM8_SWIM_CSR_SAFE_MASK |
             STM8_SWIM_CSR_SWIM_DM |
             STM8_SWIM_CSR_HS |
             STM8_SWIM_CSR_RST |
             STM8_SWIM_CSR_HSIT;
    CHECK_SWIM(stlink_swim_write(stl, STM8_SWIM_CSR, 1, buf));
    CHECK_SWIM(stlink_swim_do_05(stl));
    // demo resumes blinking
    CHECK_SWIM(stlink_swim_do_03(stl, 0x00));
    CHECK_SWIM(stlink_swim_do_07(stl));
    // demo stops blinking

    return 0;
}

static int swim(stlink *stl)
{
    uint16_t size = 0;
    int ret = stlink_swim_get_size(stl, &size);
    if (ret != 0)
        return -1;
    printf("size = 0x%" PRIx16 "\n", size);

    uint8_t *buf = malloc(size);

    ret = stlink_swim_get_02(stl, 0x01);
    if (ret != 0)
        return -1;
    CHECK_SWIM(stlink_swim_do_07(stl));

    ret = swim_prologue(stl);
    if (ret != 0)
        return -1;

    // Flash program memory
    uint32_t flash_start = 0x8000;
    uint16_t flash_size = 32 * 1024;
    uint32_t flash_end = flash_start + flash_size;
    FILE *file = fopen("flash.bin", "w");
    if (file == NULL)
        return -1;
    for (uint32_t addr = flash_start; addr < flash_end; addr += size) {
        uint16_t len = (addr + size > flash_end) ? (flash_end - addr) : size;
        ret = stlink_swim_begin_read(stl, addr, len);
        if (ret != 0) {
            fclose(file);
            return -1;
        }
        ret = swim_poll(stl);
        if (ret != 0) {
            fclose(file);
            return -1;
        }
        ret = stlink_swim_read(stl, len, buf);
        if (ret != 0) {
            fclose(file);
            return -1;
        }
        dump_data(buf, len);
        if (fwrite(buf, 1, len, file) < len) {
            fclose(file);
            return -1;
        }
    }
    fclose(file);

    ret = swim_epilogue(stl);
    if (ret != 0)
        return -1;

    // ---

    ret = swim_prologue(stl);
    if (ret != 0)
        return -1;

    uint32_t eeprom_start = 0x4000;
    uint16_t eeprom_size = 1024;
    uint32_t eeprom_end = eeprom_start + eeprom_size;
    for (uint32_t addr = eeprom_start; addr < eeprom_end; addr += size) {
        uint16_t len = (addr + size > eeprom_end) ? (eeprom_end - addr) : size;
        SWIM_READ(addr, len, buf);
        dump_data(buf, len);
    }

    ret = swim_epilogue(stl);
    if (ret != 0)
        return -1;

    // ---

    ret = swim_prologue(stl);
    if (ret != 0)
        return -1;

    // Option bytes
    for (uint32_t addr = 0x4800; addr <= 0x480e; addr++) {
        SWIM_READ(addr, 1, buf);
        dump_data(buf, 1);
    }
    SWIM_READ(STM8S105_OPTBL, 1, buf);
    dump_data(buf, 1);

    ret = swim_epilogue(stl);
    if (ret != 0)
        return -1;

    free(buf);

    return 0;
}

__attribute__((unused))
static int swim_flash(stlink *stl)
{
    uint16_t size = 0;
    int ret = stlink_swim_get_size(stl, &size);
    if (ret != 0)
        return -1;
    printf("size = 0x%" PRIx16 "\n", size);

    uint8_t *buf = malloc(size);

    ret = swim_prologue(stl);
    if (ret != 0)
        return -1;

    // Unlock program memory
    buf[0] = 0x56;
    CHECK_SWIM(stlink_swim_write(stl, STM8S105_FLASH_PUKR, 1, buf));
    buf[0] = 0xae;
    CHECK_SWIM(stlink_swim_write(stl, STM8S105_FLASH_PUKR, 1, buf));
    // Unlock data EEPROM and option bytes
    buf[0] = 0xae;
    CHECK_SWIM(stlink_swim_write(stl, STM8S105_FLASH_DUKR, 1, buf));
    buf[0] = 0x56;
    CHECK_SWIM(stlink_swim_write(stl, STM8S105_FLASH_DUKR, 1, buf));
    SWIM_READ(STM8S105_FLASH_IAPSR, 1, buf);
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
    CHECK_SWIM(stlink_swim_write(stl, STM8S105_CLK_CKDIVR, 1, buf));
    buf[0] = 0x00;
    buf[1] = 0x00;
    buf[2] = 0x30;
    CHECK_SWIM(stlink_swim_write(stl, STM8_REG_PCE, 3, buf));
    buf[0] = 0xe8;
    CHECK_SWIM(stlink_swim_write(stl, STM8_REG_CC, 1, buf));

    SWIM_READ(STM8_DM_CSR2, 1, buf);
    dump_data(buf, 1);
    buf[0] = STM8_DM_CSR2_STALL | STM8_DM_CSR2_FLUSH;
    CHECK_SWIM(stlink_swim_write(stl, STM8_DM_CSR2, 1, buf));
    buf[0] = STM8_DM_CSR2_FLUSH;
    CHECK_SWIM(stlink_swim_write(stl, STM8_DM_CSR2, 1, buf));

    // XXX buf
    CHECK_SWIM(stlink_swim_write(stl, 0x012f, 0x0202, buf));
    // XXX buf
    CHECK_SWIM(stlink_swim_write(stl, 0x0331, 0x0202, buf));
    SWIM_READ(0x012f, 1, buf);
    dump_data(buf, 1);

    return 0;
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
