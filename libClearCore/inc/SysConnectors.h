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
    \file SysConnectors.h

    Defines tools related to the entire ClearCore system and its connectors
    (constants, enums, unions, structs).
**/


#ifndef __SYSCONNECTORS_H__
#define __SYSCONNECTORS_H__


#ifdef __cplusplus
extern "C" {
#endif

/** The total number of H-Bridge capable connectors on the ClearCore (2). **/
#define HBRIDGE_CON_CNT  (2)
/** The total number of motor connectors on the ClearCore (4). **/
#define MOTOR_CON_CNT (4)

/**
    \enum ClearCorePins

    \brief ClearCore PIN definitions.

    \note These do not strictly map to "pins" on the CPU. They map to the
    internal connector objects that simulate or mediate access to the actual
    processor resources.

**/
typedef enum {
    CLEARCORE_PIN_INVALID = -1, ///< Invalid connector index
    CLEARCORE_PIN_IO0,  ///< [00] IO-0 ClearCore::DigitalInOutAnalogOut Connector
    CLEARCORE_PIN_IO1,  ///< [01] IO-1 ClearCore::DigitalInOut Connector
    CLEARCORE_PIN_IO2,  ///< [02] IO-2 ClearCore::DigitalInOut Connector
    CLEARCORE_PIN_IO3,  ///< [03] IO-3 ClearCore::DigitalInOut Connector
    CLEARCORE_PIN_IO4,  ///< [04] IO-4 ClearCore::DigitalInOutHBridge Connector
    CLEARCORE_PIN_IO5,  ///< [05] IO-5 ClearCore::DigitalInOutHBridge Connector
    CLEARCORE_PIN_DI6,  ///< [06] DI-6 ClearCore::DigitalIn Connector
    CLEARCORE_PIN_DI7,  ///< [07] DI-7 ClearCore::DigitalIn Connector
    CLEARCORE_PIN_DI8,  ///< [08] DI-8 ClearCore::DigitalIn Connector
    CLEARCORE_PIN_A9,   ///< [09] A-9 ClearCore::DigitalInAnalogIn Connector
    CLEARCORE_PIN_A10,  ///< [10] A-10 ClearCore::DigitalInAnalogIn Connector
    CLEARCORE_PIN_A11,  ///< [11] A-11 ClearCore::DigitalInAnalogIn Connector
    CLEARCORE_PIN_A12,  ///< [12] A-12 ClearCore::DigitalInAnalogIn Connector
    CLEARCORE_PIN_LED,  ///< [13] User LED ClearCore::LedDriver Connector
    CLEARCORE_PIN_M0,   ///< [14] M-0 ClearCore::MotorDriver Connector
    CLEARCORE_PIN_M1,   ///< [15] M-1 ClearCore::MotorDriver Connector
    CLEARCORE_PIN_M2,   ///< [16] M-2 ClearCore::MotorDriver Connector
    CLEARCORE_PIN_M3,   ///< [17] M-3 ClearCore::MotorDriver Connector
    CLEARCORE_PIN_COM0, ///< [18] COM-0 ClearCore::SerialDriver Connector
    CLEARCORE_PIN_COM1, ///< [19] COM-1 ClearCore::SerialDriver Connector
    CLEARCORE_PIN_USB,  ///< [20] USB ClearCore::SerialUsb Connector
    CLEARCORE_PIN_MAX,  ///< [21] Count of Connectors on the ClearCore board
    // Motor inputs
    CLEARCORE_PIN_M0_INA,   ///< [22] M-0 ClearCore::MotorDriver InA
    CLEARCORE_PIN_M1_INA,   ///< [23] M-1 ClearCore::MotorDriver InA
    CLEARCORE_PIN_M2_INA,   ///< [24] M-2 ClearCore::MotorDriver InA
    CLEARCORE_PIN_M3_INA,   ///< [25] M-3 ClearCore::MotorDriver InA
    CLEARCORE_PIN_M0_INB,   ///< [26] M-0 ClearCore::MotorDriver InB
    CLEARCORE_PIN_M1_INB,   ///< [27] M-1 ClearCore::MotorDriver InB
    CLEARCORE_PIN_M2_INB,   ///< [28] M-2 ClearCore::MotorDriver InB
    CLEARCORE_PIN_M3_INB,   ///< [29] M-3 ClearCore::MotorDriver InB
    // CCIO-8 Pins
    CLEARCORE_PIN_CCIO_BASE = 64,///< [64] Base index of CCIO-8 connectors
    CLEARCORE_PIN_CCIOA0 = CLEARCORE_PIN_CCIO_BASE, ///< CCIO-8 board 1, connector 0
    CLEARCORE_PIN_CCIOA1,   ///< CCIO-8 board 1, connector 1
    CLEARCORE_PIN_CCIOA2,   ///< CCIO-8 board 1, connector 2
    CLEARCORE_PIN_CCIOA3,   ///< CCIO-8 board 1, connector 3
    CLEARCORE_PIN_CCIOA4,   ///< CCIO-8 board 1, connector 4
    CLEARCORE_PIN_CCIOA5,   ///< CCIO-8 board 1, connector 5
    CLEARCORE_PIN_CCIOA6,   ///< CCIO-8 board 1, connector 6
    CLEARCORE_PIN_CCIOA7,   ///< CCIO-8 board 1, connector 7
    CLEARCORE_PIN_CCIOB0,   ///< CCIO-8 board 2, connector 0
    CLEARCORE_PIN_CCIOB1,   ///< CCIO-8 board 2, connector 1
    CLEARCORE_PIN_CCIOB2,   ///< CCIO-8 board 2, connector 2
    CLEARCORE_PIN_CCIOB3,   ///< CCIO-8 board 2, connector 3
    CLEARCORE_PIN_CCIOB4,   ///< CCIO-8 board 2, connector 4
    CLEARCORE_PIN_CCIOB5,   ///< CCIO-8 board 2, connector 5
    CLEARCORE_PIN_CCIOB6,   ///< CCIO-8 board 2, connector 6
    CLEARCORE_PIN_CCIOB7,   ///< CCIO-8 board 2, connector 7
    CLEARCORE_PIN_CCIOC0,   ///< CCIO-8 board 3, connector 0
    CLEARCORE_PIN_CCIOC1,   ///< CCIO-8 board 3, connector 1
    CLEARCORE_PIN_CCIOC2,   ///< CCIO-8 board 3, connector 2
    CLEARCORE_PIN_CCIOC3,   ///< CCIO-8 board 3, connector 3
    CLEARCORE_PIN_CCIOC4,   ///< CCIO-8 board 3, connector 4
    CLEARCORE_PIN_CCIOC5,   ///< CCIO-8 board 3, connector 5
    CLEARCORE_PIN_CCIOC6,   ///< CCIO-8 board 3, connector 6
    CLEARCORE_PIN_CCIOC7,   ///< CCIO-8 board 3, connector 7
    CLEARCORE_PIN_CCIOD0,   ///< CCIO-8 board 4, connector 0
    CLEARCORE_PIN_CCIOD1,   ///< CCIO-8 board 4, connector 1
    CLEARCORE_PIN_CCIOD2,   ///< CCIO-8 board 4, connector 2
    CLEARCORE_PIN_CCIOD3,   ///< CCIO-8 board 4, connector 3
    CLEARCORE_PIN_CCIOD4,   ///< CCIO-8 board 4, connector 4
    CLEARCORE_PIN_CCIOD5,   ///< CCIO-8 board 4, connector 5
    CLEARCORE_PIN_CCIOD6,   ///< CCIO-8 board 4, connector 6
    CLEARCORE_PIN_CCIOD7,   ///< CCIO-8 board 4, connector 7
    CLEARCORE_PIN_CCIOE0,   ///< CCIO-8 board 5, connector 0
    CLEARCORE_PIN_CCIOE1,   ///< CCIO-8 board 5, connector 1
    CLEARCORE_PIN_CCIOE2,   ///< CCIO-8 board 5, connector 2
    CLEARCORE_PIN_CCIOE3,   ///< CCIO-8 board 5, connector 3
    CLEARCORE_PIN_CCIOE4,   ///< CCIO-8 board 5, connector 4
    CLEARCORE_PIN_CCIOE5,   ///< CCIO-8 board 5, connector 5
    CLEARCORE_PIN_CCIOE6,   ///< CCIO-8 board 5, connector 6
    CLEARCORE_PIN_CCIOE7,   ///< CCIO-8 board 5, connector 7
    CLEARCORE_PIN_CCIOF0,   ///< CCIO-8 board 6, connector 0
    CLEARCORE_PIN_CCIOF1,   ///< CCIO-8 board 6, connector 1
    CLEARCORE_PIN_CCIOF2,   ///< CCIO-8 board 6, connector 2
    CLEARCORE_PIN_CCIOF3,   ///< CCIO-8 board 6, connector 3
    CLEARCORE_PIN_CCIOF4,   ///< CCIO-8 board 6, connector 4
    CLEARCORE_PIN_CCIOF5,   ///< CCIO-8 board 6, connector 5
    CLEARCORE_PIN_CCIOF6,   ///< CCIO-8 board 6, connector 6
    CLEARCORE_PIN_CCIOF7,   ///< CCIO-8 board 6, connector 7
    CLEARCORE_PIN_CCIOG0,   ///< CCIO-8 board 7, connector 0
    CLEARCORE_PIN_CCIOG1,   ///< CCIO-8 board 7, connector 1
    CLEARCORE_PIN_CCIOG2,   ///< CCIO-8 board 7, connector 2
    CLEARCORE_PIN_CCIOG3,   ///< CCIO-8 board 7, connector 3
    CLEARCORE_PIN_CCIOG4,   ///< CCIO-8 board 7, connector 4
    CLEARCORE_PIN_CCIOG5,   ///< CCIO-8 board 7, connector 5
    CLEARCORE_PIN_CCIOG6,   ///< CCIO-8 board 7, connector 6
    CLEARCORE_PIN_CCIOG7,   ///< CCIO-8 board 7, connector 7
    CLEARCORE_PIN_CCIOH0,   ///< CCIO-8 board 8, connector 0
    CLEARCORE_PIN_CCIOH1,   ///< CCIO-8 board 8, connector 1
    CLEARCORE_PIN_CCIOH2,   ///< CCIO-8 board 8, connector 2
    CLEARCORE_PIN_CCIOH3,   ///< CCIO-8 board 8, connector 3
    CLEARCORE_PIN_CCIOH4,   ///< CCIO-8 board 8, connector 4
    CLEARCORE_PIN_CCIOH5,   ///< CCIO-8 board 8, connector 5
    CLEARCORE_PIN_CCIOH6,   ///< CCIO-8 board 8, connector 6
    CLEARCORE_PIN_CCIOH7,   ///< CCIO-8 board 8, connector 7
    CLEARCORE_PIN_CCIO_MAX,
} ClearCorePins;

#ifdef __cplusplus
} // extern "C"
#endif

#ifdef __cplusplus

namespace ClearCore {

/**
    SysConnectorState union allocating 1 bit per connector.

    This union is used to hold a bitmask of state information at a
    system level instead of having to query each connector individually.

    \note Names in the bitfield match the #ClearCorePins enum.
    \note This interface does not include any attached CCIO-8 pins. To read all
    CCIO-8 pins see CcioBoardManager::OutputState().
**/
union SysConnectorState {
    /**
        Constructor to allow initialization lists.
    **/
    SysConnectorState(uint32_t initialBits) : reg(initialBits) {}
    /**
        Default Constructor
    **/
    SysConnectorState() : reg(0) {}

    /**
        Broad access to the whole register
    **/
    uint32_t reg;
    /**
        Field access to the system connector state register
    **/
    struct {
        // Matched to ClearCorePins enum.
        uint32_t CLEARCORE_PIN_IO0 : 1;
        uint32_t CLEARCORE_PIN_IO1 : 1;
        uint32_t CLEARCORE_PIN_IO2 : 1;
        uint32_t CLEARCORE_PIN_IO3 : 1;
        uint32_t CLEARCORE_PIN_IO4 : 1;
        uint32_t CLEARCORE_PIN_IO5 : 1;
        uint32_t CLEARCORE_PIN_DI6 : 1;
        uint32_t CLEARCORE_PIN_DI7 : 1;
        uint32_t CLEARCORE_PIN_DI8 : 1;
        uint32_t CLEARCORE_PIN_A9 : 1;
        uint32_t CLEARCORE_PIN_A10 : 1;
        uint32_t CLEARCORE_PIN_A11 : 1;
        uint32_t CLEARCORE_PIN_A12 : 1;
        uint32_t CLEARCORE_PIN_LED : 1;
        uint32_t CLEARCORE_PIN_M0 : 1;
        uint32_t CLEARCORE_PIN_M1 : 1;
        uint32_t CLEARCORE_PIN_M2 : 1;
        uint32_t CLEARCORE_PIN_M3 : 1;
        uint32_t CLEARCORE_PIN_COM0 : 1;
        uint32_t CLEARCORE_PIN_COM1 : 1;
        uint32_t CLEARCORE_PIN_USB : 1;
    } bit;
};

} // ClearCore namespace
#endif // defined(__cplusplus)

#endif // !__SYSCONNECTORS_H__