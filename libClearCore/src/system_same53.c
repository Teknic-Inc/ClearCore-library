/**
 * \file
 *
 * \brief Low-level initialization functions called upon chip startup.
 *
 * Copyright (c) 2017 Atmel Corporation,
 *                    a wholly owned subsidiary of Microchip Technology Inc.
 *
 * \asf_license_start
 *
 * \page License
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * \asf_license_stop
 *
 */

#include <same53.h>
#include "SysUtils.h"

/**
 * Initial system clock frequency. The System RC Oscillator (RCSYS) provides
 *  the source for the main clock at chip startup.
 */
// Final CPU speed & DPLL1 frequency
#define __CLEARCORE_CLOCK_HZ    (120000000)             // 120 MHz
#define __SYSTEM_CLOCK          (__CLEARCORE_CLOCK_HZ)
// Oscillator output into XOSC1
#define __CLEARCORE_OSC_HZ      (25000000)              // 25 MHz
// GCLK0 FREQ
#define __CLEARCORE_GCLK0_HZ    __CLEARCORE_CLOCK_HZ
// GCLK1 FREQ
#define __CLEARCORE_GCLK1_HZ    (500000)                // 500 kHz
// GCLK4 FREQ
#define __CLEARCORE_GCLK4_HZ    (48000000)              // 48 MHz
// GCLK5 FREQ
#define __CLEARCORE_GCLK5_HZ    (1000000)               // 1 MHz
// GCLK6 FREQ - set for 500Hz PWM with / 16; HLFB / 31.25Hz max period
#define __CLEARCORE_GCLK6_HZ    (128000*16)             // 2.048 MHz
// GCLK7 FREQ
#define __CLEARCORE_GCLK7_HZ    (10000000)              // 10 MHz
// DPLL0 FREQ
#define __CLEARCORE_DPLL0_HZ    (96000000)              // 96 MHz
// DPLL1 FREQ
#define __CLEARCORE_DPLL1_HZ    (120000000)             // 120 MHz

/*!< System Clock Frequency (Core Clock)*/
uint32_t SystemCoreClock = __SYSTEM_CLOCK;

/**
 * Initialize the system
 *
 * @brief  Setup the microcontroller system.
 *         Initialize the system and update the SystemCoreClock variable.
 */
void SystemInit(void) {
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Start and setup the various oscillators
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    // Start the external 10MHz MEMS oscillator
    OSCCTRL->XOSCCTRL[1].reg =
        OSCCTRL_XOSCCTRL_IMULT(4) |
        OSCCTRL_XOSCCTRL_IPTAT(3) |
        OSCCTRL_XOSCCTRL_ENABLE;
    // Wait for clock to run
    while (!OSCCTRL->STATUS.bit.XOSCRDY1) {
        continue;
    }
    // Create 1MHz clock on GCLK5 to act as source for DPLL0/1 and SERCOM6
    GCLK->GENCTRL[5].reg = GCLK_GENCTRL_SRC(GCLK_GENCTRL_SRC_XOSC1_Val) |
                           GCLK_GENCTRL_GENEN |
                           GCLK_GENCTRL_DIV(__CLEARCORE_OSC_HZ /
                                            __CLEARCORE_GCLK5_HZ) |
                           GCLK_GENCTRL_IDC;
    SYNCBUSY_WAIT(GCLK, GCLK_SYNCBUSY_GENCTRL5);

    // Make good 120MHz CPU clock using DPLL1 multiplying GCLK5 up
    SET_CLOCK_SOURCE(OSCCTRL_GCLK_ID_FDPLL1, 5);
    // Set the integer part of the frequency multiplier (loop divider ratio)
    OSCCTRL->Dpll[1].DPLLRATIO.reg =
    OSCCTRL_DPLLRATIO_LDR(__CLEARCORE_DPLL1_HZ / __CLEARCORE_GCLK5_HZ - 1);  
    // Set GCLK as the DPLL clock reference, and set Wake Up Fast
    OSCCTRL->Dpll[1].DPLLCTRLB.reg = OSCCTRL_DPLLCTRLB_REFCLK_GCLK |
                                     OSCCTRL_DPLLCTRLB_WUF;
        
    // Set the DPLL (digital phase-locked loop) to run in standby and sleep mode
    // If ONDEMAND is not set, the signal will be generated constantly
    // Finally, enable the DPLL
    OSCCTRL->Dpll[1].DPLLCTRLA.reg = OSCCTRL_DPLLCTRLA_RUNSTDBY |
                                     OSCCTRL_DPLLCTRLA_ENABLE;

    while (OSCCTRL->STATUS.bit.DPLL1LCKR) {
        continue;
    }
    // Route DPLL1 @ 120MHz to CPU Clock before killing off 48MHz clock we
    // started with.
    GCLK->GENCTRL[0].reg = GCLK_GENCTRL_SRC(GCLK_GENCTRL_SRC_DPLL1_Val) |
                           GCLK_GENCTRL_GENEN |
                           GCLK_GENCTRL_DIV(0);
    SYNCBUSY_WAIT(GCLK, GCLK_SYNCBUSY_GENCTRL0);
    // Clocks running and locked, switch core clock to 120MHz
    MCLK->CPUDIV.reg = MCLK_CPUDIV_DIV_DIV1;

    // Use 96MHz clock for USB with / 2 on GCLK4 for 48MHz
    // using GCLK5 as reference.
    SET_CLOCK_SOURCE(OSCCTRL_GCLK_ID_FDPLL0, 5);
    // set the integer part of the frequency multiplier (loop divider ratio)
    OSCCTRL->Dpll[0].DPLLRATIO.reg =
        OSCCTRL_DPLLRATIO_LDR(__CLEARCORE_DPLL0_HZ / __CLEARCORE_GCLK5_HZ - 1);
        
    // Set the lock timeout value to Default (none, automatic lock)
    // Set the dedicated GCLK reference
    // Set Wake Up Fast
    OSCCTRL->Dpll[0].DPLLCTRLB.reg = OSCCTRL_DPLLCTRLB_LTIME_DEFAULT |
                                     OSCCTRL_DPLLCTRLB_REFCLK_GCLK |
                                     OSCCTRL_DPLLCTRLB_WUF;
    // enable the DPLL
    OSCCTRL->Dpll[0].DPLLCTRLA.reg = OSCCTRL_DPLLCTRLA_ENABLE;

    // Disable DFLL48M since we are going to use DPLL to generate 48MHz.
    GCLK->PCHCTRL[OSCCTRL_GCLK_ID_DFLL48].bit.CHEN = 0;
    while (GCLK->PCHCTRL[OSCCTRL_GCLK_ID_DFLL48].bit.CHEN) {
        continue;
    }

    OSCCTRL->DFLLCTRLA.reg = 0;
    // Setup GCLK4 to output 48 MHz for USB
    GCLK->GENCTRL[4].reg = GCLK_GENCTRL_GENEN |
                           GCLK_GENCTRL_DIV(__CLEARCORE_DPLL0_HZ /
                                            __CLEARCORE_GCLK4_HZ) |
                           GCLK_GENCTRL_SRC_DPLL0;
    // Wait for clock domain sync
    SYNCBUSY_WAIT(GCLK, GCLK_SYNCBUSY_GENCTRL4);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Setup clock sources from oscillators or other sources
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    // Create 500kHz clock from GCLK1 to act as source for S&D mask
    GCLK->GENCTRL[1].reg = GCLK_GENCTRL_SRC(GCLK_GENCTRL_SRC_XOSC1_Val) |
                           GCLK_GENCTRL_GENEN |
                           GCLK_GENCTRL_DIV(__CLEARCORE_OSC_HZ /
                                            __CLEARCORE_GCLK1_HZ) |
                           GCLK_GENCTRL_IDC |
                           GCLK_GENCTRL_OE;

    // Make sure PORT module is powered up and clocked
    // Should be on already: CLOCK_ENABLE(APBBMASK, PORT_);
    // Make sure SERCOMS are powered up and clocked
    CLOCK_ENABLE(APBAMASK, SERCOM0_);
    CLOCK_ENABLE(APBBMASK, TC3_); // HLFB(2)
    CLOCK_ENABLE(APBAMASK, EIC_);

    CLOCK_ENABLE(APBBMASK, EVSYS_);
    CLOCK_ENABLE(APBBMASK, SERCOM2_);          // XBee

    CLOCK_ENABLE(APBCMASK, TC4_); // HLFB(0)

    CLOCK_ENABLE(AHBMASK, GMAC_);
    CLOCK_ENABLE(APBCMASK, GMAC_); // Ethernet

    CLOCK_ENABLE(APBDMASK, SERCOM4_);          // SD
    CLOCK_ENABLE(APBDMASK, SERCOM7_);
    CLOCK_ENABLE(APBDMASK, ADC1_);
    CLOCK_ENABLE(APBCMASK, TC5_); // HLFB(1)
    CLOCK_ENABLE(APBAMASK, TC0_); // HLFB(3)

    CLOCK_ENABLE(APBDMASK, TC6_); // HBridge PWM output

    // Enable the cache controller
    CMCC->CTRL.reg = CMCC_CTRL_CEN;
    // Enable the FPU
    SCB->CPACR = 0xFU << 20;

    // set up GCLK6 for OUT TCx and HLFB TCx
    GCLK->GENCTRL[6].reg = GCLK_GENCTRL_GENEN |
                           GCLK_GENCTRL_SRC(GCLK_GENCTRL_SRC_DPLL1_Val) |
                           GCLK_GENCTRL_DIV(__CLEARCORE_DPLL1_HZ /
                                            __CLEARCORE_GCLK6_HZ);
    SYNCBUSY_WAIT(GCLK, GCLK_SYNCBUSY_GENCTRL6);

    // set up GCLK7 for SPI sercom clocking
    GCLK->GENCTRL[7].reg = GCLK_GENCTRL_GENEN |
                           GCLK_GENCTRL_SRC(GCLK_GENCTRL_SRC_DPLL1_Val) |
                           GCLK_GENCTRL_DIV(__CLEARCORE_DPLL1_HZ /
                                            __CLEARCORE_GCLK7_HZ);

    // CPU Clock @ 120MHz on GCLK(0), GCLK(6)=2.048MHz
    SET_CLOCK_SOURCE(EIC_GCLK_ID, 6);
    // HLFB timers
    SET_CLOCK_SOURCE(TC0_GCLK_ID, 6);
    SET_CLOCK_SOURCE(TC4_GCLK_ID, 6);
    SET_CLOCK_SOURCE(TC6_GCLK_ID, 6);
    
    // NOTE: TC7 and TC6 share same clock source
    // SET_CLOCK_SOURCE(TC7_GCLK_ID, 6);

    // ZL: Is this still needed?
    while (GCLK->SYNCBUSY.reg) {
        continue;
    }
    return;
}

/**
 * Update SystemCoreClock variable
 *
 * @brief  Updates the SystemCoreClock with current core Clock
 *         retrieved from cpu registers.
 */
void SystemCoreClockUpdate(void) {
    // Not implemented
    return;
}

/**
 * Update GClk frequency
 *
 * @brief  Updates the divisor on the specified GClk to
 *         generate the requested frequency
 */
void GClkFreqUpdate(uint8_t gclkIndex, uint32_t freqReq) {
    // This adjustment is only supported for GClks that use XOSC1 as the src
    if (GCLK->GENCTRL[gclkIndex].bit.SRC != GCLK_GENCTRL_SRC_XOSC1_Val) {
        return;
    }

    // Configure the clock divisor for the requested frequency
    GCLK->GENCTRL[gclkIndex].bit.DIV = __CLEARCORE_OSC_HZ / freqReq;
    while (GCLK->SYNCBUSY.vec.GENCTRL & (1 << gclkIndex)) {
        continue;
    }
}
