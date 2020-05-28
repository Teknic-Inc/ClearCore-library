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
    \file SysTiming.h
    \brief ClearCore timing profiling utility functions
**/

// include guard
#ifndef __SYSTIMING_H__
#define __SYSTIMING_H__

#include <stdint.h>

/** Number of CPU cycles, in Hz. (120MHz) **/
#ifndef CPU_CLK
#define CPU_CLK 120000000
#endif // CPU_CLK

/**
    ClearCore sample rate for main interrupt processing (5 kHz).
**/
#ifndef _CLEARCORE_SAMPLE_RATE_HZ
#define _CLEARCORE_SAMPLE_RATE_HZ (5000)
#endif

/**
    ClearCore sample rate, expressed in sample times (5).
**/
#define MS_TO_SAMPLES (_CLEARCORE_SAMPLE_RATE_HZ / 1000)
/**
    Number of CPU cycles per interrupt time (24,000).
**/
#define CYCLES_PER_INTERRUPT   (CPU_CLK / _CLEARCORE_SAMPLE_RATE_HZ)
/**
    ClearCore sample time, expressed in microseconds (200us).
**/
#define SAMPLE_PERIOD_MICROSECONDS (1000000UL / _CLEARCORE_SAMPLE_RATE_HZ)
/**
    Number of CPU cycles per microsecond (120).
**/
#define CYCLES_PER_MICROSECOND (CPU_CLK / 1000000)
/**
    Number of CPU cycles per millisecond (120,000).
**/
#define CYCLES_PER_MILLISECOND (CPU_CLK / 1000)
/**
    Number of CPU cycles per second (120,000,000).
**/
#define CYCLES_PER_SECOND      (CPU_CLK)



namespace ClearCore {

/** Refresh rate of ClearCore background processing.
    \note The refresh rate is 5 kHz, so the refresh occurs once every 200
    microseconds.
 **/
const uint16_t SampleRateHz = _CLEARCORE_SAMPLE_RATE_HZ;

/**
    \class SysTiming
    \brief ClearCore system timing class

    This class provides an interface for various timing-related operations.
**/
class SysTiming {
    friend class SysManager;

public:
#ifndef HIDE_FROM_DOXYGEN
    /**
        \brief Return the minimum and maximum fast interrupt duration cycles

        Returns the minimum and maximum isr duration cycles since the last call
        to GetIsrLoading. The minimum and maximum values then reset to the
        number of cycles in the last interrupt.

        \param[out] minSlot The minimum slot loading cycles since the last call
        \param[out] maxSlot The maximum slot loading cycles since the last call
    **/
    void GetIsrLoading(uint32_t &minSlot, uint32_t &maxSlot);

    /**
        Public accessor for singleton instance
    **/
    static SysTiming &Instance();
#endif

    /**
        \brief Number of microseconds elapsed since the ClearCore was
        initialized.

        Uses the processor's cycle counter register to calculate the
        number of microseconds elapsed.

        \code{.cpp}
        // Implement a timeout of 750 microseconds.
        uint32_t timeout = 750;
        uint32_t start = Microseconds();
        while (Microseconds() - start < timeout) {
            // wait for timeout...
        }
        \endcode

        \returns Number of microseconds since board initialization.

        \note Rolls over every ~71.5 minutes (at UINT32_MAX microseconds)
    **/
    uint32_t Microseconds();

    /**
        Resets the microsecond timer.

        \code{.cpp}
        // Implement a timeout of 750 microseconds.
        uint32_t timeout = 750;
        ResetMicroseconds();
        while (Microseconds() < timeout) {
            // wait for timeout...
        }
        \endcode
    **/
    void ResetMicroseconds();

    /**
        \brief Number of milliseconds elapsed since the ClearCore was
        initialized.

        Uses the fast update interrupt counter to retrieve the number of
        milliseconds elapsed.

        \code{.cpp}
        // Implement a timeout of 2000 milliseconds.
        uint32_t timeout = 2000;
        uint32_t start = Milliseconds();
        while (Milliseconds() - start < timeout) {
            // wait for timeout...
        }
        \endcode

        \return Number of milliseconds since board initialization.

        \note Rolls over every ~49.7 days (at UINT32_MAX milliseconds)
    **/
    volatile const uint32_t &Milliseconds() {
        return m_msTickCnt;
    }

    /**
        Resets the millisecond timer.

        \code{.cpp}
        // Implement a timeout of 2000 milliseconds.
        uint32_t timeout = 2000;
        ResetMilliseconds();
        while (Milliseconds() < timeout) {
            // wait for timeout...
        }
        \endcode
    **/
    void ResetMilliseconds();

#ifndef HIDE_FROM_DOXYGEN
    /**
        \brief Sets the SysTick period

        Sets the SysTick period, also resets SysTickCnt.
        Default 1000uS

        Setting the SysTick faster than the ClearCore Sample Rate will cause the
        UpdateSlow to be updated at the ClearCore Sample Rate.

        \param[in] microseconds (optional) SysTick period

        \return success
    **/
    bool SysTickPeriodMicroSec(uint32_t microseconds = 1000);
#endif

private:
    uint32_t m_isrStartCycle;
    uint32_t m_isrMinCycles;
    uint32_t m_isrMaxCycles;
    uint32_t m_isrLastCycles;
    uint32_t m_msTickCnt;
    uint8_t m_fractMsTick;
    uint32_t m_lastIsrStartCnt;
    uint32_t m_microAdj;
    uint32_t m_microAdjHigh;
    uint32_t m_microAdjLow;
    uint32_t m_microAdjHighRemainder;
    uint32_t m_microAdjLowRemainder;


    /**
        Constructor
    **/
    SysTiming();

    /**
        \brief Signal the start of the main interrupt service routine

        Captures the CPU clock cycle counter at the start of the ISR
    **/
    void IsrStart();
    /**
        \brief Signal the end of the main interrupt service routine

        Captures the CPU clock cycle counter at the end of the ISR.
        Updates the minimum and maximum ISR duration values.
    **/
    void IsrEnd();
    /**
        \brief Update at the sample rate

        Updates the millisecond tick counter. Keeps track of CPU
        cycle counter overflows so that the microseconds calculations
        can properly wrap at UINT32_MAX
    **/
    void Update();

};

}

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/**
    \brief Number of milliseconds since the ClearCore was initialized

    Uses the fast update interrupt counter to retrieve the number of
    milliseconds elapsed.

    \return milliseconds

    \note Rolls over every ~49.7 days (at UINT32_MAX milliseconds)
**/
uint32_t Milliseconds(void);

/**
    \brief Number of microseconds since the ClearCore was initialized

    Uses the processor's cycle counter register to calculate the
    number of microseconds elapsed.

    \returns Microseconds

    \note Rolls over every ~71.5 minutes (at UINT32_MAX microseconds)
**/
uint32_t Microseconds(void);

/**
    \brief Blocks for operations cycles CPU cycles

    \param[in] cycles Time in CPU cycles to delay
**/
void Delay_cycles(uint64_t cycles);

/**
    \brief Blocks operations for ms milliseconds

    \param[in] ms Time in milliseconds to delay
**/
inline void Delay_ms(uint32_t ms)  {
    return Delay_cycles(static_cast<uint64_t>(ms) * CYCLES_PER_MILLISECOND);
}

/**
    \brief Blocks for operations usec microseconds

    \param[in] usec Time in microseconds to delay
**/
inline void Delay_us(uint32_t usec) {
    return Delay_cycles(static_cast<uint64_t>(usec) * CYCLES_PER_MICROSECOND);
}

#ifdef __cplusplus
}
#endif // __cplusplus

// end of include guard
#endif // __SYSTIMING_H__
