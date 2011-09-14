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

#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "bswap.h"
#include "stlink.h"


static inline void dump_cdb(uint8_t *cdb, uint8_t len)
{
    printf("%s: CDB:", __func__);
    for (int i = 0; i < len; i++) {
        printf(" %02" PRIX8, cdb[i]);
    }
    printf("\n");
}

void stlink_get_version(stlink *stl)
{
    printf("getting version...\n");
    uint8_t cdb[6]; // sic!
    memset(cdb, 0, sizeof(cdb));
    cdb[0] = STLINK_GET_VERSION;
    unsigned char buf[6];
    int ret = stlink_send_command(stl, cdb, sizeof(cdb), buf, sizeof(buf), true);
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
    int ret = stlink_send_command(stl, cdb, sizeof(cdb), buf, sizeof(buf), true);
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
    int ret = stlink_send_command(stl, cdb, sizeof(cdb), buf, 0, true);
    if (ret != 0) {
        fprintf(stderr, "%s: command failed: %d\n", __func__, ret);
        return;
    }
    printf("exited DFU mode\n");
}

void stlink_enter_swd_mode(stlink *stl)
{
    printf("entering SWD mode...\n");
    uint8_t cdb[10];
    memset(cdb, 0, sizeof(cdb));
    cdb[0] = STLINK_DEBUG_COMMAND;
    cdb[1] = STLINK_DEBUG_ENTER;
    cdb[2] = STLINK_DEBUG_ENTER_SWD;
    int ret = stlink_send_command(stl, cdb, sizeof(cdb), NULL, 0, true);
    if (ret != 0) {
        fprintf(stderr, "%s: command failed: %d\n", __func__, ret);
        return;
    }
    printf("entered SWD mode\n");
}

void stlink_swim_enter(stlink *stl)
{
    printf("entering SWIM mode...\n");
    uint8_t cdb[2];
    memset(cdb, 0, sizeof(cdb));
    cdb[0] = STLINK_SWIM_COMMAND;
    cdb[1] = STLINK_SWIM_ENTER;
    int ret = stlink_send_command(stl, cdb, sizeof(cdb), NULL, 0, true);
    if (ret != 0) {
        fprintf(stderr, "%s: command failed: %d\n", __func__, ret);
        return;
    }
    printf("entered SWIM mode\n");
}

int stlink_swim_exit(stlink *stl)
{
    printf("exiting SWIM mode...\n");
    uint8_t cdb[2];
    memset(cdb, 0, sizeof(cdb));
    cdb[0] = STLINK_SWIM_COMMAND;
    cdb[1] = STLINK_SWIM_EXIT;
    int ret = stlink_send_command(stl, cdb, sizeof(cdb), NULL, 0, true);
    if (ret != 0) {
        fprintf(stderr, "%s: command failed: %d\n", __func__, ret);
        return -1;
    }
    printf("exited SWIM mode\n");
    return 0;
}

int stlink_swim_get_size(stlink *stl, uint16_t *size)
{
    printf("reading size...\n");
    uint8_t cdb[2];
    memset(cdb, 0, sizeof(cdb));
    cdb[0] = STLINK_SWIM_COMMAND;
    cdb[1] = STLINK_SWIM_GET_SIZE;
    uint16_t buf;
    int ret = stlink_send_command(stl, cdb, sizeof(cdb), (unsigned char *)&buf, sizeof(buf), true);
    if (ret != 0) {
        fprintf(stderr, "%s: command failed: %d\n", __func__, ret);
        return -1;
    }
    *size = le16_to_cpu(buf);
    return 0;
}

int stlink_swim_get_02(stlink *stl, uint8_t x)
{
    printf("reading 0x02...\n");
    uint8_t cdb[3];
    memset(cdb, 0, sizeof(cdb));
    cdb[0] = STLINK_SWIM_COMMAND;
    cdb[1] = STLINK_SWIM_GET_02;
    cdb[2] = x;
    uint8_t buf[8];
    int ret = stlink_send_command(stl, cdb, sizeof(cdb), buf, sizeof(buf), true);
    if (ret != 0) {
        fprintf(stderr, "%s: command failed: %d\n", __func__, ret);
        return -1;
    }
    printf("%s:", __func__);
    for (int i = 0; i < sizeof(buf); i++) {
        printf(" %02" PRIX8, buf[i]);
    }
    printf("\n");
    return 0;
}

int stlink_swim_do_03(stlink *stl, uint8_t x)
{
    printf("doing 0x03...\n");
    uint8_t cdb[3];
    memset(cdb, 0, sizeof(cdb));
    cdb[0] = STLINK_SWIM_COMMAND;
    cdb[1] = STLINK_SWIM_DO_03;
    cdb[2] = x;
    int ret = stlink_send_command(stl, cdb, sizeof(cdb), NULL, 0, true);
    if (ret != 0) {
        fprintf(stderr, "%s: command failed: %d\n", __func__, ret);
        return -1;
    }
    return 0;
}

int stlink_swim_do_04(stlink *stl)
{
    printf("doing 0x04...\n");
    uint8_t cdb[2]; // 10
    memset(cdb, 0, sizeof(cdb));
    cdb[0] = STLINK_SWIM_COMMAND;
    cdb[1] = STLINK_SWIM_DO_04;
    int ret = stlink_send_command(stl, cdb, sizeof(cdb), NULL, 0, true);
    if (ret != 0) {
        fprintf(stderr, "%s: command failed: %d\n", __func__, ret);
        return -1;
    }
    return 0;
}

int stlink_swim_do_05(stlink *stl)
{
    printf("doing 0x05...\n");
    uint8_t cdb[2]; // 10
    memset(cdb, 0, sizeof(cdb));
    cdb[0] = STLINK_SWIM_COMMAND;
    cdb[1] = STLINK_SWIM_DO_05;
    int ret = stlink_send_command(stl, cdb, sizeof(cdb), NULL, 0, true);
    if (ret != 0) {
        fprintf(stderr, "%s: command failed: %d\n", __func__, ret);
        return -1;
    }
    return 0;
}

int stlink_swim_do_06(stlink *stl)
{
    printf("doing 0x06...\n");
    uint8_t cdb[2]; // 10
    memset(cdb, 0, sizeof(cdb));
    cdb[0] = STLINK_SWIM_COMMAND;
    cdb[1] = STLINK_SWIM_DO_06;
    int ret = stlink_send_command(stl, cdb, sizeof(cdb), NULL, 0, true);
    if (ret != 0) {
        fprintf(stderr, "%s: command failed: %d\n", __func__, ret);
        return -1;
    }
    return 0;
}

int stlink_swim_do_07(stlink *stl)
{
    printf("doing 0x07...\n");
    uint8_t cdb[2]; // 10
    memset(cdb, 0, sizeof(cdb));
    cdb[0] = STLINK_SWIM_COMMAND;
    cdb[1] = STLINK_SWIM_DO_07;
    int ret = stlink_send_command(stl, cdb, sizeof(cdb), NULL, 0, true);
    if (ret != 0) {
        fprintf(stderr, "%s: command failed: %d\n", __func__, ret);
        return -1;
    }
    return 0;
}

int stlink_swim_do_08(stlink *stl)
{
    printf("doing 0x08...\n");
    uint8_t cdb[2]; // 10
    memset(cdb, 0, sizeof(cdb));
    cdb[0] = STLINK_SWIM_COMMAND;
    cdb[1] = STLINK_SWIM_DO_08;
    int ret = stlink_send_command(stl, cdb, sizeof(cdb), NULL, 0, true);
    if (ret != 0) {
        fprintf(stderr, "%s: command failed: %d\n", __func__, ret);
        return -1;
    }
    return 0;
}

int stlink_swim_get_busy(stlink *stl, uint32_t *status)
{
    printf("reading 0x09...\n");
    uint8_t cdb[2]; // 10
    memset(cdb, 0, sizeof(cdb));
    cdb[0] = STLINK_SWIM_COMMAND;
    cdb[1] = STLINK_SWIM_GET_BUSY;
    uint8_t buf[4];
    int ret = stlink_send_command(stl, cdb, sizeof(cdb), buf, sizeof(buf), true);
    if (ret != 0) {
        fprintf(stderr, "%s: command failed: %d\n", __func__, ret);
        return -1;
    }
#if 0
    printf("%s:", __func__);
    for (int i = 0; i < sizeof(buf); i++) {
        printf(" %02" PRIX8, buf[i]);
    }
    printf("\n");
#endif
    uint32_t count = (buf[3] << 16) | (buf[2] << 8) | buf[1];
    printf("%s: busy = 0x%02" PRIX8 ", count = 0x%06" PRIx32 "\n", __func__, buf[0], count);
    *status = le32_to_cpu(*(uint32_t *)buf);
    return 0;
}

int stlink_swim_write(stlink *stl, uint32_t addr, uint16_t len, uint8_t *buffer)
{
    printf("writing at 0x%06" PRIx32 " (0x%" PRIx16 ")...\n", addr, len);
    uint8_t cdb[16];
    memset(cdb, 0, sizeof(cdb));
    cdb[0] = STLINK_SWIM_COMMAND;
    cdb[1] = STLINK_SWIM_DO_0A;
    *(uint16_t *)&cdb[2] = cpu_to_be16(len);
    *(uint32_t *)&cdb[4] = cpu_to_be32(addr);
    memcpy(&cdb[8], buffer, (len > 8) ? 8 : len);
    int ret = stlink_send_command(stl, cdb, sizeof(cdb), buffer + 8, (len > 8) ? (len - 8) : 0, false);
    if (ret != 0) {
        fprintf(stderr, "%s: command failed: %d\n", __func__, ret);
        return -1;
    }
    return 0;
}

int stlink_swim_begin_read(stlink *stl, uint32_t addr, uint16_t len)
{
    printf("initiating read at 0x%06" PRIx32 " (0x%" PRIx16 ")...\n", addr, len);
    uint8_t cdb[8]; // 10
    memset(cdb, 0, sizeof(cdb));
    cdb[0] = STLINK_SWIM_COMMAND;
    cdb[1] = STLINK_SWIM_BEGIN_READ;
    *(uint16_t *)&cdb[2] = cpu_to_be16(len);
    *(uint32_t *)&cdb[4] = cpu_to_be32(addr);
    int ret = stlink_send_command(stl, cdb, sizeof(cdb), NULL, 0, true);
    if (ret != 0) {
        fprintf(stderr, "%s: command failed: %d\n", __func__, ret);
        return -1;
    }
    return 0;
}

int stlink_swim_read(stlink *stl, uint16_t length, uint8_t *buffer)
{
    printf("reading 0x%" PRIx16 " bytes...\n", length);
    uint8_t cdb[2]; // 10
    memset(cdb, 0, sizeof(cdb));
    cdb[0] = STLINK_SWIM_COMMAND;
    cdb[1] = STLINK_SWIM_READ;
    int ret = stlink_send_command(stl, cdb, sizeof(cdb), buffer, length, true);
    if (ret != 0) {
        fprintf(stderr, "%s: command failed: %d\n", __func__, ret);
        return -1;
    }
    return 0;
}
