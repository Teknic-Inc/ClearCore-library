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
#include "SysTiming.h"
#include "SysUtils.h"
#include "atomic_gcc.h"
#include <stdlib.h>


namespace ClearCore {

EncoderInput &EncoderIn = EncoderInput::Instance();

EncoderInput &EncoderInput::Instance() {
    static EncoderInput *instance = new EncoderInput();
    return *instance;
}

int32_t EncoderInput::Position() {
    return atomic_load_n(&m_curPosn) + atomic_load_n(&m_offsetAdjustment);
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
        PMUX_SELECTION(m_indexInfo->gpioPort, m_indexInfo->gpioPin, PER_TIMER_PDEC);
        
        SYNCBUSY_WAIT(PDEC, PDEC_SYNCBUSY_CTRLB);
        
        // Block the interrupt while clearing the position history
        __disable_irq();
        PDEC->CTRLBSET.reg = PDEC_CTRLBSET_CMD_READSYNC;
        SYNCBUSY_WAIT(PDEC, PDEC_SYNCBUSY_COUNT);
        int16_t currentHwPosn = PDEC->COUNT.reg;
        m_hwPosn = currentHwPosn;
        m_velocity = 0;
        for(uint8_t i = 0; i < VEL_EST_SAMPLES; i++) {
            m_posnHistory[i] = currentHwPosn;
        }
        m_posnHistoryIndex = 0;
        m_enabled = true;
        __enable_irq();
    }
    else {
        m_enabled = false;
        m_velocity = 0;
        PDEC->CTRLA.bit.ENABLE = 0;
        PDEC->CTRLBSET.reg = PDEC_CTRLBSET_CMD_STOP;
        // Enabling peripheral mux for interrupts
        PMUX_SELECTION(m_aInfo->gpioPort, m_aInfo->gpioPin, PER_EXTINT);
        PMUX_SELECTION(m_bInfo->gpioPort, m_bInfo->gpioPin, PER_EXTINT);
        PMUX_SELECTION(m_indexInfo->gpioPort, m_indexInfo->gpioPin, PER_EXTINT);
    }
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
      m_enabled(false) {
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
                      PDEC_CTRLA_PINEN2 | PDEC_CTRLA_ANGULAR_Msk;

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
    // Adjust the measured position
    atomic_add_fetch(&m_curPosn, currentHwPosn - m_hwPosn);
    m_hwPosn = currentHwPosn;
    // Calculate the velocity based on the position change in the 
    // last VEL_EST_SAMPLES sample times and convert to cnts/sec
    int16_t posnDelta = currentHwPosn - m_posnHistory[m_posnHistoryIndex];
    m_velocity = static_cast<int32_t>(posnDelta) *
        (_CLEARCORE_SAMPLE_RATE_HZ / VEL_EST_SAMPLES);
    m_posnHistory[m_posnHistoryIndex] = currentHwPosn;
    m_posnHistoryIndex = (m_posnHistoryIndex + 1) % VEL_EST_SAMPLES;
}

}