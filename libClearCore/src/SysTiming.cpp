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
    ClearCore timing profiling utility functions
**/

#include "SysTiming.h"
#include <sam.h>

namespace ClearCore {

// Make the interrupt rate visible to everyone
extern const uint16_t SampleRateHz;
extern bool FastSysTick;
volatile uint32_t tickCnt = 0;

SysTiming &TimingMgr = SysTiming::Instance();

SysTiming::SysTiming() :
    m_isrStartCycle(0),
    m_isrMinCycles(UINT32_MAX),
    m_isrMaxCycles(0),
    m_isrLastCycles(0),
    m_msTickCnt(0),
    m_fractMsTick(MS_TO_SAMPLES),
    m_lastIsrStartCnt(0),
    m_microAdj(0),
    m_microAdjHigh(0),
    m_microAdjLow(0),
    m_microAdjHighRemainder(0),
    m_microAdjLowRemainder(0) {}


SysTiming &SysTiming::Instance() {
    static SysTiming *instance = new SysTiming();
    return *instance;
}

void SysTiming::IsrStart() {
    m_isrStartCycle = DWT->CYCCNT;
}

void SysTiming::IsrEnd() {
    m_isrLastCycles = DWT->CYCCNT - m_isrStartCycle;
    if (m_isrMinCycles > m_isrLastCycles) {
        m_isrMinCycles = m_isrLastCycles;
    }
    if (m_isrMaxCycles < m_isrLastCycles) {
        m_isrMaxCycles = m_isrLastCycles;
    }
}

void SysTiming::GetIsrLoading(uint32_t &minSlot, uint32_t &maxSlot) {
    minSlot = m_isrMinCycles;
    m_isrMinCycles = m_isrLastCycles;
    maxSlot = m_isrMaxCycles;
    m_isrMaxCycles = m_isrLastCycles;
}

uint32_t SysTiming::Microseconds(void) {
    // Microseconds = CPU cycles / CYCLES_PER_MICROSECOND
    // Since the cycle counter wraps before Microseconds reaches UINT32_MAX
    // keep track of when the cycle counter wraps and adjust accordingly
    uint32_t cycleCounter = DWT->CYCCNT;
    if (cycleCounter > UINT32_MAX / 2) {
        return ((cycleCounter - m_microAdjHighRemainder) /
                CYCLES_PER_MICROSECOND) + m_microAdjHigh;
    }
    else {
        return ((cycleCounter + m_microAdjLowRemainder) /
                CYCLES_PER_MICROSECOND) + m_microAdjLow;
    }
}

void SysTiming::Update() {
    // Detaching a debugger can clear CoreDebug_DEMCR_TRCENA_Msk
    // so make sure it stays set to keep the cycle counter enabled
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;

    // Update the millisecond tick counter
    if (!--m_fractMsTick) {
        m_msTickCnt++;
        m_fractMsTick = MS_TO_SAMPLES;
    }

    // Since the cycleCounter wraps at 2^32 and we have to divide cycleCounter
    // by CYCLES_PER_MICROSECOND to get microseconds, the microsecond
    // calculation would wrap before we get to the desired 2^32 wrap point.
    // To account for this we need to keep a counter of how many times we hit
    // the wrap point to effectively extend the number of bits in the
    // cycleCounter and control the wrap point properly.
    // Precalculate adjustment values at UINT32_MAX/4 and UINT32_MAX*3/4
    // by timing when the adjustments are calculated and used, we can safely
    // modify the values when they are not in use (~9 sec margin).
    if (((m_isrStartCycle ^ m_lastIsrStartCnt) & 0xc0000000) == 0x40000000) {
        // At UINT32_MAX*3/4, prepare for the upcoming wrap point
        if (m_isrStartCycle & 0x80000000) {
            // Increment the wrap counter and set the adjustment to be used
            // when the cycle counter is < UINT32_MAX/2
            uint64_t cycCnt64 = (uint64_t)(++m_microAdj) << 32;
            m_microAdjLow = cycCnt64 / CYCLES_PER_MICROSECOND;
            // Are there any remainder bits to be added?
            if (m_microAdjLow) {
                m_microAdjLowRemainder =
                    cycCnt64 - (m_microAdjLow * CYCLES_PER_MICROSECOND);
            }
            // If this adjustment value does not modify the microsecond count
            // it is safe to reset the wrap counter and remainder to zero
            else {
                m_microAdj = 0;
                m_microAdjLowRemainder = 0;
            }
        }
        // At UINT32_MAX/4, set the adjustment to be used when the
        // cycle counter is > UINT32_MAX/2
        else {
            m_microAdjHigh = m_microAdjLow + 1;
            m_microAdjHighRemainder =
                CYCLES_PER_MICROSECOND - m_microAdjLowRemainder;
        }
    }
    m_lastIsrStartCnt = m_isrStartCycle;
}

void SysTiming::ResetMilliseconds() {
    m_msTickCnt = 0;
    m_fractMsTick = MS_TO_SAMPLES;
}

void SysTiming::ResetMicroseconds() {
    m_microAdj = 0;
    m_microAdjHigh = 0;
    m_microAdjLow = 0;
    m_microAdjHighRemainder = 0;
    m_microAdjLowRemainder = 0;
    m_lastIsrStartCnt -= DWT->CYCCNT;
    DWT->CYCCNT = 0;
}

bool SysTiming::SysTickPeriodMicroSec(uint32_t microSeconds) {
    // If the SysTick is faster than the sample rate set a
    // flag to do the "slow update" within the sample interrupt
    FastSysTick = microSeconds < SAMPLE_PERIOD_MICROSECONDS;
    return SysTick_Config(microSeconds * CYCLES_PER_MICROSECOND);
}
}

#ifdef __cplusplus
extern "C" {
#endif

uint32_t Milliseconds(void) {
    return ClearCore::TimingMgr.Milliseconds();
}


uint32_t Microseconds(void) {
    return ClearCore::TimingMgr.Microseconds();
}

void Delay_cycles(uint64_t cycles) {
    // Get a snapshot of the cycle counter as we enter the delay function
    uint32_t cyclesLast = DWT->CYCCNT;

    // If we do not need to delay, bail out
    if (cycles == 0) {
        return;
    }

    uint64_t cyclesRemaining = cycles;
    uint32_t cyclesNow = DWT->CYCCNT;

    while (cyclesRemaining > cyclesNow - cyclesLast) {
        cyclesRemaining -= (cyclesNow - cyclesLast);
        cyclesLast = cyclesNow;
        cyclesNow = DWT->CYCCNT;
    }
}

#ifdef __cplusplus
}
#endif // __cplusplus