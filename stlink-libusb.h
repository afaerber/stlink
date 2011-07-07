#ifndef STLINK_LIBUSB_H
#define STLINK_LIBUSB_H


#include <stdbool.h>
#include <libusb-1.0/libusb.h>


typedef struct STLink stlink;

stlink *stlink_open(libusb_context *usb_context);
void stlink_close(stlink *stl);
int stlink_send_command(stlink *stl, uint8_t *cdb, uint8_t cdb_length,
                        uint8_t *buffer, int transfer_length, bool inbound);

void stlink_get_version(stlink *stl);
int stlink_get_current_mode(stlink *stl);
void stlink_exit_dfu_mode(stlink *stl);
void stlink_enter_swd_mode(stlink *stl);
void stlink_swim_enter(stlink *stl);
int stlink_swim_exit(stlink *stl);
int stlink_swim_get_size(stlink *stl, uint16_t *size);
int stlink_swim_get_02(stlink *stl, uint8_t x);
int stlink_swim_do_03(stlink *stl, uint8_t x);
int stlink_swim_do_04(stlink *stl);
int stlink_swim_do_05(stlink *stl);
int stlink_swim_do_06(stlink *stl);
int stlink_swim_do_07(stlink *stl);
int stlink_swim_do_08(stlink *stl);
int stlink_swim_get_busy(stlink *stl, uint32_t *status);
int stlink_swim_write(stlink *stl, uint32_t addr, uint16_t len, uint8_t *buffer);
int stlink_swim_begin_read(stlink *stl, uint32_t addr, uint16_t len);
int stlink_swim_read(stlink *stl, uint16_t length, uint8_t *buffer);


#endif
