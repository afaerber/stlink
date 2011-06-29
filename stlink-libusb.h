#ifndef STLINK_LIBUSB_H
#define STLINK_LIBUSB_H


#include <libusb-1.0/libusb.h>


typedef struct STLink stlink;

stlink *stlink_open(libusb_context *usb_context);
void stlink_close(stlink *stl);
int stlink_send_command(stlink *stl, uint8_t *cdb, uint8_t cdb_length,
                        uint8_t *buffer, int expected_length);

void stlink_get_version(stlink *stl);
int stlink_get_current_mode(stlink *stl);
void stlink_exit_dfu_mode(stlink *stl);
void stlink_enter_swd_mode(stlink *stl);


#endif
