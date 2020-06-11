/*
 * Copyright (c) 2020 Teknic, Inc.
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

/**
    LED shift register chain implementation

    Implementation of ClearCore shift register chain for LED displays and
    connector setups.
**/

#include "ShiftRegister.h"
#include <sam.h>
#include "atomic_utils.h"
#include "HardwareMapping.h"
#include "SysTiming.h"
#include "SysUtils.h"

namespace ClearCore {

/**
    Constructs and adjusts inversions for hardware constraints
**/
ShiftRegister::ShiftRegister() :
    m_fastCounter(FAST_COUNTER_PERIOD, FAST_COUNTER_CC),
    m_breathingCounter(),
    m_fadeCounter(),
    m_patternMasks{UINT32_MAX},
    m_patternOutputs{SR_UNDERGLOW_MASK},
    m_altOutput(0),
    m_initialized(false),
    m_blinkCodeActive(false),
    m_blinkCodeState(false),
    m_useAltOutput(false),
    m_pendingOutput(0),
    m_lastOutput(0) {
    m_shiftInversions.reg = 0xffffffff;
    m_shiftInversions.bit.LED_USB = 0;
    m_shiftInversions.bit.LED_IO_4 = 0;
    m_shiftInversions.bit.LED_IO_5 = 0;
    m_shiftInversions.bit.LED_COM_0 = 0;
    m_shiftInversions.bit.LED_COM_1 = 0;
    m_shiftInversions.bit.UNDERGLOW = 0;
    m_shiftInversions.bit.EN_OUT_0 = 0;
    m_shiftInversions.bit.EN_OUT_1 = 0;
    m_shiftInversions.bit.EN_OUT_2 = 0;
    m_shiftInversions.bit.EN_OUT_3 = 0;
    m_shiftInversions.bit.UART_TTL_0 = 0;
    m_shiftInversions.bit.UART_TTL_1 = 0;
}

/**
    Turn on the shifter and setup the mode for SPI
**/
void ShiftRegister::Initialize() {
    SET_CLOCK_SOURCE(SERCOM6_GCLK_ID_CORE, 5);
    CLOCK_ENABLE(APBDMASK, SERCOM6_);

    // Set up pins for SERCOM6 in SPI master mode and enable it to control them
    PMUX_SELECTION(SR_CLK.gpioPort, SR_CLK.gpioPin, PER_SERCOM);
    PMUX_ENABLE(SR_CLK.gpioPort, SR_CLK.gpioPin);

    PMUX_SELECTION(SR_DATA.gpioPort, SR_DATA.gpioPin, PER_SERCOM);
    PMUX_ENABLE(SR_DATA.gpioPort, SR_DATA.gpioPin);

    PMUX_SELECTION(SR_DATA_RET.gpioPort, SR_DATA_RET.gpioPin, PER_SERCOM);
    PMUX_ENABLE(SR_DATA_RET.gpioPort, SR_DATA_RET.gpioPin);

    // Set up Load/Enable pins as outputs
    DATA_OUTPUT_STATE(SR_ENn.gpioPort, 1UL << SR_ENn.gpioPin, true);
    DATA_OUTPUT_STATE(SR_LOAD.gpioPort, 1UL << SR_LOAD.gpioPin, false);
    DATA_DIRECTION_OUTPUT(SR_ENn.gpioPort, (1UL << SR_ENn.gpioPin));
    DATA_DIRECTION_OUTPUT(SR_LOAD.gpioPort, (1UL << SR_LOAD.gpioPin));

    // A pointer to the SPI register to make things easier.
    SercomSpi *sercomSpi = &SERCOM6->SPI;

    // Disable SERCOM6 to switch its role
    sercomSpi->CTRLA.bit.ENABLE = 0;
    SYNCBUSY_WAIT(sercomSpi, SERCOM_SPI_SYNCBUSY_ENABLE);

    // Sets SERCOM6 to SPI Master mode
    sercomSpi->CTRLA.reg |= SERCOM_SPI_CTRLA_MODE(0x3);
    // Sets PAD[3] to DO, PAD[2] to DI, and sets LSB-first transmission
    sercomSpi->CTRLA.reg |= SERCOM_SPI_CTRLA_DOPO(0x2) |
                            SERCOM_SPI_CTRLA_DIPO(0x2) |
                            SERCOM_SPI_CTRLA_DORD;

    // Enables the data receiver
    sercomSpi->CTRLB.bit.RXEN = 1;

    // Enables 32-bit DATA register transactions
    sercomSpi->CTRLC.reg |= SERCOM_SPI_CTRLC_DATA32B;

    // Sets the baud rate to GCLK1 frequency
    sercomSpi->BAUD.reg = 0;

    // Enables SERCOM6 and wait for core sync
    sercomSpi->CTRLA.bit.ENABLE = 1;
    SYNCBUSY_WAIT(sercomSpi, SERCOM_SPI_SYNCBUSY_ENABLE);

    // Send the initial values to the chain
    sercomSpi->DATA.reg = atomic_load_n(&m_patternOutputs[LED_BLINK_IO_SET])
                          ^ m_shiftInversions.reg;

    // Generate strobe and update
    Send();

    // Enable the chain, clear and set SR_EN_N
    DATA_OUTPUT_STATE(SR_ENn.gpioPort, 1UL << SR_ENn.gpioPin, false);

    // Allow timer tick to update
    m_initialized = true;
}

void ShiftRegister::Update() {
    if (!m_initialized) {
        return;
    }

    // Update Counter Outputs
    m_patternOutputs[LED_BLINK_FAST_STROBE]  = m_fastCounter.Update();
    m_patternOutputs[LED_BLINK_BREATHING]    = m_breathingCounter.Update();
    m_patternOutputs[LED_BLINK_FADE]         = m_fadeCounter.Update();

    Send();
}

void ShiftRegister::Send() {
    // Wait for TX-complete interrupt flag in case we get here too quickly
    while (!(SERCOM6->SPI.INTFLAG.bit.TXC)) {
        continue;
    }
    uint32_t output;

    // Strobe the output with minimum pulse width to display last transfer
    DATA_OUTPUT_STATE(SR_LOAD.gpioPort, 1UL << SR_LOAD.gpioPin, true);
    DATA_OUTPUT_STATE(SR_LOAD.gpioPort, 1UL << SR_LOAD.gpioPin, false);
    while (!(SERCOM6->SPI.INTFLAG.bit.RXC)) {
        continue;
    }
    m_latchedOutput = SERCOM6->SPI.DATA.reg ^ m_shiftInversions.reg;
    m_lastOutput = m_pendingOutput;

    if (m_useAltOutput) {
        output = m_altOutput;
    }
    else {
        // Start the output with the low priority mask
        output = m_patternOutputs[LED_BLINK_IO_SET];
        // Start at 1 to skip the user LEDs
        for (uint32_t i = LED_BLINK_IO_SET + 1; i < LED_BLINK_CODE_MAX; i++) {
            // AND in the inverse of the mask to clear out the lower priority
            // patterns.
            output &= ~m_patternMasks[i];
            // Set the output bits to the output of the pattern output.
            output |= m_patternOutputs[i] & m_patternMasks[i];
        }

        if (m_blinkCodeActive) {
            output &= ~SR_UNDERGLOW_MASK;
            if (m_blinkCodeState) {
                output |= SR_UNDERGLOW_MASK;
            }
        }
    }
    m_pendingOutput = output;

    // Apply inversion
    output ^= m_shiftInversions.reg;

    SERCOM6->SPI.DATA.reg = output;
}

/**
    Turn all of the LEDs on briefly so the user can see that they all work.
**/
void ShiftRegister::DiagnosticLedSweep() {
    m_altOutput = 0;
    m_useAltOutput = true;
    // Illuminate bank 2
    for (uint8_t i = 0; i < LED_BANK_2_LEN; i++) {
        m_altOutput |= LED_BANK_2[i];
        Delay_ms(DELAY_TIME);
    }

    // Illuminate bank 0 and 1 simultaneously
    uint8_t largerBankLen = (LED_BANK_1_LEN > LED_BANK_0_LEN) ? LED_BANK_1_LEN
                            : LED_BANK_0_LEN;
    for (uint8_t i = 0; i < largerBankLen; i++) {
        if (i < LED_BANK_0_LEN) {
            m_altOutput |= LED_BANK_0[i];
        }
        if (i < LED_BANK_1_LEN) {
            m_altOutput |= LED_BANK_1[i];
        }

        Delay_ms(DELAY_TIME);
    }

    Delay_ms(50);

    // Turn them off the same way they were turned on
    for (uint8_t i = 0; i < LED_BANK_2_LEN; i++) {
        m_altOutput &= ~LED_BANK_2[i];
        Delay_ms(DELAY_TIME);
    }

    ShifterStateSet(SR_UNDERGLOW_MASK);

    for (uint8_t i = 0; i < largerBankLen; i++) {
        if (i < LED_BANK_0_LEN) {
            m_altOutput &= ~LED_BANK_0[i];
        }
        if (i < LED_BANK_1_LEN) {
            m_altOutput &= ~LED_BANK_1[i];
        }

        Delay_ms(DELAY_TIME);
    }
    m_useAltOutput = false;
}

} // ClearCore namespace