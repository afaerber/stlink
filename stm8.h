/*
 * Constants for STMicroelectronics STM8 platform
 *
 * Copyright (c) 2011 Andreas Färber <andreas.faerber@web.de>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef STM8_H
#define STM8_H


// UM0470
enum STM8CPURegisters {
    STM8_REG_A      = 0x007f00,
    STM8_REG_PCE    = 0x007f01,
    STM8_REG_PCH    = 0x007f02,
    STM8_REG_PCL    = 0x007f03,
    STM8_REG_XH     = 0x007f04,
    STM8_REG_XL     = 0x007f05,
    STM8_REG_YH     = 0x007f06,
    STM8_REG_YL     = 0x007f07,
    STM8_REG_SPH    = 0x007f08,
    STM8_REG_SPL    = 0x007f09,
    STM8_REG_CC     = 0x007f0a,
};

// UM0470
enum STM8SWIMRegisters {
    STM8_SWIM_CSR   = 0x007f80,
    // CLK_SWIMCCR product dependent (50CDh)
};

enum STM8SWIMControlStatusRegisterBits {
    STM8_SWIM_CSR_SAFE_MASK = 1 << 7,
    STM8_SWIM_CSR_NO_ACCESS = 1 << 6,
    STM8_SWIM_CSR_SWIM_DM   = 1 << 5,
    STM8_SWIM_CSR_HS        = 1 << 4,
    STM8_SWIM_CSR_OSCOFF    = 1 << 3,
    STM8_SWIM_CSR_RST       = 1 << 2,
    STM8_SWIM_CSR_HSIT      = 1 << 1,
    STM8_SWIM_CSR_PRI       = 1 << 0,
};

// UM0470
enum STM8DMRegisters {
    STM8_DM_BRK1E   = 0x007f90,
    STM8_DM_BKR1H   = 0x007f91,
    STM8_DM_BRK1L   = 0x007f92,
    STM8_DM_BRK2E   = 0x007f93,
    STM8_DM_BRK2H   = 0x007f94,
    STM8_DM_BRK2L   = 0x007f95,
    STM8_DM_CR1     = 0x007f96,
    STM8_DM_CR2     = 0x007f97,
    STM8_DM_CSR1    = 0x007f98,
    STM8_DM_CSR2    = 0x007f99,
    STM8_DM_ENFCTR  = 0x007f9a,
};

enum STM8DMControlStatusRegister2Bits {
    STM8_DM_CSR2_SWBKE  = 1 << 5,
    STM8_DM_CSR2_SWBKF  = 1 << 4,
    STM8_DM_CSR2_STALL  = 1 << 3,
    STM8_DM_CSR2_FLUSH  = 1 << 0,
};

enum STM8S105xxRegisters {
    STM8S105_OPT0   = 0x004800,
    STM8S105_OPT1   = 0x004801,
    STM8S105_NOPT1  = 0x004802,
    STM8S105_OPT2   = 0x004803,
    STM8S105_NOPT2  = 0x004804,
    STM8S105_OPT3   = 0x004805,
    STM8S105_NOPT3  = 0x004806,
    STM8S105_OPT4   = 0x004807,
    STM8S105_NOPT4  = 0x004808,
    STM8S105_OPT5   = 0x004809,
    STM8S105_NOPT5  = 0x00480a,
    STM8S105_OPT6   = 0x00480b,
    STM8S105_NOPT6  = 0x00480c,
    STM8S105_OPT7   = 0x00480d,
    STM8S105_NOPT7  = 0x00480e,
    STM8S105_OPTBL  = 0x00487e,
    STM8S105_NOPTBL = 0x00487f,

    STM8S105_FLASH_CR1      = 0x00505a,
    STM8S105_FLASH_CR2      = 0x00505b,
    STM8S105_FLASH_NCR2     = 0x00505c,
    STM8S105_FLASH_FPR      = 0x00505d,
    STM8S105_FLASH_NFPR     = 0x00505e,
    STM8S105_FLASH_IAPSR    = 0x00505f,
    STM8S105_FLASH_PUKR     = 0x005062,
    STM8S105_FLASH_DUKR     = 0x005064,

    STM8S105_CLK_CKDIVR     = 0x0050c6,
    STM8S105_CLK_SWIMCCR    = 0x0050cd,
};


#endif
