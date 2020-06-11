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
    \file MotorManager.h
    ClearCore motor-connector manager class
**/
#ifndef __MOTORMANAGER_H__
#define __MOTORMANAGER_H__

#include <stdint.h>
#include "HardwareMapping.h"
#include "MotorDriver.h"

namespace ClearCore {

/**
    \class MotorManager
    \brief ClearCore motor-connector manager.

    This class manages shared settings for the MotorDriver connectors.

    For more detailed information on the ClearCore Motor Control and Motion
    Generation systems, check out the \ref MotorDriverMain and \ref MoveGen
    informational pages.
**/
class MotorManager {
public:
    /**
        Output step rates to be sent to motors.
    **/
    typedef enum {
        /**
            Select the slow speed step input rate (100 kHz, 5uS pulse width)
        **/
        CLOCK_RATE_LOW,
        /**
            Select the medium speed step input rate (500 kHz, 1uS pulse width)
        **/
        CLOCK_RATE_NORMAL,
        /**
            Select the fast speed step input rate (2 MHz, 250nS pulse width)
        **/
        CLOCK_RATE_HIGH
    } MotorClockRates;

    /**
        Indicates a pair of MotorDriver Connectors.
    **/
    typedef enum {
        /**
            MotorDriver Connectors M-0 and M-1.
        **/
        MOTOR_M0M1 = 0,
        /**
            MotorDriver Connectors M-2 and M-3.
        **/
        MOTOR_M2M3 = 1,
        /**
            The total number of pairs of MotorDriver Connectors.
        **/
        NUM_MOTOR_PAIRS = 2,
        /**
            All MotorDriver Connectors.
        **/
        MOTOR_ALL = NUM_MOTOR_PAIRS,
    } MotorPair;

#ifndef HIDE_FROM_DOXYGEN
    /**
        Public accessor for singleton instance.
    **/
    static MotorManager &Instance();

    /**
        Initialize hardware and/or internal state.
    **/
    void Initialize();
#endif

    /**
        \brief Sets the output step rate for the motor step generators.

        Sets the step rate for the MotorDriver connectors as a group.
        They cannot be individually set.

        \note Setting a HIGH clock rate when using a ClearPath motor may cause
        errors. NORMAL clock rate is recommended for ClearPath motors.

        \code{.cpp}
        // Set all MotorDrivers' input clock rate to the high rate
        MotorMgr.MotorInputClocking(MotorManager::CLOCK_RATE_HIGH);
        \endcode

        \param[in] newRate Step rate to be set

        \return Success
    **/
    bool MotorInputClocking(MotorClockRates newRate);

    /**
        \brief Sets the operational mode for the specified MotorDriver
        connectors.

        Sets the mode for the specified MotorDriver connectors in tandem.
        They cannot be individually set.

        \code{.cpp}
        // Set M-2 and M-3's mode to step and direction
        MotorMgr.MotorModeSet(MotorManager::MOTOR_M2M3,
                                Connector::CPM_MODE_STEP_AND_DIR);
        \endcode

        \param[in] motorPair Connectors to be set to specified mode.
        \param[in] newMode Connector modes to be set.
        The valid modes for the MotorDriver connectors are:
        - Connector#CPM_MODE_STEP_AND_DIR
        - Connector#CPM_MODE_A_DIRECT_B_DIRECT
        - Connector#CPM_MODE_A_DIRECT_B_PWM
        - Connector#CPM_MODE_A_PWM_B_PWM.

        \return Success
    **/
    bool MotorModeSet(MotorPair motorPair, Connector::ConnectorModes newMode);

protected:
    uint8_t m_gclkIndex;
    MotorClockRates m_clockRate;
    ClearCorePorts m_stepPorts[NUM_MOTOR_PAIRS];
    uint32_t m_stepDataBits[NUM_MOTOR_PAIRS];
    Connector::ConnectorModes m_motorModes[NUM_MOTOR_PAIRS];

private:
#define CPM_CLOCK_RATE_LOW_HZ \
    (100000 / _CLEARCORE_SAMPLE_RATE_HZ * _CLEARCORE_SAMPLE_RATE_HZ)
#define CPM_CLOCK_RATE_NORMAL_HZ \
    (500000 / _CLEARCORE_SAMPLE_RATE_HZ * _CLEARCORE_SAMPLE_RATE_HZ)
#define CPM_CLOCK_RATE_HIGH_HZ \
    (2000000 / _CLEARCORE_SAMPLE_RATE_HZ * _CLEARCORE_SAMPLE_RATE_HZ)

    bool m_initialized;

    /**
        Construct, wire in the Gclk and the mode control pins
    **/
    MotorManager();

    void PinMuxSet();
};

} // ClearCore namespace

#endif  // __MOTORMANAGER_H__
