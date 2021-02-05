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

/*
    Implements the Input Manager class.

    Provides consolidated access to the input states of all the ClearCore
    connectors.
 */

#include "InputManager.h"
#include <stddef.h>
#include "atomic_utils.h"
#include "SysUtils.h"

namespace ClearCore {

InputManager &InputMgr = InputManager::Instance();

InputManager &InputManager::Instance() {
    static InputManager *instance = new InputManager();
    return *instance;
}

/**
    Constructor
**/
InputManager::InputManager()
    : m_inputPtrs(),
      m_inputsUnfiltered { },
      m_inputsUnfilteredChanges { },
      m_inputRegRT(0),
      m_inputRegRisen(0),
      m_inputRegFallen(0),
      m_interruptsMask(0),
      m_interruptsEnabled(true),
      m_interruptServiceRoutines(),
      m_oneTimeFlags(0) {}

/**
    Initialize the InputManager.
**/
void InputManager::Initialize() {
    SetInputRegisters(NULL, NULL, NULL);
}

void InputManager::SetInputRegisters(volatile uint32_t *a, volatile uint32_t *b,
                                     volatile uint32_t *c) {
    m_inputPtrs[PORTA] = a ? a : &PORT->Group[PORTA].IN.reg;
    m_inputPtrs[PORTB] = b ? b : &PORT->Group[PORTB].IN.reg;
    m_inputPtrs[PORTC] = c ? c : &PORT->Group[PORTC].IN.reg;
}

uint32_t InputManager::EicSense(InterruptTrigger trigger) {
    // Logic on input pins is inverted: LOW <-> HIGH, RISING <-> FALLING
    switch (trigger) {
        case RISING:
            return EIC_CONFIG_SENSE0_FALL;
        case FALLING:
            return EIC_CONFIG_SENSE0_RISE;
        case CHANGE:
            return EIC_CONFIG_SENSE0_BOTH;
        case HIGH:
            return EIC_CONFIG_SENSE0_LOW;
        case LOW:
            return EIC_CONFIG_SENSE0_HIGH;
        default:
            return EIC_CONFIG_SENSE0_NONE;
    }
}

bool InputManager::InterruptHandlerSet(int8_t extInt, voidFuncPtr callback,
                                       InterruptTrigger trigger, bool enable,
                                       bool oneTime) {
    if (extInt < 0 || extInt >= EIC_NUMBER_OF_INTERRUPTS) {
        return false; // Invalid external interrupt number
    }

    EIC->CTRLA.bit.ENABLE = 0;
    SYNCBUSY_WAIT(EIC, EIC_SYNCBUSY_ENABLE);

    // Clear any existing interrupt flag
    EIC->INTFLAG.reg = (1UL << extInt);

    if (callback != nullptr) {
        // Clear the existing interrupt trigger condition
        uint8_t shiftAmt = 4 * (extInt % 8);
        EIC->CONFIG[extInt / 8].reg &= ~(0xf << shiftAmt);

        // Set the interrupt trigger condition
        EIC->CONFIG[extInt / 8].reg |=
            static_cast<uint32_t>(EicSense(trigger) << shiftAmt);
    }
    else {
        enable = false;
    }

    m_interruptServiceRoutines[extInt] = callback;

    if (oneTime) {
        m_oneTimeFlags |= (1UL << extInt);
    }
    else  {
        m_oneTimeFlags &= ~(1UL << extInt);
    }

    // Enable the interrupt if requested.
    InterruptEnable(extInt, enable);

    EIC->CTRLA.bit.ENABLE = 1;
    SYNCBUSY_WAIT(EIC, EIC_SYNCBUSY_ENABLE);

    return true;
}

void InputManager::InterruptEnable(int8_t extInt, bool enable,
                                   bool clearPending) {
    if (extInt < 0 || extInt >= EIC_NUMBER_OF_INTERRUPTS) {
        return; // Invalid external interrupt number
    }

    if (enable) {
        if (clearPending) {
            // Clear any existing interrupt flag
            EIC->INTFLAG.reg = (1UL << extInt);
        }
        atomic_or_fetch(&m_interruptsMask, (1UL << extInt));
        if (m_interruptsEnabled) {
            EIC->INTENSET.reg = (1UL << extInt);
        }
    }
    else {
        atomic_and_fetch(&m_interruptsMask, ~(1UL << extInt));
        if (m_interruptsEnabled) {
            EIC->INTENCLR.reg = (1UL << extInt);
        }
    }
}

void InputManager::InterruptsEnabled(bool enable) {
    m_interruptsEnabled = enable;
    if (enable) {
        EIC->INTENSET.reg = atomic_load_n(&m_interruptsMask);
    }
    else {
        EIC->INTENCLR.reg = atomic_load_n(&m_interruptsMask);
    }
}

void InputManager::EIC_Handler(uint8_t index) {
    if (index < EIC_NUMBER_OF_INTERRUPTS) {
        // If this is a one time interrupt, disable the interrupt.
        if (m_oneTimeFlags & (1UL << index)) {
            atomic_and_fetch(&m_interruptsMask, ~(1UL << index));
            EIC->INTENCLR.reg = (1UL << index);
        }
        // Ack the interrupt early so that we don't miss subsequent events
        EIC->INTFLAG.reg = 1UL << index;
        voidFuncPtr callback = m_interruptServiceRoutines[index];
        if (callback != nullptr) {
            callback();
        }
    }
}

void InputManager::UpdateBegin() {
    for (int8_t iPort = 0; iPort < CLEARCORE_PORT_MAX; iPort++) {
        uint32_t last = m_inputsUnfiltered[iPort];
        m_inputsUnfiltered[iPort] = *m_inputPtrs[iPort];
        m_inputsUnfilteredChanges[iPort] = m_inputsUnfiltered[iPort] ^ last;
    }
}

void InputManager::UpdateEnd() {
    atomic_fetch_or(&m_inputRegRisen.reg,
                    m_inputRegRT.reg & (~m_inputRegLast.reg));
    atomic_fetch_or(&m_inputRegFallen.reg,
                    (~m_inputRegRT.reg) & m_inputRegLast.reg);
    m_inputRegLast.reg = m_inputRegRT.reg;
}

SysConnectorState InputManager::InputsRisen(SysConnectorState mask) {
    SysConnectorState retVal;
    retVal.reg = atomic_fetch_and(&m_inputRegRisen.reg, ~mask.reg) & mask.reg;
    return retVal;
}

SysConnectorState InputManager::InputsFallen(SysConnectorState mask) {
    SysConnectorState retVal;
    retVal.reg = atomic_fetch_and(&m_inputRegFallen.reg, ~mask.reg) & mask.reg;
    return retVal;
}

SysConnectorState InputManager::InputsRT(SysConnectorState mask) {
    SysConnectorState retVal;
    retVal.reg = atomic_load_n(&m_inputRegRT.reg) & mask.reg;
    return retVal;
}

} // ClearCore namespace
