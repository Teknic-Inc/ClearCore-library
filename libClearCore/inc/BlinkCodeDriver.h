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
    \file BlinkCodeDriver.h
    \brief Status blink code driver

    This class provides a state machine that manages displaying multiple status
    codes via a group / code blink pattern.
**/


#ifndef BLINKCODEDRIVER_H_
#define BLINKCODEDRIVER_H_

#include <stdint.h>

#ifndef HIDE_FROM_DOXYGEN
namespace ClearCore {


/**
    Driver for outputting blink codes that consist of a count of strobe
    blinks representing a code group, and solid blinks representing a
    value within that group. There can be multiple values active within
    the same group, and the group code will be output before each value.
**/
class BlinkCodeDriver {
    friend class StatusManager;
public:
    typedef enum {
        BLINK_GROUP_IO_OVERLOAD,
        BLINK_GROUP_SUPPLY_ERROR,
        BLINK_GROUP_DEVICE_ERROR,
        BLINK_GROUP_CCIO_OVERLOAD,
        BLINK_GROUP_APPLICATION,
        BLINK_GROUP_MAX,
    } BlinkCodeGroups;

    typedef enum {
        SUPPLY_ERROR_NONE = 0x00,
        SUPPLY_ERROR_VSUPPLY_LOW = 0x01,
        SUPPLY_ERROR_VSUPPLY_HIGH = 0x02,
        SUPPLY_ERROR_5VOB_OVERLOAD = 0x04,
    } SupplyErrorCodes;

    typedef enum {
        DEVICE_ERROR_NONE = 0x00,
        DEVICE_ERROR_HBRIDGE = 0x01,
        DEVICE_ERROR_SD_CARD = 0x02,
        DEVICE_ERROR_ETHERNET = 0x04,
        DEVICE_ERROR_CCIO = 0x08,
        DEVICE_ERROR_XBEE = 0x10,
    } DeviceErrors;

    typedef enum {
        CCIO_OVERLOAD_NONE = 0x00,
        CCIO_OVERLOAD_BOARD0 = 0x01,
        CCIO_OVERLOAD_BOARD1 = 0x02,
        CCIO_OVERLOAD_BOARD2 = 0x04,
        CCIO_OVERLOAD_BOARD3 = 0x08,
        CCIO_OVERLOAD_BOARD4 = 0x10,
        CCIO_OVERLOAD_BOARD5 = 0x20,
        CCIO_OVERLOAD_BOARD6 = 0x40,
        CCIO_OVERLOAD_BOARD7 = 0x80,
    } CcioOverload;

    /**
        Check if there is an active blink pattern.
    **/
    bool CodePresent() {
        return m_blinkState != IDLE;
    }

    /**
        Current state of the blink pattern.
    **/
    volatile const bool &LedState() {
        return m_ledOn;
    }

    /**
        Deactivate the given blink code.
    **/
    void BlinkCodeClear(uint8_t group, uint8_t code);

private:
    enum BlinkState {
        IDLE,
        PRE_START_DELAY,
        START_OUTPUT,
        PRE_GROUP_DELAY,
        GROUP_OUTPUT,
        GROUP_DELAY,
        PRE_CODE_DELAY,
        CODE_OUTPUT,
        CODE_DELAY,
    };
    // What are the codes that need displaying
    uint8_t m_codes[BLINK_GROUP_MAX];
    // Where are we in the code output sequence
    BlinkState m_blinkState;
    uint8_t m_currentCode;
    uint8_t m_currentGroup;
    uint16_t m_timer;
    uint16_t m_strobeCnt;
    uint16_t m_blinkCnt;
    bool m_ledOn;
    bool m_patternWrap;

    // What is the timing of the blink pattern
    uint16_t m_strobeOnOffTicks;
    uint16_t m_blinkTicks;
    uint16_t m_prestartTicks;
    uint16_t m_startTicks;
    uint16_t m_pregroupTicks;
    uint16_t m_precodeTicks;

    BlinkCodeDriver()
        : m_codes{0},
          m_blinkState(IDLE),
          m_currentCode(0),
          m_currentGroup(0),
          m_timer(0),
          m_strobeCnt(0),
          m_blinkCnt(0),
          m_ledOn(false),
          m_patternWrap(false),
          m_strobeOnOffTicks(250),
          m_blinkTicks(2500),
          m_prestartTicks(5000),
          m_startTicks(11500),
          m_pregroupTicks(5000),
          m_precodeTicks(2500) {}

    /**
        Activate the given blink code.
    **/
    void CodeGroupAdd(uint8_t group, uint8_t codes) {
        m_codes[group] |= codes;
    }

    /**
        Update the blink codes.
    **/
    void Update();

    /**
        \brief Current state of the blink pattern.
        \param[in] group The group that the blink pattern belongs to.
        \param[in] code The error code to display.
    **/
    bool NextCode(uint8_t group, uint8_t code);
};

}
#endif /* HIDE_FROM_DOXYGEN */
#endif /* BLINKCODEDRIVER_H_ */