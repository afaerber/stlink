#ifndef STLINK_H
#define STLINK_H


#define USB_ST_VID      0x0483
#define USB_STLINK_PID  0x3744


enum CDBOpcodes {
    STLINK_GET_VERSION      = 0xf1,
    STLINK_DEBUG_COMMAND    = 0xf2,
    STLINK_DFU_COMMAND      = 0xf3,
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


enum {
    STLINK_DEV_DFU_MODE     = 0x00,
    STLINK_DEV_MASS_MODE    = 0x01,
    STLINK_DEV_DEBUG_MODE   = 0x02,
};


#endif
