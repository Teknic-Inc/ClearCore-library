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
    SerialDriver implementation

    ClearCore Serial Connector Implementation.
**/
#include "SerialDriver.h"
#include <stdio.h>
#include "CcioBoardManager.h"
#include "SerialBase.h"
#include "SysUtils.h"

#define CCIO_DEFAULT_BAUD_RATE    500000

namespace ClearCore {

// LED feedback and option shift register
extern ShiftRegister ShiftReg;
// CCIO-8 management
extern CcioBoardManager &CcioMgr;

SerialDriver::SerialDriver(uint16_t index,
                           ShiftRegister::Masks feedBackLedMask,
                           ShiftRegister::Masks controlMask,
                           ShiftRegister::Masks polarityMask,
                           const PeripheralRoute *ctsMisoInfo,
                           const PeripheralRoute *rtsSsInfo,
                           const PeripheralRoute *rxSckInfo,
                           const PeripheralRoute *txMosiInfo,
                           uint8_t peripheral)
    : SerialBase(ctsMisoInfo, rtsSsInfo, rxSckInfo, txMosiInfo,
                 peripheral),
      m_index(index),
      m_ledMask(feedBackLedMask),
      m_controlMask(controlMask),
      m_polarityMask(polarityMask) {}

void SerialDriver::Initialize(ClearCorePins clearCorePin) {
    m_clearCorePin = clearCorePin;
    PortClose();
    // Default to TTL mode
    Mode(ConnectorModes::TTL);
}

bool SerialDriver::Mode(ConnectorModes newMode) {
    SerialBase::PortModes portMode;

    if (m_mode == newMode) {
        // No change needed
        return true;
    }

    if (m_mode == Connector::CCIO) {
        CcioMgr.LinkClose();
    }

    switch (newMode) {
        case Connector::CCIO:
            SpiClock(SerialDriver::SCK_LOW, SerialDriver::LEAD_CHANGE);
            SpiSsMode(SerialBase::CtrlLineModes::LINE_ON);
            Speed(CCIO_DEFAULT_BAUD_RATE);
        // Fall through
        case Connector::SPI:
            portMode = SerialBase::SPI;
            ShiftReg.ShifterStateClear(m_polarityMask);
            ShiftReg.ShifterStateSet(m_controlMask);
            break;
        case Connector::RS232:
            // Clear the polarity bit to make compatible with RS232 levels
            portMode = SerialBase::UART;
            ShiftReg.ShifterStateSet(m_polarityMask);
            ShiftReg.ShifterStateClear(m_controlMask);
            break;
        case Connector::TTL:
            portMode = SerialBase::UART;
            // Set polarity to make signaling TTL standard levels
            ShiftReg.ShifterStateClear(m_polarityMask);
            ShiftReg.ShifterStateClear(m_controlMask);
            break;
        default:
            // Invalid mode
            return false;
    }

    m_mode = newMode;

    // Set up base class to use this mode
    PortMode(portMode);

    // Delay to allow the port polarity to be written to the shift
    // register and settle for a full character time before sending data
    if (m_portOpen) {
        WaitOneCharTime();
    }
    return true;
}

void SerialDriver::PortOpen() {
    if (!SerialBase::PortIsOpen()) {
        SerialBase::PortOpen();
        // Delay to allow the port polarity to be written to the shift
        // register and settle for a full character time before sending data
        WaitOneCharTime();
        // LED under connector on
        ShiftReg.ShifterStateSet(m_ledMask);

        // Initialize the CCIO manager
        if (m_mode == Connector::CCIO) {
            CcioMgr.CcioDiscover(this);
        }
    }
}

void SerialDriver::PortClose() {
    if (SerialBase::PortIsOpen()) {
        if (m_mode == Connector::CCIO) {
            CcioMgr.LinkClose();
        }
        SerialBase::PortClose();
        // LED under connector off
        ShiftReg.ShifterStateClear(m_ledMask);
    }
}

} // ClearCore namespace
