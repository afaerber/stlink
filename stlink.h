#ifndef STLINK_H
#define STLINK_H


#define USB_ST_VID      0x0483
#define USB_STLINK_PID  0x3744


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


// UM0470
enum STM8CPURegisters {
    STM8_REG_A      = 0x7f00,
    STM8_REG_PCE    = 0x7f01,
    STM8_REG_PCH    = 0x7f02,
    STM8_REG_PCL    = 0x7f03,
    STM8_REG_XH     = 0x7f04,
    STM8_REG_XL     = 0x7f05,
    STM8_REG_YH     = 0x7f06,
    STM8_REG_YL     = 0x7f07,
    STM8_REG_SPH    = 0x7f08,
    STM8_REG_SPL    = 0x7f09,
    STM8_REG_CC     = 0x7f0a,
};

// UM0470
enum STM8SWIMRegisters {
    STM8_SWIM_CSR   = 0x7f80,
    // CLK_SWIMCCR product dependent (50CDh)
};

// UM0470
enum STM8DMRegisters {
    STM8_DM_BRK1E   = 0x7f90,
    STM8_DM_BKR1H   = 0x7f91,
    STM8_DM_BRK1L   = 0x7f92,
    STM8_DM_BRK2E   = 0x7f93,
    STM8_DM_BRK2H   = 0x7f94,
    STM8_DM_BRK2L   = 0x7f95,
    STM8_DM_CR1     = 0x7f96,
    STM8_DM_CR2     = 0x7f97,
    STM8_DM_CSR1    = 0x7f98,
    STM8_DM_CSR2    = 0x7f99,
    STM8_DM_ENFCTR  = 0x7f9a,
};

enum STM8S105xxRegisters {
    STM8S105_CLK_CKDIVR     = 0x50c6,
    STM8S105_CLK_SWIMCCR    = 0x50cd,
};


#endif
