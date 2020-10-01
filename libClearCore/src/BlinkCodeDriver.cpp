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
    Status blink code driver implementation

    This class provides a state machine that manages displaying multiple status
    codes via a group / code blink pattern.
**/

#include "BlinkCodeDriver.h"

namespace ClearCore {

void BlinkCodeDriver::Update() {
    switch (m_blinkState) {
        case IDLE:
            // Check if there is an active blink group that needs to be started
            if (!NextCode(0, 0)) {
                break;
            }
            m_timer = m_prestartTicks;
            m_ledOn = false;
            m_blinkState = PRE_START_DELAY;
            break;

        case PRE_START_DELAY:
            if (--m_timer) {
                // Still waiting
                break;
            }
            m_patternWrap = false;
            m_strobeCnt = m_startTicks / m_strobeOnOffTicks;
            m_timer = m_strobeOnOffTicks;
            m_ledOn = true;
            m_blinkState = START_OUTPUT;
            break;

        case START_OUTPUT:
            if (--m_timer) {
                // Strobe value is still active
                break;
            }
            m_ledOn = !m_ledOn;
            if (--m_strobeCnt) {
                m_timer = m_strobeOnOffTicks;
                // More strobe pulses to send
                break;
            }
            m_timer = m_pregroupTicks;
            m_ledOn = false;
            m_blinkState = PRE_GROUP_DELAY;
            break;

        case PRE_GROUP_DELAY:
            if (--m_timer) {
                // Still waiting
                break;
            }
            m_blinkCnt = m_currentGroup + 1;
            m_strobeCnt = m_blinkTicks / m_strobeOnOffTicks;
            m_timer = m_strobeOnOffTicks;
            m_ledOn = true;
            m_blinkState = GROUP_OUTPUT;
            break;

        case GROUP_OUTPUT:
            if (--m_timer) {
                // Strobe value is still active
                break;
            }
            m_ledOn = !m_ledOn;
            if (--m_strobeCnt) {
                m_timer = m_strobeOnOffTicks;
                // More strobe pulses to send
                break;
            }
            m_timer = m_blinkTicks;
            m_ledOn = false;
            m_blinkState = GROUP_DELAY;
            break;

        case GROUP_DELAY:
            if (--m_timer) {
                // Still waiting
                break;
            }
            if (--m_blinkCnt) {
                // Output the next strobe blink for the group
                m_strobeCnt = m_blinkTicks / m_strobeOnOffTicks;
                m_timer = m_strobeOnOffTicks;
                m_ledOn = true;
                m_blinkState = GROUP_OUTPUT;
                break;
            }
            m_timer = m_precodeTicks;
            m_ledOn = false;
            m_blinkState = PRE_CODE_DELAY;
            break;

        case PRE_CODE_DELAY:
            if (--m_timer) {
                // Still waiting
                break;
            }
            m_blinkCnt = m_currentCode + 1;
            m_timer = m_blinkTicks;
            m_ledOn = true;
            m_blinkState = CODE_OUTPUT;
            break;

        case CODE_OUTPUT:
            if (--m_timer) {
                // Code value is still active
                break;
            }
            m_timer = m_blinkTicks;
            m_ledOn = false;
            m_blinkState = CODE_DELAY;
            break;

        case CODE_DELAY:
            if (--m_timer) {
                // Still waiting
                break;
            }
            if (--m_blinkCnt) {
                // Output the next strobe blink for the group
                m_timer = m_blinkTicks;
                m_ledOn = true;
                m_blinkState = CODE_OUTPUT;
                break;
            }
            if (!NextCode(m_currentGroup, m_currentCode + 1)) {
                m_ledOn = false;
                m_blinkState = IDLE;
            }
            else if (m_patternWrap) {
                m_timer = m_prestartTicks;
                m_ledOn = false;
                m_blinkState = PRE_START_DELAY;
            }
            else {
                m_timer = m_pregroupTicks;
                m_ledOn = false;
                m_blinkState = PRE_GROUP_DELAY;
            }
            break;
    }
}

bool BlinkCodeDriver::NextCode(uint8_t group, uint8_t code) {
    // Bounds check the parameters and adjust accordingly
    if (code > 7) {
        code = 0;
        group++;
    }
    if (group >= BLINK_GROUP_MAX) {
        group = 0;
        m_patternWrap = true;
    }
    if (!(m_codes[group] >> code)) {
        // There is not an active code within the current group
        // Search for the next group with an active code
        code = 0;
        uint8_t iGroup;
        // Search from the next index up to the maximum group index
        for (iGroup = group + 1; iGroup < BLINK_GROUP_MAX; iGroup++) {
            if (m_codes[iGroup]) {
                // Found an active group
                break;
            }
        }
        if (iGroup >= BLINK_GROUP_MAX) {
            // Now search the groups from 0 up to the current index
            for (iGroup = 0; iGroup <= group; iGroup++) {
                if (m_codes[iGroup]) {
                    // Found an active group
                    break;
                }
            }
            if (iGroup > group) {
                // No active group codes found
                return false;
            }
            m_patternWrap = true;
        }
        // Set the index of the next active group found
        group = iGroup;
    }
    // There is an active code within group, find out the code value
    // Set the first group/code pair to output
    for (; !(m_codes[group] & (1 << code)); code++) {
        continue;
    }
    m_currentGroup = group;
    m_currentCode = code;
    return true;
}

void BlinkCodeDriver::BlinkCodeClear(uint8_t group, uint8_t code) {
    if (code == 0 || code > 8) {
        return; // Not a valid code.
    }
    if (group == 0 || group > BLINK_GROUP_MAX) {
        return; // Not a valid group.
    }

    m_codes[group - 1] &= ~(1UL << (code - 1));
}

}