/*
 * Copyright (c) 2011 Andreas Färber <andreas.faerber@web.de>
 *
 * CDBs and magic values derived from <https://github.com/texane/stlink>:
 * Copyright (c) 2010 "Capt'ns Missing Link" Authors. All rights reserved.
 * Author: Martin Capitanio <m@capitanio.org>
 * The stlink related constants kindly provided by Oliver Spencer (OpenOCD)
 * for use in a GPL compatible license.
 */

#include "stlink-libusb.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "bswap.h"
#include "stlink.h"


void stlink_get_version(stlink *stl)
{
    printf("getting version...\n");
    uint8_t cdb[10];
    memset(cdb, 0, sizeof(cdb));
    cdb[0] = STLINK_GET_VERSION;
    unsigned char buf[6];
    int ret = stlink_send_command(stl, cdb, sizeof(cdb), buf, sizeof(buf));
    if (ret != 0) {
        fprintf(stderr, "%s: command failed: %d\n", __func__, ret);
        return;
    }
    printf("version: %02X %02X %02X %02X %02X %02X\n", buf[0], buf[1], buf[2], buf[3], buf[4], buf[5]);
    uint16_t vid = le16_to_cpu(*(uint16_t *)&buf[2]);
    uint16_t pid = le16_to_cpu(*(uint16_t *)&buf[4]);
    printf("vid = 0x%04X, pid = 0x%04X\n", vid, pid);
    uint8_t stlink_v = buf[0] >> 4;
    uint8_t jtag_v = ((buf[0] & 0xf) << 2) | (buf[1] >> 6);
    uint8_t swim_v = buf[1] & 0x3f;
    printf("stlink_v = %" PRIu8 ", jtag_v = %" PRIu8 ", swim_v = %" PRIu8 "\n",
           stlink_v, jtag_v, swim_v);
}

int stlink_get_current_mode(stlink *stl)
{
    printf("getting current mode...\n");
    uint8_t cdb[10];
    memset(cdb, 0, sizeof(cdb));
    cdb[0] = STLINK_GET_CURRENT_MODE;
    unsigned char buf[2];
    int ret = stlink_send_command(stl, cdb, sizeof(cdb), buf, sizeof(buf));
    if (ret != 0) {
        fprintf(stderr, "%s: command failed: %d\n", __func__, ret);
        return -1;
    }
    printf("current mode: %02X %02X\n", buf[0], buf[1]);
    return buf[0];
}

void stlink_exit_dfu_mode(stlink *stl)
{
    printf("exiting DFU mode...\n");
    uint8_t cdb[10];
    memset(cdb, 0, sizeof(cdb));
    cdb[0] = STLINK_DFU_COMMAND;
    cdb[1] = STLINK_DFU_EXIT;
    unsigned char buf[2];
    int ret = stlink_send_command(stl, cdb, sizeof(cdb), buf, 0);
    if (ret != 0) {
        fprintf(stderr, "%s: command failed: %d\n", __func__, ret);
        return;
    }
    printf("exited DFU mode\n");
}

void stlink_enter_swd_mode(stlink *stl)
{
    printf("entering SWD mode...\n");
    uint8_t cdb[6];
    memset(cdb, 0, sizeof(cdb));
    cdb[0] = STLINK_DEBUG_COMMAND;
    cdb[1] = STLINK_DEBUG_ENTER;
    cdb[2] = STLINK_DEBUG_ENTER_SWD;
    int ret = stlink_send_command(stl, cdb, sizeof(cdb), NULL, 0);
    if (ret != 0) {
        fprintf(stderr, "%s: command failed: %d\n", __func__, ret);
        return;
    }
    printf("entered SWD mode\n");
}
