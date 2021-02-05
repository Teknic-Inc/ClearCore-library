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
    DigitalIn implementation

    ClearCore Base Digital Input connector class implementation
**/

#include "DigitalIn.h"
#include <sam.h>
#include "atomic_utils.h"
#include "InputManager.h"
#include "SysUtils.h"

namespace ClearCore {

extern ShiftRegister ShiftReg;
extern InputManager &InputMgr;

#define OVERLOAD_CHECK_HOLDOFF 3

// Stubbed connector base; purposefully set up in an invalid state
Connector::Connector()
    : m_clearCorePin(CLEARCORE_PIN_INVALID),
      m_mode(INVALID_NONE) {}

/**
    Construct and wire in our input bit
**/
DigitalIn::DigitalIn(
    ShiftRegister::Masks ledMask,
    const PeripheralRoute *inputInfo)
    : Connector(),
      m_ledMask(ledMask),
      m_inputPort(inputInfo->gpioPort),
      m_inputDataBit(inputInfo->gpioPin),
      m_inputDataMask(1UL << inputInfo->gpioPin),
      m_extInt(inputInfo->extInt),
      m_interruptAvail(inputInfo->extIntAvail),
      m_changeRegPtr(nullptr),
      m_inRegPtr(nullptr),
      m_inputRegRTPtr(nullptr),
      m_stateFiltered(false),
      m_filterLength(3),
      m_filterTicksLeft(1) {}

/**
    Set connector's internal state and update filtering if required.
**/
void DigitalIn::Refresh() {
    if (*m_changeRegPtr & m_inputDataMask) {
        m_filterTicksLeft = m_filterLength;

        if (!m_filterLength) {
            // If the filter length is zero, set the filtered state
            UpdateFilterState();
        }
    }
    else if (m_filterTicksLeft && !--m_filterTicksLeft) {
        // When we decrement to zero, set the filtered state
        UpdateFilterState();
    }
}

/**
    Initialize a digital input connector
**/
void DigitalIn::Initialize(ClearCorePins clearCorePin) {
    // Clean up any state that Reinitialize may require
    m_mode = INVALID_NONE;
    m_stateFiltered = false;
    m_filterLength = 3;
    m_filterTicksLeft = 1;

    // Enabling peripheral mux for interrupts
    PMUX_SELECTION(m_inputPort, m_inputDataBit, PER_EXTINT);

    // For EIC handling on connectors that share interrupt pins.
    if (m_interruptAvail) {
        PIN_CONFIGURATION(m_inputPort, m_inputDataBit,
                          PORT_PINCFG_PMUXEN | PORT_PINCFG_INEN);
    }
    else {
        // Disable interrupts on all other connectors
        PIN_CONFIGURATION(m_inputPort, m_inputDataBit, PORT_PINCFG_INEN);
    }

    m_changeRegPtr = &InputMgr.m_inputsUnfilteredChanges[m_inputPort];
    m_inRegPtr = &InputMgr.m_inputsUnfiltered[m_inputPort];
    m_inputRegRTPtr = &InputMgr.m_inputRegRT.reg;

    ShiftReg.ShifterState(m_stateFiltered, m_ledMask);

    m_clearCorePin = clearCorePin;
    Mode(INPUT_DIGITAL);
}

int16_t DigitalIn::State() {
    if (m_filterLength == 0) {
        // Pull an unfiltered, real time input value.
        return StateRT();
    }
    return m_stateFiltered;
}

int16_t DigitalIn::StateRT() {
    // Pull an unfiltered, real time input value.
    return !(*InputMgr.m_inputPtrs[m_inputPort] & m_inputDataMask);
}

bool DigitalIn::InputRisen() {
    return InputMgr.InputsRisen(1UL << m_clearCorePin).reg;
}

bool DigitalIn::InputFallen() {
    return InputMgr.InputsFallen(1UL << m_clearCorePin).reg;
}

bool DigitalIn::InterruptHandlerSet(voidFuncPtr callback,
                                    InputManager::InterruptTrigger trigger,
                                    bool enable) {
    if (!m_interruptAvail) {
        return false;
    }

    return InputMgr.InterruptHandlerSet(m_extInt, callback, trigger, enable);
}

void DigitalIn::InterruptEnable(bool enable) {
    InputMgr.InterruptEnable(m_extInt, enable);
}

// Write the current filtered pin status back to the member variables
void DigitalIn::UpdateFilterState() {
    m_stateFiltered = !(*m_inRegPtr & m_inputDataMask);
    ShiftReg.ShifterState(m_stateFiltered, m_ledMask);

    // Update the SysManager Register
    if (m_stateFiltered) {
        atomic_or_fetch(m_inputRegRTPtr, 1UL << m_clearCorePin);
    }
    else {
        atomic_and_fetch(m_inputRegRTPtr, ~(1UL << m_clearCorePin));
    }
}

} // ClearCore namespace
