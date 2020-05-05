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
    Implementation of ClearCore step mode connector
**/

#include "MotorManager.h"
#include <sam.h>
#include "AdcManager.h"
#include "MotorDriver.h"
#include "ShiftRegister.h"
#include "SysConnectors.h"
#include "SysUtils.h"

namespace ClearCore {
#define MAIN_INTERRUPT_GCLK_ID    1

extern MotorDriver *const MotorConnectors[MOTOR_CON_CNT];
extern ShiftRegister ShiftReg;

MotorManager &MotorMgr = MotorManager::Instance();

MotorManager &MotorManager::Instance() {
    static MotorManager *instance = new MotorManager();
    return *instance;
}

/**
    Construct and wire in our output pins
**/
MotorManager::MotorManager()
    : m_gclkIndex(MAIN_INTERRUPT_GCLK_ID),
      m_clockRate(CLOCK_RATE_NORMAL),
      m_initialized(false) {
    m_stepPorts[MOTOR_M0M1] =  Mtr_CLK_01.gpioPort;
    m_stepPorts[MOTOR_M2M3] = Mtr_CLK_23.gpioPort;
    m_stepDataBits[MOTOR_M0M1] = Mtr_CLK_01.gpioPin;
    m_stepDataBits[MOTOR_M2M3] = Mtr_CLK_23.gpioPin;
    m_motorModes[MOTOR_M0M1] = Connector::CPM_MODE_A_DIRECT_B_DIRECT;
    m_motorModes[MOTOR_M2M3] = Connector::CPM_MODE_A_DIRECT_B_DIRECT;
}

/**
    Set the motor pulse rate.

    Returns true if successfully set.
**/
bool MotorManager::MotorInputClocking(MotorClockRates newRate) {
    if (m_clockRate == newRate && m_initialized) {
        // Same rate as before, nothing to change
        return false;
    }

    uint32_t clkReq;
    bool modeValid = true;

    switch (newRate) {
        case CLOCK_RATE_LOW:
            clkReq = CPM_CLOCK_RATE_LOW_HZ;
            break;
        case CLOCK_RATE_NORMAL:
            clkReq = CPM_CLOCK_RATE_NORMAL_HZ;
            break;
        case CLOCK_RATE_HIGH:
            clkReq = CPM_CLOCK_RATE_HIGH_HZ;
            break;
        default:
            modeValid = false;
            break;
    }

    if (!modeValid) {
        return false;
    }

    // Mode change successful; update the step rate.
    m_clockRate = newRate;

    // Configure TCC0 for the step step carrier signal
    TCC0->CTRLA.bit.ENABLE = 0; // Disable TCC0
    TCC1->CTRLA.bit.ENABLE = 0; // Disable TCC1

    SYNCBUSY_WAIT(TCC0, TCC_SYNCBUSY_ENABLE);
    SYNCBUSY_WAIT(TCC1, TCC_SYNCBUSY_ENABLE);

    GClkFreqUpdate(m_gclkIndex, clkReq);
    int32_t newPeriod = clkReq / _CLEARCORE_SAMPLE_RATE_HZ;

    TCC0->COUNT.reg = 0;
    TCC1->COUNT.reg = 0;

    // Clear out any pending command
    for (int8_t iChannel = 0; iChannel < TCC0_CC_NUM; iChannel++) {
        TCC0->CC[iChannel].reg = 0;
        TCC0->CCBUF[iChannel].reg = 0;
    }

    for (int8_t iChannel = 0; iChannel < TCC1_CC_NUM; iChannel++) {
        TCC1->CC[iChannel].reg = 0;
        TCC1->CCBUF[iChannel].reg = 0;
    }

    TCC0->PER.reg = newPeriod - 1;
    TCC1->PER.reg = newPeriod - 1;

    // Notify the StepGenerators of the new maximum rate
    for (uint8_t iMotor = 0; iMotor < MOTOR_CON_CNT; iMotor++) {
        MotorConnectors[iMotor]->StepsPerSampleMaxSet(newPeriod);
    }

    TCC0->CTRLA.bit.ENABLE = 1; // Enable TCC0
    TCC1->CTRLA.bit.ENABLE = 1; // Enable TCC1

    SYNCBUSY_WAIT(TCC0, TCC_SYNCBUSY_ENABLE);
    SYNCBUSY_WAIT(TCC1, TCC_SYNCBUSY_ENABLE);

    return true;
}

bool MotorManager::MotorModeSet(MotorPair motorPair,
                                Connector::ConnectorModes newMode) {
    if (motorPair == MOTOR_ALL) {
        return MotorModeSet(MOTOR_M0M1, newMode) &&
               MotorModeSet(MOTOR_M2M3, newMode);
    }

    switch (newMode) {
        case Connector::CPM_MODE_A_DIRECT_B_DIRECT:
        case Connector::CPM_MODE_STEP_AND_DIR:
        case Connector::CPM_MODE_A_DIRECT_B_PWM:
        case Connector::CPM_MODE_A_PWM_B_PWM:
            m_motorModes[motorPair] = newMode;
            MotorConnectors[motorPair * 2]->Mode(newMode);
            MotorConnectors[motorPair * 2 + 1]->Mode(newMode);

            if (newMode == Connector::CPM_MODE_STEP_AND_DIR) {
                PMUX_ENABLE(m_stepPorts[motorPair],
                            m_stepDataBits[motorPair]);
            }
            else {
                PMUX_DISABLE(m_stepPorts[motorPair],
                             m_stepDataBits[motorPair]);
            }
            break;
        default:
            break;
    }
    return (m_motorModes[motorPair] == newMode);
}

void MotorManager::Initialize() {
    m_motorModes[MOTOR_M0M1] = Connector::CPM_MODE_A_DIRECT_B_DIRECT;
    m_motorModes[MOTOR_M2M3] = Connector::CPM_MODE_A_DIRECT_B_DIRECT;
    MotorInputClocking(CLOCK_RATE_NORMAL); // This will set m_clockRate

    for (uint8_t i = 0; i < NUM_MOTOR_PAIRS; i++) {
        // Configure the GClk output pin that will be used as the CPM step
        // output carrier signal
        PIN_CONFIGURATION(m_stepPorts[i], m_stepDataBits[i],  0);
        DATA_OUTPUT_STATE(m_stepPorts[i], 1UL << m_stepDataBits[i], false);
        PMUX_SELECTION(m_stepPorts[i], m_stepDataBits[i], PER_GCLK_AC);
        DATA_DIRECTION_OUTPUT(m_stepPorts[i], 1UL << m_stepDataBits[i]);
    }

    PinMuxSet();

    m_initialized = true;
}

/**
    Helper function to control if the step rate signal is active
**/
void MotorManager::PinMuxSet() {
    // Configure the motor connectors to be in the specified mode
    for (uint8_t iMotor = 0; iMotor < MOTOR_CON_CNT; iMotor++) {
        MotorConnectors[iMotor]->Mode(m_motorModes[iMotor / 2]);
    }

    // Turn on the carrier signals for S&D if needed
    for (uint8_t iMotorPair = 0; iMotorPair < NUM_MOTOR_PAIRS; iMotorPair++) {
        if (m_motorModes[iMotorPair] == Connector::CPM_MODE_STEP_AND_DIR) {
            PMUX_ENABLE(m_stepPorts[iMotorPair], m_stepDataBits[iMotorPair]);
        }
        else {
            PMUX_DISABLE(m_stepPorts[iMotorPair], m_stepDataBits[iMotorPair]);
        }
    }
}

} // ClearCore namespace