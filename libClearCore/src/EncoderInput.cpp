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
    ClearCore position decoder object.

    Provides position information from quadrature and index signals.
**/

#include "EncoderInput.h"
#include "HardwareMapping.h"
#include "InputManager.h"
#include "SysTiming.h"
#include "SysUtils.h"
#include "atomic_utils.h"
#include <stdlib.h>

#define EIC_INDEX_INTERRUPT_PRIORITY 1
#define EIC_INTERRUPT_PRIORITY 7

namespace ClearCore {
extern InputManager &InputMgr;
extern EncoderInput EncoderIn;

void IndexCallback() {
    PDEC->CTRLBSET.reg = PDEC_CTRLBSET_CMD_READSYNC;
    SYNCBUSY_WAIT(PDEC, PDEC_SYNCBUSY_COUNT);
    EncoderIn.IndexDetected(PDEC->COUNT.reg);
}

int32_t EncoderInput::Position() {
    return atomic_load_n(&m_curPosn) + atomic_load_n(&m_offsetAdjustment);
}

int32_t EncoderInput::IndexPosition() {
    return m_indexPosn + atomic_load_n(&m_offsetAdjustment);
}

int32_t EncoderInput::Position(int32_t newPosn) {
    int32_t newOffset = newPosn - atomic_load_n(&m_curPosn);
    int32_t oldOffset = atomic_exchange_n(&m_offsetAdjustment, newOffset);
    return newOffset - oldOffset;
}

void EncoderInput::AddToPosition(int32_t posnAdjust) {
    atomic_add_fetch(&m_offsetAdjustment, posnAdjust);
}

void EncoderInput::Enable(bool isEnabled) {
    while (PDEC->SYNCBUSY.reg);
    if (isEnabled) {
        PDEC->CTRLA.bit.ENABLE = 1;
        PDEC->CTRLBSET.reg = PDEC_CTRLBSET_CMD_START;

        // Enabling peripheral mux for position decoder
        PMUX_SELECTION(m_aInfo->gpioPort, m_aInfo->gpioPin, PER_TIMER_PDEC);
        PMUX_SELECTION(m_bInfo->gpioPort, m_bInfo->gpioPin, PER_TIMER_PDEC);
        PMUX_SELECTION(m_indexInfo->gpioPort, m_indexInfo->gpioPin, PER_EXTINT);

        SYNCBUSY_WAIT(PDEC, PDEC_SYNCBUSY_CTRLB);

        // Block the interrupt while clearing the position history
        __disable_irq();
        PDEC->CTRLBSET.reg = PDEC_CTRLBSET_CMD_READSYNC;
        SYNCBUSY_WAIT(PDEC, PDEC_SYNCBUSY_COUNT);
        int16_t currentHwPosn = PDEC->COUNT.reg;
        m_hwPosn = currentHwPosn;
        m_velocity = 0;
        for(uint16_t i = 0; i < VEL_EST_SAMPLES; i++) {
            m_posnHistory[i] = currentHwPosn;
        }
        m_posnHistoryIndex = 0;
        m_enabled = true;
        __enable_irq();

        NVIC_SetPriority((IRQn_Type)(EIC_0_IRQn + m_indexInfo->extInt),
                         EIC_INDEX_INTERRUPT_PRIORITY);

        // Set an ISR to be called when the index pulse is seen.
        // Set the interrupt up as a one-time event and re-enable at sample time.
        InputMgr.InterruptHandlerSet(m_indexInfo->extInt, IndexCallback,
                                     m_indexInverted ? InputManager::FALLING : InputManager::RISING,
                                     m_enabled, true);
    }
    else {
        InputMgr.InterruptEnable(m_indexInfo->extInt, false);
        m_enabled = false;
        m_indexDetected = false;
        m_processIndex = false;
        m_velocity = 0;
        PDEC->CTRLA.bit.ENABLE = 0;
        PDEC->CTRLBSET.reg = PDEC_CTRLBSET_CMD_STOP;
        // Enabling peripheral mux for interrupts
        PMUX_SELECTION(m_aInfo->gpioPort, m_aInfo->gpioPin, PER_EXTINT);
        PMUX_SELECTION(m_bInfo->gpioPort, m_bInfo->gpioPin, PER_EXTINT);
        PMUX_SELECTION(m_indexInfo->gpioPort, m_indexInfo->gpioPin, PER_EXTINT);
        NVIC_SetPriority((IRQn_Type)(EIC_0_IRQn + m_indexInfo->extInt),
                         EIC_INTERRUPT_PRIORITY);
    }
}

void EncoderInput::IndexInverted(bool invert) {
    m_indexInverted = invert;
    InputMgr.InterruptHandlerSet(m_indexInfo->extInt, IndexCallback,
                                 m_indexInverted ? InputManager::FALLING : InputManager::RISING,
                                 m_enabled, true);
}

void EncoderInput::SwapDirection(bool isSwapped) {
    if (PDEC->CTRLA.bit.SWAP != isSwapped) {
        bool wasEnabled = PDEC->CTRLA.bit.ENABLE;
        if (wasEnabled) {
            PDEC->CTRLA.bit.ENABLE = 0;
            SYNCBUSY_WAIT(PDEC, PDEC_SYNCBUSY_ENABLE);
            PDEC->CTRLA.bit.SWAP = isSwapped;
            PDEC->CTRLA.bit.ENABLE = wasEnabled;
            SYNCBUSY_WAIT(PDEC, PDEC_SYNCBUSY_ENABLE);
            PDEC->CTRLBSET.reg = PDEC_CTRLBSET_CMD_START;
        }
        else {
            PDEC->CTRLA.bit.SWAP = isSwapped;
        }
    }
}

/*
    Construct and wire in our IO pins
*/
EncoderInput::EncoderInput()
    : m_aInfo(&IN06n_QuadA),
      m_bInfo(&IN07n_QuadB),
      m_indexInfo(&IN08n_QuadI),
      m_curPosn(0),
      m_offsetAdjustment(0),
      m_velocity(0),
      m_hwPosn(0),
      m_posnHistory{0},
      m_posnHistoryIndex(0),
      m_enabled(false),
      m_processIndex(false),
      m_hwIndex(0),
      m_indexPosn(0),
      m_indexDetected(false),
      m_indexInverted(false),
      m_stepsLast(0) {
}


void EncoderInput::Initialize() {

    // Set up PDEC

    // Set the clock source for PDEC to GCLK0 (120 MHz) and enable
    // the peripheral channel
    SET_CLOCK_SOURCE(PDEC_GCLK_ID, 0);

    // Enables the peripheral clock to PDEC
    CLOCK_ENABLE(APBCMASK, PDEC_);

    PDEC->CTRLA.reg = PDEC_CTRLA_MODE_QDEC | PDEC_CTRLA_CONF_X4 |
                      PDEC_CTRLA_PINEN0 | PDEC_CTRLA_PINEN1 |
                      PDEC_CTRLA_ANGULAR_Msk;

}

void EncoderInput::Update() {
    //If the encoder is disabled, just return
    if (!m_enabled) {
        return;
    }
    // Refresh the COUNT reading
    PDEC->CTRLBSET.reg = PDEC_CTRLBSET_CMD_READSYNC;
    SYNCBUSY_WAIT(PDEC, PDEC_SYNCBUSY_COUNT);
    int16_t currentHwPosn = PDEC->COUNT.reg;
    m_stepsLast = currentHwPosn - m_hwPosn;
    
    m_indexDetected = m_processIndex;
    if (m_processIndex) {
        m_indexPosn = atomic_load_n(&m_curPosn) + m_hwIndex - m_hwPosn;
        m_processIndex = false;
        // Re-enable the index capture interrupt
        InputMgr.InterruptEnable(m_indexInfo->extInt, true, false);
    }
    m_hwPosn = currentHwPosn;
    // Adjust the measured position
    int32_t posnNow = atomic_add_fetch(&m_curPosn, (int32_t)m_stepsLast);
    // Calculate the velocity based on the position change in the 
    // last VEL_EST_SAMPLES sample times and convert to cnts/sec
    int32_t posnDelta = posnNow - m_posnHistory[m_posnHistoryIndex];
    m_velocity = posnDelta * (_CLEARCORE_SAMPLE_RATE_HZ / VEL_EST_SAMPLES);
    m_posnHistory[m_posnHistoryIndex] = posnNow;
    m_posnHistoryIndex = (m_posnHistoryIndex + 1) % VEL_EST_SAMPLES;
}

bool EncoderInput::QuadratureError() {
    return PDEC->STATUS.bit.QERR;
}

void EncoderInput::ClearQuadratureError() {
    // Clear the quadrature error status
    PDEC->STATUS.reg = PDEC_STATUS_QERR;
}

}