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
    ClearCore CCIO-8 manager class implementation.
**/

#include "CcioBoardManager.h"
#include <stddef.h>
#include "SerialBase.h"
#include "SerialDriver.h"
#include "ShiftRegister.h"
#include "SysConnectors.h"
#include "StatusManager.h"
#include "SysTiming.h"

namespace ClearCore {

extern ShiftRegister ShiftReg;
extern StatusManager &StatusMgr;
extern volatile uint32_t tickCnt;
CcioBoardManager &CcioMgr = CcioBoardManager::Instance();

#define MARKER_BYTE (0xCC)
#define CCIO_REDISCOVER_TIME_TICKS (1000 * MS_TO_SAMPLES)

#if MAX_CCIO_DEVICES != 8
#error "MAX_CCIO_DEVICES has changed. CcioIoBuf setup needs updating."
#endif

/**
    Modifies a specific bit in originalNumber at position to be
    the value specified by finalBitValue.
    \note position = 0 modifies LSB

    \param[in] originalNumber Number to be modified
    \param[in] position Index of the bit to be modified
    \param[in] finalBitValue New value of the bit at the given position
    \return Modified number
**/
inline uint64_t modifyBit(uint64_t originalNumber, uint8_t position,
                          bool finalBitValue) {
    uint64_t mask = 1ULL << position;
    // First clear the bit at position, then OR it with the desired value.
    return (originalNumber & ~mask) |
           (static_cast<uint64_t>(finalBitValue) << position);
}

/**
    Reverses the bytes of the given 32-bit value

    \param[in] value Number to be swapped
    \return Swapped value
**/
inline uint32_t reverseBytes(uint32_t value) {
    uint32_t res = 0;
    __asm("REV %[result], %[input]" : [result] "=r"(res) : [input] "r"(value));
    return res;
}

CcioBoardManager &CcioBoardManager::Instance() {
    static CcioBoardManager *instance = new CcioBoardManager();
    return *instance;
}

#ifndef HIDE_FROM_DOXYGEN
CcioBoardManager::CcioBoardManager()
    : m_writeBuf(),
      m_readBuf(),
      m_discoverState(CCIO_SEARCH),
      m_serPort(NULL),
      m_ccioCnt(0),
      m_ccioRefreshRate(1),
      m_ccioRefreshDelay(0),
      m_throttledOutputs(0),
      m_currentInputs(0),
      m_filteredInputs(0),
      m_currentOutputs(0),
      m_outputMask(0),
      m_lastOutputsSwapped(~0),
      m_lastOutputs(0),
      m_outputsWithThrottling(0),
      m_ccioMask(0),
      m_pulseActive(0),
      m_pulseValue(0),
      m_pulseStopPending(0),
      m_consGlitchCnt(0),
      m_ccioLinkBroken(false),
      m_ccioOverloaded(0),
      m_ccioOverloadAccum(0),
      m_overloadSinceStartupAccum(0),
      m_inputRegRisen(0),
      m_inputRegFallen(0),
      m_faultLed(ShiftRegister::SR_NO_FEEDBACK_MASK),
      m_autoRediscover(true),
      m_lastDiscoverTime(0) {
}
#endif

void CcioBoardManager::Initialize() {
    for (uint8_t i = 0; i < CCIO_PIN_CNT; i++) {
        m_ccioPins[i].Initialize((ClearCorePins)(i + CLEARCORE_PIN_CCIO_BASE));
    }
    CcioDiscover(NULL);

    m_ccioCnt = 0;
    m_ccioMask = 0;
    m_ccioRefreshRate = 1;
    m_ccioRefreshDelay = 0;
    m_throttledOutputs = 0;
    m_currentInputs = 0;
    m_filteredInputs = 0;
    m_currentOutputs = 0;
    m_outputMask = 0;
    m_lastOutputsSwapped = ~0ULL;
    m_lastOutputs = 0;
    m_outputsWithThrottling = 0;
    m_ccioMask = 0;
    m_pulseActive = 0;
    m_pulseValue = 0;
    m_pulseStopPending = 0;
    m_consGlitchCnt = 0;
    m_ccioLinkBroken = false;
    m_ccioOverloaded = 0;
    m_ccioOverloadAccum = 0;
    m_inputRegRisen = 0;
    m_inputRegFallen = 0;
    m_autoRediscover = true;
}

bool CcioBoardManager::PinState(ClearCorePins pinNum) {
    if (pinNum < CLEARCORE_PIN_CCIO_BASE || pinNum >= CLEARCORE_PIN_CCIO_MAX) {
        return false;
    }

    // Reposition the pin reference to map it into a shift amount
    int8_t bitIndex = pinNum - CLEARCORE_PIN_CCIO_BASE;
    return ((m_filteredInputs >> bitIndex) & 1);
}

uint64_t CcioBoardManager::InputsRisen(uint64_t mask) {
    uint64_t retVal;
    // Block the interrupt while performing the clear on read masking
    __disable_irq();
    retVal = m_inputRegRisen & mask;
    m_inputRegRisen &= ~mask;
    __enable_irq();
    return retVal;
}

uint64_t CcioBoardManager::InputsFallen(uint64_t mask) {
    uint64_t retVal;
    // Block the interrupt while performing the clear on read masking
    __disable_irq();
    retVal = m_inputRegFallen & mask;
    m_inputRegFallen &= ~mask;
    __enable_irq();
    return retVal;
}

uint64_t CcioBoardManager::IoOverloadAccum() {
    uint64_t retVal;
    // Block the interrupt while performing the clear on read masking
    __disable_irq();
    retVal = m_ccioOverloadAccum;
    m_ccioOverloadAccum = m_ccioOverloaded;
    __enable_irq();
    return retVal;
}

void CcioBoardManager::Refresh() {
    // Don't refresh until CcioDiscover is called
    if (!m_serPort || !m_ccioCnt || m_ccioLinkBroken) {
        return;
    }

    uint64_t mask = 1;

    // Refresh pulse counts when pulses are active
    if (m_pulseActive) {
        uint64_t pulsesEnded = 0;
        uint64_t pulseRise = 0;
        uint64_t pulseFall = 0;

        for (uint8_t i = 0; i < CCIO_PINS_PER_BOARD * m_ccioCnt; i++) {
            // Pulse out update
            if (m_pulseActive & mask) {
                CcioPin &currentPin = m_ccioPins[i];
                if (!--currentPin.m_pulseTicksRemaining) {
                    if (m_pulseValue & mask) {
                        // Turn off the pulse
                        pulseFall |= mask;
                        currentPin.m_pulseTicksRemaining =
                            currentPin.m_pulseOffTicks;
                        // Increment the counter after a complete cycle
                        if (++currentPin.m_pulseCounter >=
                                currentPin.m_pulseStopCount &&
                                currentPin.m_pulseStopCount) {
                            pulsesEnded |= mask;
                        }
                        // If a stop is pending, handle it now that a cycle has
                        // completed
                        if (m_pulseStopPending & mask) {
                            pulsesEnded |= mask;
                            m_pulseStopPending &= ~mask;
                        }
                    }
                    else {
                        // If a stop is pending, stop any upcoming pulses
                        if (m_pulseStopPending & mask) {
                            pulsesEnded |= mask;
                            m_pulseStopPending &= ~mask;
                        }
                        else {
                            // Turn on the pulse
                            pulseRise |= mask;
                            currentPin.m_pulseTicksRemaining =
                                currentPin.m_pulseOnTicks;
                        }
                    }
                }
            }
            mask <<= 1;
        }

        // Update the pulse info with the bits that changed
        m_pulseActive &= ~pulsesEnded;
        m_pulseValue = (m_pulseValue | pulseRise) & ~pulseFall;
        // currentOutputs are inverted from display
        m_currentOutputs = (m_currentOutputs | pulseRise) & ~pulseFall;
    }

    // Bail out unless the refresh delay has timed out
    if (--m_ccioRefreshDelay) {
        return;
    }
    else {
        // Reset delay
        m_ccioRefreshDelay = m_ccioRefreshRate;
    }

    // Wait for the previous SPI transfer to complete
    m_serPort->SpiAsyncWaitComplete();
    m_serPort->SpiSsMode(SerialBase::CtrlLineModes::LINE_OFF);

    // Save the current inputs and the last inputs
    uint64_t lastInputs = m_currentInputs;

    bool markerError =
        m_readBuf.buf8[MAX_CCIO_DEVICES + m_ccioCnt + 1] != MARKER_BYTE;
    m_readBuf.buf8[MAX_CCIO_DEVICES + m_ccioCnt + 1] = 0;
    // Verify that the data read back matches what we had sent
    if (markerError || m_lastOutputsSwapped ^ m_readBuf.buf64.outputsSwapped) {
        if ((m_consGlitchCnt++ >= MAX_GLITCH_LIM) && (MAX_GLITCH_LIM > 0)) {
            // Announce link broken
            m_ccioLinkBroken = true;
            m_serPort->SpiSsMode(SerialBase::CtrlLineModes::LINE_ON);
            m_ccioOverloaded = 0;
            StatusMgr.BlinkCode(
                BlinkCodeDriver::BLINK_GROUP_DEVICE_ERROR,
                BlinkCodeDriver::DEVICE_ERROR_CCIO);
            ShiftReg.LedInFault(m_faultLed, false);
            ShiftReg.LedPattern(m_faultLed,
                                ShiftRegister::LED_BLINK_CCIO_ONLINE,
                                false);
            // We're offline now, bail out
            return;
        }
    }
    else {
        m_consGlitchCnt = 0;
        m_currentInputs =
            (~m_readBuf.buf64.inputs) >> ((MAX_CCIO_DEVICES - m_ccioCnt)
                                          * CCIO_PINS_PER_BOARD);
    }

    mask = 1;
    uint64_t settledChanges = 0;
    uint64_t changedInputs = lastInputs ^ m_currentInputs;
    uint64_t overloadedOutputSample = m_outputsWithThrottling & ~lastInputs;
    uint64_t overloadedOutputRT = m_ccioOverloaded;
    for (uint8_t i = 0; i < CCIO_PINS_PER_BOARD * m_ccioCnt; i++) {
        CcioPin &currentPin = m_ccioPins[i];
        if (m_throttledOutputs & mask) {
            if (!(--currentPin.m_overloadFoldbackCnt)) {
                // Coming out of foldback, reset the overload
                // delay timer and restore the pin state
                m_throttledOutputs &= ~mask;
                currentPin.m_overloadTripCnt = CCIO_OVERLOAD_TRIP_TICKS;
            }
        }
        // Read filtering update
        else if (overloadedOutputSample & mask) {
            // When the overload counter hits zero, signal the overload
            if (currentPin.m_overloadTripCnt &&
                    !--currentPin.m_overloadTripCnt) {
                m_throttledOutputs |= mask;
                currentPin.m_overloadFoldbackCnt = CCIO_OVERLOAD_FOLDBACK_TICKS;
                overloadedOutputRT |= mask;
            }
        }
        else {
            // Not overloaded, reset the overload delay timer
            currentPin.m_overloadTripCnt = CCIO_OVERLOAD_TRIP_TICKS;
            overloadedOutputRT &= ~mask;
        }

        if (changedInputs & mask) {
            currentPin.m_filterTicksLeft = currentPin.m_filterLength;
            if (!currentPin.m_filterLength) {
                settledChanges |= mask;
            }
        }
        else if (currentPin.m_filterTicksLeft &&
                 !(--currentPin.m_filterTicksLeft)) {
            // When we decrement to zero, set the filtered state
            settledChanges |= mask;
        }
        mask <<= 1;
    }

    // Update the filtered input value with the bits that changed
    lastInputs = m_filteredInputs;
    // If filtering, update the values that settled this sample
    m_filteredInputs = (m_filteredInputs & ~settledChanges) |
                       (m_currentInputs & settledChanges);
    // Find the rise/fall
    m_inputRegRisen |= (m_filteredInputs & ~lastInputs);
    m_inputRegFallen |= (~m_filteredInputs & lastInputs);

    // Output overloaded check if there wasn't a glitch
    if (m_consGlitchCnt == 0) {
        // There is an overload if the output was "asserted" and
        // the input was "deasserted", or the output is throttling
        // due to a previous overload condition.
        IoOverloadRT(overloadedOutputRT & m_ccioMask);
    }

    // Store the last outputs that had been sent for glitch comparison
    m_lastOutputsSwapped = m_writeBuf.buf64.outputsSwapped;
    m_lastOutputs = m_currentOutputs;

    // Put the current outputs in the write buffer
    // Do not assert any bits that are inputs or throttled outputs
    m_outputsWithThrottling =
        m_currentOutputs & ~m_throttledOutputs & m_outputMask;
    // Outputs are written in reverse byte order so the last byte written
    // corresponds to the first CCIO-8 in the chain
    uint64_t outputSwapped =
        ~((static_cast<uint64_t>(reverseBytes(
                                     static_cast<uint32_t>(m_outputsWithThrottling))) << 32) |
          reverseBytes(static_cast<uint32_t>(m_outputsWithThrottling >> 32)));
    m_writeBuf.buf64.outputsSwapped =
        outputSwapped >> ((MAX_CCIO_DEVICES - m_ccioCnt) * CCIO_PINS_PER_BOARD);
    m_writeBuf.buf8[(MAX_CCIO_DEVICES - m_ccioCnt)] = MARKER_BYTE;
    // Start the SPI transfer
    m_serPort->SpiSsMode(SerialBase::CtrlLineModes::LINE_ON);
    m_serPort->SpiTransferDataAsync(
        m_writeBuf.buf8 + (MAX_CCIO_DEVICES - m_ccioCnt),
        m_readBuf.buf8 + (MAX_CCIO_DEVICES - m_ccioCnt) + 1, 2 * m_ccioCnt + 1);
}

void CcioBoardManager::RefreshSlow() {
    if (m_serPort && LinkBroken() && m_autoRediscover &&
            tickCnt - m_lastDiscoverTime > CCIO_REDISCOVER_TIME_TICKS) {
        // Reset the discover state and try to remake the broken link network
        m_discoverState = CCIO_SEARCH;
        CcioDiscover(m_serPort);
    }
}

void CcioBoardManager::IoOverloadRT(uint64_t overloadState) {
    // OR the current overload state into the accum
    m_ccioOverloadAccum |= overloadState;

    // No change, just return
    if (m_ccioOverloaded == overloadState) {
        return;
    }
    m_ccioOverloaded = overloadState;
    ShiftReg.LedInFault(m_faultLed, overloadState);

    // If there are overload bits set that have not previously been flagged
    // update the since startup overload register and the blink codes
    if (overloadState & ~m_overloadSinceStartupAccum) {
        m_overloadSinceStartupAccum |= overloadState;
        // Any bits in the accum have the appropriate blink already running
        // We just need to add the blinks for the new overloads
        uint8_t *charPtr = (uint8_t *)&overloadState;
        for (uint8_t i = 0; i < m_ccioCnt; i++) {
            if (charPtr[i]) {
                StatusMgr.BlinkCode(
                    BlinkCodeDriver::BLINK_GROUP_CCIO_OVERLOAD, 1 << i);
            }
        }
    }
}

void CcioBoardManager::PinState(ClearCorePins pinNum, bool newState) {
    if (pinNum < CLEARCORE_PIN_CCIO_BASE || pinNum >= CLEARCORE_PIN_CCIO_MAX) {
        return;
    }

    // Reposition the pin reference to work correctly with masking
    uint32_t bitNum = static_cast<uint32_t>(pinNum - CLEARCORE_PIN_CCIO_BASE);

    // Toggle the bit here and flush change in Refresh
    m_currentOutputs = modifyBit(m_currentOutputs, bitNum, newState);
}

void CcioBoardManager::OutputPulsesStart(ClearCorePins pinNum, uint32_t onTime,
        uint32_t offTime, uint16_t pulseCount,
        bool blockUntilDone) {
    if (pinNum < CLEARCORE_PIN_CCIO_BASE || pinNum >= CLEARCORE_PIN_CCIO_MAX) {
        return;
    }
    if (onTime == 0 || offTime == 0) {
        return;
    }
    // Reposition the pin reference to work correctly with masking
    pinNum = static_cast<ClearCorePins>(pinNum - CLEARCORE_PIN_CCIO_BASE);
    uint64_t pinMask = 1ULL << pinNum;
    // Do not start output pulses if we are in input mode
    if (!(pinMask & m_outputMask)) {
        return;
    }
    CcioPin &currentPin = m_ccioPins[pinNum];
    currentPin.m_pulseCounter = 0;
    currentPin.m_pulseStopCount = pulseCount;
    currentPin.m_pulseOnTicks = onTime * MS_TO_SAMPLES;
    currentPin.m_pulseOffTicks = offTime * MS_TO_SAMPLES;

    if (!(m_pulseActive & pinMask)) {
        currentPin.m_pulseTicksRemaining = currentPin.m_pulseOnTicks;
        m_pulseActive |= pinMask;
        m_pulseValue |= pinMask;
        // If a previous stop was pending, cancel it
        m_pulseStopPending &= ~pinMask;
        // Turn on the output directly
        m_currentOutputs |= pinMask;
    }

    if (blockUntilDone && pulseCount != 0) {
        while (OutputPulsesActive() & pinMask) {
            continue;
        }
    }
}

void CcioBoardManager::LinkClose() {
    m_discoverState = CCIO_SEARCH;
    ShiftReg.LedPattern(m_faultLed, ShiftRegister::LED_BLINK_CCIO_COMM_ERR,
                        false);
    ShiftReg.LedInFault(m_faultLed, m_ccioOverloaded);
    Initialize();
}

void CcioBoardManager::OutputPulsesStop(ClearCorePins pinNum,
                                        bool stopImmediately) {
    ClearCorePins ccioPinNum;
    if (pinNum < CLEARCORE_PIN_CCIO_BASE || pinNum >= CLEARCORE_PIN_CCIO_MAX) {
        return;
    }
    // Reposition the pin reference to work correctly with masking
    ccioPinNum = static_cast<ClearCorePins>(pinNum - CLEARCORE_PIN_CCIO_BASE);

    uint64_t pinMask = 1ULL << ccioPinNum;
    if (stopImmediately) {
        m_pulseActive &= ~pinMask;
        // Turn off the output directly
        m_currentOutputs &= ~pinMask;
    }
    else {
        m_pulseStopPending |= pinMask;
    }
}

uint8_t CcioBoardManager::CcioDiscover(SerialDriver *comInstance) {
    // Ignore calls to this function if the link has been built and is healthy
    if (m_discoverState == CCIO_FOUND || (LinkBroken() && !m_autoRediscover)) {
        return m_ccioCnt;
    }

    uint8_t numFound = 0;
    uint8_t flushCnt = 0;
    bool sendData = true;
    bool flush0Success = false;

    m_serPort = comInstance;
    if (!m_serPort) {
        m_faultLed = ShiftRegister::Masks::SR_NO_FEEDBACK_MASK;
        m_lastDiscoverTime = tickCnt;
        return 0;
    }

    m_faultLed = m_serPort->m_ledMask;

    m_serPort->SpiSsMode(SerialBase::CtrlLineModes::LINE_ON);
    while (m_discoverState != CCIO_FOUND) {
        // Fail after too many attempts
        if (flushCnt >= MAX_FLUSH_ATTEMPTS) {
            m_ccioLinkBroken = true;
            StatusMgr.BlinkCode(
                BlinkCodeDriver::BLINK_GROUP_DEVICE_ERROR,
                BlinkCodeDriver::DEVICE_ERROR_CCIO);
            ShiftReg.LedPattern(m_faultLed,
                                ShiftRegister::LED_BLINK_CCIO_ONLINE,
                                false);
            m_lastDiscoverTime = tickCnt;
            return 0;
        }

        switch (m_discoverState) {
            case CCIO_SEARCH:
                if (sendData) {
                    // Flush with 1's
                    FillBuffer(m_writeBuf.buf8, 2 * MAX_CCIO_DEVICES, 0xff);
                    m_serPort->SpiTransferData(m_writeBuf.buf8, m_readBuf.buf8,
                                               2 * MAX_CCIO_DEVICES);
                    sendData = false;
                }
                else {
                    // Check if any 1's got through, otherwise resend 1s
                    if (!AllEntriesEqual(m_readBuf.buf8,
                                         2 * MAX_CCIO_DEVICES, 0)) {
                        m_discoverState = CCIO_TEST;
                        flushCnt = 0;
                        flush0Success = false;
                    }
                    flushCnt++;
                    sendData = true;
                }
                break;
            case CCIO_TEST:
                if (sendData) {
                    if (!flush0Success) {
                        // Attempt to flush with 0's
                        FillBuffer(m_writeBuf.buf8, 2 * MAX_CCIO_DEVICES, 0);
                        m_serPort->SpiTransferData(m_writeBuf.buf8,
                                                   m_readBuf.buf8,
                                                   2 * MAX_CCIO_DEVICES);
                    }
                    else {
                        // Flush with a's, send extra byte to check for too many
                        // CCIOs
                        FillBuffer(m_writeBuf.buf8,
                                   2 * MAX_CCIO_DEVICES + 1, 0xaa);
                        m_serPort->SpiTransferData(m_writeBuf.buf8,
                                                   m_readBuf.buf8,
                                                   2 * MAX_CCIO_DEVICES + 1);
                    }
                    sendData = false;
                }
                else {
                    if (!flush0Success) {
                        // If 0's got through, try again with a's; otherwise
                        // resend 0's.
                        if (!AllEntriesEqual(m_readBuf.buf8,
                                             2 * MAX_CCIO_DEVICES, 0xff)) {
                            flush0Success = true;
                        }
                        flushCnt++;
                    }
                    else {
                        uint8_t i;
                        bool foundAA = false;
                        // Count until we see a's
                        for (i = 0; i < 2 * MAX_CCIO_DEVICES && !foundAA; i++) {
                            if (m_readBuf.buf8[i] == 0xaa) {
                                foundAA = true;
                            }
                            else {
                                numFound++;
                            }
                        }
                        if (!foundAA &&
                                m_readBuf.buf8[2 * MAX_CCIO_DEVICES] != 0xaa) {
                            // Error state - too many CCIOs
                            m_ccioCnt = 0;
                            m_ccioMask = 0;
                            m_ccioRefreshRate = RefreshRate();
                            m_ccioLinkBroken = true;
                            StatusMgr.BlinkCode(
                                BlinkCodeDriver::BLINK_GROUP_DEVICE_ERROR,
                                BlinkCodeDriver::DEVICE_ERROR_CCIO);
                            ShiftReg.LedPattern(m_faultLed,
                                                ShiftRegister::LED_BLINK_CCIO_ONLINE,
                                                false);
                            m_lastDiscoverTime = tickCnt;
                            return 0;
                        }
                        // Break from loop
                        m_discoverState = CCIO_FOUND;
                        m_readBuf.Clear();
                    }
                    sendData = true;
                }
                break;
            default:
                break;
        }
    }

    // numFound is the number of input and output regs found
    // so divide by 2 to get CCIO-8 count
    numFound >>= 1;
    m_ccioCnt = numFound;
    m_ccioMask = (1 << (m_ccioCnt * CCIO_PINS_PER_BOARD)) - 1;
    m_ccioRefreshRate = RefreshRate();

    if (numFound != 0) {
        // Store the last outputs that had been sent for glitch comparison
        m_lastOutputsSwapped =
            UINT64_MAX >> ((MAX_CCIO_DEVICES - m_ccioCnt) *
                           CCIO_PINS_PER_BOARD);

        // Clear the outputs so that the CCIOs initialize cleanly
        m_writeBuf.Clear();
        m_writeBuf.buf64.outputsSwapped =
            UINT64_MAX >> ((MAX_CCIO_DEVICES - m_ccioCnt) *
                           CCIO_PINS_PER_BOARD);
        m_writeBuf.buf8[MAX_CCIO_DEVICES] = MARKER_BYTE;

        // Start the SPI transfer
        m_serPort->SpiTransferData(m_writeBuf.buf8 +
                                   (MAX_CCIO_DEVICES - m_ccioCnt),
                                   m_readBuf.buf8 +
                                   (MAX_CCIO_DEVICES - m_ccioCnt) + 1,
                                   2 * m_ccioCnt + 1);
        m_serPort->SpiSsMode(SerialBase::CtrlLineModes::LINE_OFF);
        m_writeBuf.buf8[(MAX_CCIO_DEVICES - m_ccioCnt)] = MARKER_BYTE;
        m_writeBuf.buf8[MAX_CCIO_DEVICES] = 0;
        m_serPort->SpiSsMode(SerialBase::CtrlLineModes::LINE_ON);
        m_serPort->SpiTransferData(m_writeBuf.buf8 +
                                   (MAX_CCIO_DEVICES - m_ccioCnt),
                                   m_readBuf.buf8 +
                                   (MAX_CCIO_DEVICES - m_ccioCnt) + 1,
                                   2 * m_ccioCnt + 1);
        m_serPort->SpiSsMode(SerialBase::CtrlLineModes::LINE_OFF);

        // We are now online and initialized
        m_ccioRefreshDelay = m_ccioRefreshRate;
        m_consGlitchCnt = 0;
        m_ccioOverloaded = 0;
        m_ccioLinkBroken = false;
        ShiftReg.LedInFault(m_faultLed, m_ccioOverloaded);
    }

    ShiftReg.LedPattern(m_faultLed,
                        ShiftRegister::LED_BLINK_CCIO_ONLINE,
                        !m_ccioLinkBroken && !m_ccioOverloaded &&
                        (numFound > 0));

    m_lastDiscoverTime = tickCnt;
    return numFound;
}

void CcioBoardManager::CcioRediscoverEnable(bool enable) {
    m_autoRediscover = enable;
}

CcioPin *CcioBoardManager::PinByIndex(ClearCorePins connectorIndex) {
    if (connectorIndex >= ClearCorePins::CLEARCORE_PIN_CCIO_BASE &&
            connectorIndex < ClearCorePins::CLEARCORE_PIN_CCIO_MAX) {
        return &m_ccioPins[connectorIndex - CLEARCORE_PIN_CCIO_BASE];
    }
    else {
        return NULL;
    }
}

} // ClearCore namespace
