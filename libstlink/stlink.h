#ifndef STLINK_H
#define STLINK_H


#define USB_VID_ST      0x0483
#define USB_PID_STLINK  0x3744


enum CDBOpcodes {
    STLINK_GET_VERSION      = 0xf1,
    STLINK_DEBUG_COMMAND    = 0xf2,
    STLINK_DFU_COMMAND      = 0xf3,
    STLINK_SWIM_COMMAND     = 0xf4,
    STLINK_GET_CURRENT_MODE = 0xf5,
};

enum {
    STLINK_DEBUG_ENTER  = 0x20,
    STLINK_DEBUG_EXIT   = 0x21,
};
enum {
    STLINK_DEBUG_ENTER_SWD = 0xa3,
};

enum {
    STLINK_DFU_EXIT = 0x07,
};

enum STLinkSWIMCommands {
    STLINK_SWIM_ENTER       = 0x00,
    STLINK_SWIM_EXIT        = 0x01,
    STLINK_SWIM_GET_02      = 0x02,
    STLINK_SWIM_DO_03       = 0x03,
    STLINK_SWIM_DO_04       = 0x04,
    STLINK_SWIM_DO_05       = 0x05,
    STLINK_SWIM_DO_06       = 0x06,
    STLINK_SWIM_DO_07       = 0x07,
    STLINK_SWIM_DO_08       = 0x08,
    STLINK_SWIM_GET_BUSY    = 0x09,
    STLINK_SWIM_DO_0A       = 0x0a,
    STLINK_SWIM_BEGIN_READ  = 0x0b,
    STLINK_SWIM_READ        = 0x0c,
    STLINK_SWIM_GET_SIZE    = 0x0d,
};

enum STLinkModes {
    STLINK_DEV_DFU_MODE     = 0x00,
    STLINK_DEV_MASS_MODE    = 0x01,
    STLINK_DEV_DEBUG_MODE   = 0x02,
    STLINK_DEV_SWIM_MODE    = 0x03,
};


#endif
