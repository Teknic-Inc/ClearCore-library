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
    \file ShiftRegister.h
    \brief LED shift register access class

    This class provides control and access to the LED shift register while not
    in a connector context.
**/

#ifndef __SHIFTREGISTER_H__
#define __SHIFTREGISTER_H__

#include <stdint.h>
#include "atomic_utils.h"
#include "BlinkCodeDriver.h"

namespace ClearCore {

#ifndef HIDE_FROM_DOXYGEN
/**
    \class ShiftRegister
    \brief LED control and connector configuration class

    This class manages access to the LED/Configuration shift register so LEDs
    and the shift register may be controlled directly.
**/
class ShiftRegister {
    friend class AdcManager;
    friend class DigitalInAnalogIn;
    friend class DigitalInOutAnalogOut;
    friend class DigitalIn;
    friend class DigitalInOut;
    friend class DigitalInOutHBridge;
    friend class CcioBoardManager;
    friend class LedDriver;
    friend class MotorDriver;
    friend class MotorManager;
    friend class SerialDriver;
    friend class StatusManager;
    friend class SysManager;
    friend class TestIO;

public:

    /**
        \union ShiftChain
        \brief Shift register access type.

        Public Access Type to shift register items. The low level code takes
        care of any inversions. True implies LED or feature enabled.
    **/
    union ShiftChain {
        /**
            Parallel view of the shift register.
        **/
        uint32_t reg;

        /**
            Bit-wise view of the shift register.
        **/
        struct _bit {
            uint32_t A_CTRL_3 : 1;            // 0
            uint32_t A_CTRL_2 : 1;            // 1
            uint32_t LED_IO_5 : 1;            // 2
            uint32_t LED_IO_4 : 1;            // 3
            uint32_t LED_IO_3 : 1;            // 4
            uint32_t LED_IO_2 : 1;            // 5
            uint32_t LED_IO_1 : 1;            // 6
            uint32_t LED_IO_0 : 1;            // 7
            uint32_t EN_OUT_3 : 1;            // 8
            uint32_t EN_OUT_2 : 1;            // 9
            uint32_t EN_OUT_1 : 1;            // 10
            uint32_t EN_OUT_0 : 1;            // 11
            uint32_t UART_TTL_1 : 1;          // 12
            uint32_t UART_TTL_0 : 1;          // 13
            uint32_t UNDERGLOW : 1;           // 14
            uint32_t LED_USB : 1;             // 15
            uint32_t UART_SPI_SEL_1 : 1;      // 16
            uint32_t UART_SPI_SEL_0 : 1;      // 17
            uint32_t LED_COM_0 : 1;           // 18
            uint32_t LED_COM_1 : 1;           // 19
            uint32_t CFG00_AOUT : 1;          // 20
            uint32_t LED_DI_6 : 1;            // 21
            uint32_t LED_DI_7 : 1;            // 22
            uint32_t LED_DI_8 : 1;            // 23
            uint32_t LED_ADI_12 : 1;          // 24
            uint32_t LED_ADI_11 : 1;          // 25
            uint32_t LED_ADI_10 : 1;          // 26
            uint32_t LED_ADI_09 : 1;          // 27
            uint32_t ANAIN_DIGITAL_12 : 1;    // 28
            uint32_t ANAIN_DIGITAL_11 : 1;    // 29
            uint32_t ANAIN_DIGITAL_10 : 1;    // 30
            uint32_t ANAIN_DIGITAL_09 : 1;    // 31
        } bit;

        // Construct
        ShiftChain() {
            reg = 0;
        }

        // Construct
        ShiftChain(uint32_t val) {
            reg = val;
        }
    }; // ShiftChain

    /**
        \enum Masks

        \brief Location of shift register outputs as a bit mask.
    **/
    enum Masks {
        SR_NO_FEEDBACK_MASK = 0,
        SR_A_CTRL_3_MASK = 1U << 0,
        SR_A_CTRL_2_MASK = 1U << 1,
        SR_LED_IO_5_MASK = 1U << 2,
        SR_LED_IO_4_MASK = 1U << 3,
        SR_LED_IO_3_MASK = 1U << 4,
        SR_LED_IO_2_MASK = 1U << 5,
        SR_LED_IO_1_MASK = 1U << 6,
        SR_LED_IO_0_MASK = 1U << 7,
        SR_EN_OUT_3_MASK = 1U << 8,
        SR_EN_OUT_2_MASK = 1U << 9,
        SR_EN_OUT_1_MASK = 1U << 10,
        SR_EN_OUT_0_MASK = 1U << 11,
        SR_UART_TTL_1_MASK = 1U << 12,
        SR_UART_TTL_0_MASK = 1U << 13,
        SR_UNDERGLOW_MASK = 1U << 14,
        SR_LED_USB_MASK = 1U << 15,
        SR_UART_SPI_SEL_1_MASK = 1U << 16,
        SR_UART_SPI_SEL_0_MASK = 1U << 17,
        SR_LED_COM_0_MASK = 1U << 18,
        SR_LED_COM_1_MASK = 1U << 19,
        SR_CFG00_AOUT_MASK = 1U << 20,
        SR_LED_DI_6_MASK = 1U << 21,
        SR_LED_DI_7_MASK = 1U << 22,
        SR_LED_DI_8_MASK = 1U << 23,
        SR_LED_ADI_12_MASK = 1U << 24,
        SR_LED_ADI_11_MASK = 1U << 25,
        SR_LED_ADI_10_MASK = 1U << 26,
        SR_LED_ADI_09_MASK = 1U << 27,
        SR_ANAIN_DIGITAL_12_MASK = 1U << 28,
        SR_ANAIN_DIGITAL_11_MASK = 1U << 29,
        SR_ANAIN_DIGITAL_10_MASK = 1U << 30,
        SR_ANAIN_DIGITAL_09_MASK = 1U << 31
    };

    /**
        \enum LED_BLINK_CODE

        \brief LED blink codes for use with errors or normal operations.
        The enum should be ordered by increasing priority.
    **/
    enum LED_BLINK_CODE {
        // LED Patterns
        LED_BLINK_IO_SET = 0, // lowest priority
        LED_BLINK_FADE,
        LED_BLINK_BREATHING,
        LED_BLINK_FAST_STROBE, // highest priority
        // Max value for bounds checking
        LED_BLINK_CODE_MAX,
        LED_BLINK_PWM = LED_BLINK_FADE,
        // Mapped Error Codes to Patterns
        LED_BLINK_OVERLOAD = LED_BLINK_FAST_STROBE,
        LED_BLINK_CCIO_COMM_ERR = LED_BLINK_FADE,
        LED_BLINK_CCIO_ONLINE = LED_BLINK_BREATHING
    };

    /**
        Construct and prepare shift register for initialization.
    **/
    ShiftRegister();

private:
    /**
        Initializes the shift register and begins the timer tick.
    **/
    void Initialize();

    /**
        \return Returns true if any non-underglow output is overloaded.
    **/
    volatile const uint32_t &OverloadActive() {
        return m_patternMasks[LED_BLINK_OVERLOAD];
    }

    /**
        \return Return True if the shift register is ready for operations.
    **/
    volatile const bool &Ready() {
        return m_initialized;
    }

    /**
        \brief Public accessor to shift register state.

        \return Current shift register state.
    **/
    ShiftChain ShifterState() {
        return atomic_load_n(&m_patternOutputs[LED_BLINK_IO_SET]);
    }

    /**
        \brief Public accessor to shift register state.

        \return Current shift register state.
    **/
    bool ShifterState(Masks bitToGet) {
        return atomic_load_n(&m_patternOutputs[LED_BLINK_IO_SET]) & bitToGet;
    }

    /**
        \brief The last state written to the SPI data register.

        \return The shift register state last written to the SPI data register.
    **/
    volatile const uint32_t &LastOutput() {
        return m_lastOutput;
    }

    /**
        \brief Atomic set/clear of shift register state fields.

        \param[in] setFlds True to set fields, false to clear.
        \param[in] fldsToChange Union of fields in the shift register to be set
        or cleared.
    **/
    void ShifterState(bool setFlds, const ShiftChain &fldsToChange) {
        setFlds ? ShifterStateSet(fldsToChange)
        : ShifterStateClear(fldsToChange);
    }

    /**
        \brief Atomic set/clear of shift register state fields.

        \param[in] setFlds True to set fields, false to clear.
        \param[in] bitsToChange LED bit in the shift register to be set
        or cleared.
    **/
    void ShifterState(bool setFlds, Masks bitsToChange) {
        setFlds ? ShifterStateSet(bitsToChange)
        : ShifterStateClear(bitsToChange);
    }

    /**
        \brief Atomic toggle of shift register state fields.

        \param[in] fldsToToggle Union of fields in the shift register to
                   be toggled.
    **/
    void ShifterStateToggle(ShiftChain fldsToToggle) {
        atomic_xor_fetch(&m_patternOutputs[LED_BLINK_IO_SET], fldsToToggle.reg);
    }

    /**
        \brief Replaces the shift register state fields with value.

        \param[in] value What to set the shift register state to.
    **/
    void ShifterStateReplace(uint32_t value) {
        atomic_exchange_n(&m_patternOutputs[LED_BLINK_IO_SET], value);
    }
    /**
        \brief Set or clear an LED's active fault state display.

        \param[in] ledMask A mask to indicate which LED should have a fault
        state display set or cleared.
        \param[in] state True if the fault state display should be set, false if
        the fault state display should be cleared.
    **/
    void LedInFault(uint32_t ledMask, bool state) {
        if (state) {
            m_patternMasks[LED_BLINK_OVERLOAD] |= ledMask;
        }
        else {
            m_patternMasks[LED_BLINK_OVERLOAD] &= ~ledMask;
        }
    }

    /**
        \brief Set or clear an LED's active PWM state display.

        \param[in] ledMask A mask to indicate which LED should have a PWM state
        display set or cleared.
        \param[in] state True if the PWM state display should be set, false if
        the PWM state display should be cleared.
        \param[in] index Index into the analog value array
    **/
    void LedInPwm(Masks ledMask, bool state, uint8_t index) {
        index &= 0xf;   // guard against index out of bounds
        m_fadeCounter.m_analogMasks[index] = ledMask;
        state ? m_fadeCounter.m_activeMask |= 1 << index
                                              : m_fadeCounter.m_activeMask &= ~(1 << index);
        LedPattern(ledMask, LED_BLINK_FADE, state);
    }

    /**
        \brief Set an LED's PWM state value.

        \param[in] value The duty cycle of the specified LED.
        \param[in] index Index into the analog value array
    **/
    void LedPwmValue(uint8_t index, uint32_t value) {
        index &= 0xf;   // guard against index out of bounds
        m_fadeCounter.m_valuesBuf[index] = value;
    }


    /**
        \brief Activates or deactivates the pattern on an LED.

        \param[in] ledMask LED to affect.
        \param[in] pattern Which pattern to set for the LED.
        \param[in] state If true, set the pattern. If false, clear the pattern.
    **/
    void LedPattern(uint32_t ledMask, LED_BLINK_CODE pattern,
                    bool state) {
        state ? m_patternMasks[pattern] |= ledMask
                                           : m_patternMasks[pattern] &= ~ledMask;
    }

    /**
        Simple counter to mimic a TC. Instead of returning 0 or 1, it returns
        0x00000000 or 0xffffffff. This allows easy ANDing of the results to
        create a mask.

        Update() does all of the magic, performing all logic, and returns the
        high or low output.

    **/
    struct TickCounter {
        uint32_t period;
        uint32_t cc;

    private:
        uint32_t count;

    public:
        TickCounter()
            : period(5000),
              cc(2500),
              count(0) {}

        TickCounter(uint32_t period, uint32_t cc)
            : period(period),
              cc(cc),
              count(0) {}

        const uint32_t returnTable[2] = {0x00000000, 0xffffffff};

        uint32_t Update() {
            if (!count--) {
                count = period;
            }
            return returnTable[count < cc];
        }
    };

    /**
        Counter that fades in. Works by modifying the duty cycle of a PWM
        signal.

        Period specifies how long the fade will last.
    **/
    class AnalogLedDriver {
    public:
        uint16_t m_activeMask;
        uint32_t m_lastOutput;
        uint8_t m_count;
        uint8_t m_values[16];
        uint8_t m_valuesBuf[16];
        Masks m_analogMasks[16];

        AnalogLedDriver()
            : m_activeMask(0),
              m_lastOutput(0),
              m_count(UINT8_MAX - 1),
              m_values{0},
              m_valuesBuf{0},
              m_analogMasks{SR_NO_FEEDBACK_MASK} {}

        uint32_t Update() {
            uint32_t retVal = m_lastOutput;
            if (!m_activeMask) {
                m_count = UINT8_MAX - 1;
                return 0;
            }

            if (++m_count > UINT8_MAX >> 2) {
                retVal = 0;
                m_count = 0;
                for (uint8_t i = 0; i < 16; i++) {
                    if (m_activeMask & (1 << i) && m_valuesBuf[i]) {
                        m_values[i] = m_valuesBuf[i];
                        retVal |= m_analogMasks[i];
                    }
                }
            }
            else {
                uint8_t compare = m_count << 2;
                for (uint8_t i = 0; i < 16; i++) {
                    if ((retVal & m_analogMasks[i]) &&
                            (m_values[i] < compare)) {
                        retVal &= ~m_analogMasks[i];
                    }
                }
            }
            return m_lastOutput = retVal;
        }
    };

    /**
        Counter that fades in and out. Works by modifying the duty cycle of a
        PWM signal.

        Periods specify how long the fades will last in their respective
        directions.
    **/
    struct FadeInOutCounter {
        uint8_t m_maxValue;
        uint8_t m_minValue;

    private:
        uint8_t m_count;
        uint8_t m_compare;
        bool fadingIn;
        const uint32_t returnTable[2] = {0x00000000, 0xffffffff};

    public:
        FadeInOutCounter(uint8_t maxValue = UINT8_MAX >> 1,
                         uint8_t minValue = 0x08)
            : m_maxValue(maxValue),
              m_minValue(minValue),
              m_count(0),
              m_compare(0),
              fadingIn(true) {}

        uint32_t Update() {
            if (++m_count >= UINT8_MAX >> 2) {
                m_count = 0;
                if (fadingIn) {
                    if (++m_compare >= m_maxValue) {
                        fadingIn = false;
                    }
                }
                else {
                    if (--m_compare <= m_minValue) {
                        fadingIn = true;
                    }
                }
            }
            return returnTable[m_count << 2 < m_compare];
        }
    };

    // the "close" LEDs
    static const uint8_t LED_BANK_0_LEN = 6;
    const Masks LED_BANK_0[LED_BANK_0_LEN] = {SR_LED_IO_0_MASK,
                                              SR_LED_IO_1_MASK, SR_LED_IO_2_MASK, SR_LED_IO_3_MASK, SR_LED_IO_4_MASK,
                                              SR_LED_IO_5_MASK
                                             };

    // the "far" LEDs
    static const uint8_t LED_BANK_1_LEN = 7;
    const Masks LED_BANK_1[LED_BANK_1_LEN] = {SR_LED_ADI_12_MASK,
                                              SR_LED_ADI_11_MASK, SR_LED_ADI_10_MASK, SR_LED_ADI_09_MASK,
                                              SR_LED_DI_8_MASK, SR_LED_DI_7_MASK, SR_LED_DI_6_MASK
                                             };

    // the "misc" LEDs
    static const uint8_t LED_BANK_2_LEN = 4;
    const Masks LED_BANK_2[LED_BANK_2_LEN] = {SR_UNDERGLOW_MASK,
                                              SR_LED_USB_MASK, SR_LED_COM_0_MASK, SR_LED_COM_1_MASK
                                             };

    static const uint16_t DELAY_TIME = 25; // milliseconds

    // A mask that prevents sketches from changing Shift Register values that
    // aren't LEDs.
    const uint32_t SAFE_LED_MASK = SR_LED_IO_0_MASK | SR_LED_IO_1_MASK |
                                   SR_LED_IO_2_MASK | SR_LED_IO_3_MASK | SR_LED_IO_4_MASK |
                                   SR_LED_IO_5_MASK | SR_LED_DI_6_MASK | SR_LED_DI_7_MASK |
                                   SR_LED_DI_8_MASK | SR_LED_ADI_09_MASK | SR_LED_ADI_10_MASK |
                                   SR_LED_ADI_11_MASK | SR_LED_ADI_12_MASK | SR_LED_USB_MASK;

    /**
        The below constants/data members are grouped for ease of use - the
        constants directly affect the associated counters and their physical
        output.
    **/
    const uint32_t FAST_COUNTER_PERIOD = 500;
    const uint32_t FAST_COUNTER_CC = 200;
    TickCounter m_fastCounter;
    FadeInOutCounter m_breathingCounter;
    AnalogLedDriver m_fadeCounter;
    /** -------------------- END OF GROUPED SECTION-----------------------**/

    // Inversion mask of actual shift register state
    ShiftChain m_shiftInversions;

    uint32_t m_patternMasks[LED_BLINK_CODE_MAX];
    uint32_t m_patternOutputs[LED_BLINK_CODE_MAX];
    uint32_t m_altOutput;

    // Set after initialization
    bool m_initialized;
    bool m_blinkCodeActive;
    bool m_blinkCodeState;
    bool m_useAltOutput;

    // The values about to be written to the SPI data register.
    uint32_t m_pendingOutput;
    // The last values written to the SPI data register.
    uint32_t m_lastOutput;
    // The last values read from the SPI data register.
    uint32_t m_latchedOutput;

    /**
        Update the shift chain and strobe.
    **/
    void Send();

    /**
        Update from timer tick.
    **/
    void Update();

    /**
        \brief Atomic set of shift register state fields.

        \param[in] fldsToSet Union of fields in the shift register to be set.
    **/
    void ShifterStateSet(ShiftChain fldsToSet) {
        atomic_or_fetch(&m_patternOutputs[LED_BLINK_IO_SET], fldsToSet.reg);
    }

    /**
        \brief Atomic set of shift register state fields.

        \param[in] bitsToSet LED bit in the shift register to be set.
    **/
    void ShifterStateSet(Masks bitsToSet) {
        atomic_or_fetch(&m_patternOutputs[LED_BLINK_IO_SET], bitsToSet);
    }

    /**
        \brief Atomic clear of shift register state fields.

        \param[in] fldsToClr Union of fields in the shift register to be
    cleared.
    **/
    void ShifterStateClear(ShiftChain fldsToClr) {
        atomic_and_fetch(&m_patternOutputs[LED_BLINK_IO_SET], ~fldsToClr.reg);
    }

    /**
        \brief Atomic clear of shift register state fields.

        \param[in] bitsToClr LED bit in the shift register to be cleared.
    **/
    void ShifterStateClear(Masks bitsToClr) {
        atomic_and_fetch(&m_patternOutputs[LED_BLINK_IO_SET], ~bitsToClr);
    }

    /**
        \brief Turn all of the ClearCore's LEDs on. The user should be able to
        easily tell if there are any that don't work anymore.

        This function will block until the sequence is complete. It takes about
        a second to complete, and will turn the LEDs back off, except for the
        underglow.
    **/
    void DiagnosticLedSweep();

    void BlinkCode(bool blinkCodeActive, bool blinkCodeState) {
        m_blinkCodeActive = blinkCodeActive;
        m_blinkCodeState = blinkCodeState;
    }

}; // ShiftRegister
#endif

} // ClearCore namespace

#endif // __SHIFTREGISTER_H__