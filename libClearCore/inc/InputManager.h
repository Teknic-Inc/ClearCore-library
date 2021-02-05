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
    \file InputManager.h
    \brief ClearCore input state access.

    Provides consolidated access to the input state of all of the ClearCore
    connectors.
**/


#ifndef __INPUTMANAGER_H__
#define __INPUTMANAGER_H__

#include "PeripheralRoute.h"
#include "SysConnectors.h"

namespace ClearCore {

typedef void (*voidFuncPtr)(void);

/**
    \brief ClearCore input state access.

    Provides consolidated access to the input state of all of the ClearCore
    connectors.
**/
class InputManager {
    friend class DigitalIn;
    friend class SerialBase;
    friend class TestIO;
public:
    /**
        \enum InterruptTrigger

        \brief The possible input state conditions to trigger an interrupt on.
    **/
    typedef enum {
        NONE = -1,
        LOW = 0,
        HIGH = 1,
        CHANGE = 2,
        FALLING = 3,
        RISING = 4,
    } InterruptTrigger;

#ifndef HIDE_FROM_DOXYGEN
    /**
        Public accessor for singleton instance.
    **/
    static InputManager &Instance();
#endif

    /**
        \brief Clear on read accessor for inputs that have risen (transitioned
        from deasserted to asserted) sometime since the previous invocation of
        this function.
        \param [in] mask (optional) A SysConnectorState whose asserted bits
        indicate which of the ClearCore inputs to check for rising edges.
        If one of the \a bit members of this mask are deasserted, that bit will
        be ignored when checking for rising edges. If no \a mask is provided,
        it's equivalent to passing a SysConnectorState with all bits asserted,
        in which case this function would report rising edges on any of the
        ClearCore inputs.

        \code{.cpp}
        // Utilize the full InputMgr Register to view rising edges.
        uint32_t inputRisenReg = InputMgr.InputsRisen().reg;
        if (inputRisenReg) {
            // The state of one or more inputs has risen since the
            // last time InputsRisen() was called.
        }
        \endcode

        \return StatusRegister whose asserted bits indicate which ClearCore
        inputs have risen since the last poll.
    **/
    SysConnectorState InputsRisen(SysConnectorState mask = UINT32_MAX);

    /**
        \brief Clear on read accessor for inputs that have fallen (transitioned
        from asserted to deasserted) sometime since the previous invocation of
        this function.
        \param [in] mask (optional) A SysConnectorState whose asserted bits
        indicate which of the ClearCore inputs to check for falling edges.
        If one of the \a bit members of this mask are deasserted, that bit will
        be ignored when checking for falling edges. If no \a mask is provided,
        it's equivalent to passing a SysConnectorState with all bits asserted,
        in which case this function would report falling edges on any of the
        ClearCore inputs.

        \code{.cpp}
        // Generate a mask to filter only the falling edges we care to see.
        SysConnectorState mask;
        mask.bit.CLEARCORE_PIN_IO0 = 1;
        mask.bit.CLEARCORE_PIN_IO1 = 1;
        uint32_t inputFallenReg = InputMgr.InputsFallen(mask).reg;
        if (inputFallenReg) {
            // The state of either IO-0 or IO-1 has fallen since the
            // last time InputsFallen() was called.
        }
        \endcode

        \return StatusRegister whose asserted bits indicate which ClearCore
        inputs have fallen since the last poll.
    **/
    SysConnectorState InputsFallen(SysConnectorState mask = UINT32_MAX);

    /**
        \brief Current state of the on-board ClearCore inputs.
        \param [in] mask (optional) A SysConnectorState whose asserted bits
        indicate which of the ClearCore inputs to check for falling edges.
        If one of the \a bit members of this mask are deasserted, that bit will
        be ignored when checking for falling edges. If no \a mask is provided,
        it's equivalent to passing a SysConnectorState with all bits asserted,
        in which case this function would report falling edges on any of the
        ClearCore inputs.

        \code{.cpp}
        // Save the current state of all inputs for future use
        SysConnectorState currState = InputMgr.InputsRT();
        \endcode

        \return SysConnectorState whose asserted bits indicate ClearCore inputs
        that are currently asserted.
    **/
    SysConnectorState InputsRT(SysConnectorState mask = UINT32_MAX);

#ifndef HIDE_FROM_DOXYGEN
    /**
        \brief Enable or disable the interrupt on a digital input connector with
        the supplied external interrupt number.

        \param[in] extInt The external interrupt line number associated with a
        digital input connector that can trigger interrupts.
        \param[in] enable If true, enable the interrupt. If false, disable the
        interrupt.

        \note Only connectors DI-6 through A-12 can trigger interrupts.
    **/
    void InterruptEnable(int8_t extInt, bool enable, bool clearPending = false);
#endif

    /**
        \brief Enable or disable digital interrupts board-wide.

        \code{.cpp}
        // Set up an interrupt callback for 2 connectors
        ConnectorDI7.InterruptHandlerSet(&myCallback, InputManager::RISING);
        ConnectorDI8.InterruptHandlerSet(&myCallback, InputManager::RISING);
        // Enable both interrupts at once
        InputMgr.InterruptsEnabled(true);
        \endcode

        \note Only connectors DI-6 through A-12 can trigger interrupts.
        \note DigitalIn has a connector-specific version of this function as a
        class member.
    **/
    void InterruptsEnabled(bool enable);

    /**
        \brief Current enable state of digital interrupts.

        \code{.cpp}
        if (InputMgr.InterruptsEnabled()) {
            // Input interrupts are globally enabled, do something (like disable
            // them).
        }
        \endcode

        \return true if interrupts are enabled board-wide, false if not.

        \note Only connectors DI-6 through A-12 can trigger interrupts.
    **/
    bool InterruptsEnabled() {
        return m_interruptsEnabled;
    }

#ifndef HIDE_FROM_DOXYGEN
    /**
        \brief Register the interrupt service routine to be triggered when the
        given input state condition is met on the connector with the supplied
        external interrupt number.

        \param[in] extInt The external interrupt line number associated with a
        digital input connector that can trigger interrupts.
        \param[in] callback (optional) The ISR to be called when the interrupt
        is triggered. Default: nullptr.
        \param[in] trigger (optional) The input state condition on which to
        trigger the interrupt. Default: RISING.
        \param[in] enable (optional) Whether this interrupt should be
        immediately enabled. Default: true.
        \return true if the ISR was registered successfully, false otherwise.
    **/
    bool InterruptHandlerSet(int8_t extInt,
                             voidFuncPtr callback = nullptr,
                             InterruptTrigger trigger = RISING,
                             bool enable = true,
                             bool oneTime = false);

    /**
        Initialize the InputManager.
    **/
    void Initialize();

    /**
        Refresh the input register snapshots and see
        which bits have changed since the last update.
    **/
    void UpdateBegin();

    /**
        At the end of the sample time, update Rise/Fall registers.
    **/
    void UpdateEnd();

    /**
        Main external interrupt handler.
    **/
    void EIC_Handler(uint8_t index);
#endif
private:
    // State of the unfiltered input port registers from the DSP.
    volatile uint32_t *m_inputPtrs[CLEARCORE_PORT_MAX];
    uint32_t m_inputsUnfiltered[CLEARCORE_PORT_MAX];
    uint32_t m_inputsUnfilteredChanges[CLEARCORE_PORT_MAX];

    // Filtered input registers
    // Real Time register for FILTERED values
    SysConnectorState m_inputRegRT;
    // Last sample time register for FILTERED values
    SysConnectorState m_inputRegLast;
    // Rising Edge register for FILTERED values
    SysConnectorState m_inputRegRisen;
    // Falling Edge register for FILTERED values
    SysConnectorState m_inputRegFallen;
    // End input registers

    // A mask representing all connectors with registered ISRs.
    uint32_t m_interruptsMask;
    // Are interrupts enabled across the board?
    bool m_interruptsEnabled;
    // Registered interrupt service routines
    voidFuncPtr m_interruptServiceRoutines[EIC_NUMBER_OF_INTERRUPTS];
    // Bitmask indicating which interrupt handlers disable after triggerring
    uint16_t m_oneTimeFlags;

#ifndef HIDE_FROM_DOXYGEN
    /**
        Construct
    **/
    InputManager();

    // Configure the addresses to read the inputs from
    void SetInputRegisters(volatile uint32_t *a,
                           volatile uint32_t *b,
                           volatile uint32_t *c);

    /**
        Translates the \a trigger to the EIC config sense setting.
    **/
    uint32_t EicSense(InterruptTrigger trigger);

#endif // !HIDE_FROM_DOXYGEN
}; // InputManager

} // ClearCore namespace

#endif /* __INPUTMANAGER_H__ */
