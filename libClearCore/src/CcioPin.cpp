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
    Connector class for controlling a CCIO-8 pin

    This manages a CCIO-8 input/output connector on the ClearCore board.
**/

#include "CcioPin.h"
#include <sam.h>
#include "CcioBoardManager.h"
#include "SysUtils.h"

namespace ClearCore {

extern CcioBoardManager &CcioMgr;

CcioPin::CcioPin()
    : Connector(),
      m_dataBit(0),
      m_filterLength(3),
      m_filterTicksLeft(1),
      m_overloadTripCnt(CCIO_OVERLOAD_TRIP_TICKS),
      m_overloadFoldbackCnt(0),
      m_pulseOnTicks(0),
      m_pulseOffTicks(0),
      m_pulseTicksRemaining(0),
      m_pulseStopCount(0),
      m_pulseCounter(0) {}


void CcioPin::Initialize(ClearCorePins ccioPin) {
    m_clearCorePin = ccioPin;
    m_dataBit = 1ULL << (ccioPin - CLEARCORE_PIN_CCIO_BASE);
    m_mode = ConnectorModes::INPUT_DIGITAL;
    m_filterLength = 3;
    m_filterTicksLeft = 1;
    m_overloadTripCnt = CCIO_OVERLOAD_TRIP_TICKS;
    m_overloadFoldbackCnt = 0;
    m_pulseOnTicks = 0;
    m_pulseOffTicks = 0;
    m_pulseTicksRemaining = 0;
    m_pulseStopCount = 0;
    m_pulseCounter = 0;
}

bool CcioPin::Mode(ConnectorModes newMode) {
    // Bail out if we are already in the requested mode
    if (newMode == m_mode) {
        return true;
    }

    switch (newMode) {
        // Set up as output
        case OUTPUT_DIGITAL:
            CcioMgr.m_outputMask |= m_dataBit;
            m_mode = newMode;
            break;
        // Set up as input
        case INPUT_DIGITAL:
            CcioMgr.m_outputMask &= ~m_dataBit;
            CcioMgr.m_pulseActive &= ~m_dataBit;
            m_mode = newMode;
            break;
        // Unsupported mode, don't change anything
        default:
            break;
    }
    return m_mode == newMode;
}

int16_t CcioPin::State() {
    bool state = false;

    switch (m_mode) {
        case OUTPUT_DIGITAL:
            state = CcioMgr.m_currentOutputs & m_dataBit;
            break;
        case INPUT_DIGITAL:
            state = CcioMgr.m_filteredInputs & m_dataBit;
            break;
        default:
            break;
    }

    return state;
}

bool CcioPin::State(int16_t newState) {
    bool success = false;

    switch (m_mode) {
        case OUTPUT_DIGITAL:
            if (newState) {
                CcioMgr.m_currentOutputs |= m_dataBit;
            }
            else {
                CcioMgr.m_currentOutputs &= ~m_dataBit;
            }
            success = true;
            break;
        case INPUT_DIGITAL:
        default:
            break;
    }

    return success;
}

void CcioPin::Filter_ms(uint16_t len) {
    uint32_t samples =
        static_cast<uint32_t>(len) * MS_TO_SAMPLES / CcioMgr.m_ccioRefreshRate;
    if (samples > UINT16_MAX) {
        samples = UINT16_MAX;
    }
    FilterLength(samples);
}

bool CcioPin::InputRisen() {
    return CcioMgr.InputsRisen(m_dataBit);
}

bool CcioPin::InputFallen() {
    return CcioMgr.InputsFallen(m_dataBit);
}

bool CcioPin::IsInHwFault() {
    return (volatile uint64_t &)(CcioMgr.m_ccioOverloaded) & m_dataBit;
}

void CcioPin::OutputPulsesStart(uint32_t onTime, uint32_t offTime,
                                uint16_t pulseCount, bool blockUntilDone) {
    CcioMgr.OutputPulsesStart(m_clearCorePin, onTime, offTime, pulseCount,
                              blockUntilDone);
}

void CcioPin::OutputPulsesStop(bool stopImmediately) {
    CcioMgr.OutputPulsesStop(m_clearCorePin, stopImmediately);
}

} // ClearCore namespace
