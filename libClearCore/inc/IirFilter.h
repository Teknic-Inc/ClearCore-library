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

#ifndef __IIRFILTER_H__
#define __IIRFILTER_H__

#include <math.h>
#include "SysTiming.h"

#ifndef HIDE_FROM_DOXYGEN
namespace ClearCore {

//*****************************************************************************
// NAME                                                                       *
//  Iir16 class
//
// DESCRIPTION
///     \brief An IIR filter that filters a 16-bit input and provides a 16-bit
///     output.
///
///     The classic form is:
///     output = (1-K)*input + K*output
///
///     The DSP efficient form is:
///            = input - K*input + K*output
//
class Iir16 {
public:
    Iir16(void) : m_tc(0), m_z(0) {};

    /**
        Update the output with this input and return new output.
    **/
    void Update(uint16_t input) {
        m_z = ((static_cast<int64_t>(m_z) * m_tc) >> 15) -
              ((static_cast<int32_t>(input) * m_tc) << 1) +
              (static_cast<int32_t>(input) << 16);
    }
    /**
        \return Return the last output
    **/
    uint16_t LastOutput() {
        return (m_z >> 16);
    };

    /**
        Set TC
    **/
    void Tc(uint16_t newTc) {
        m_tc = newTc;
    };

    /**
        Get TC
    **/
    uint16_t Tc() {
        return m_tc;
    };

    void TcSamples(uint16_t riseSamples99pct) {
        float tcTemp = powf(.01, 1. / riseSamples99pct) * 32768 + 0.5;
        m_tc = (tcTemp < INT16_MAX) ? tcTemp : INT16_MAX;
    }

    uint16_t TcSamples() {
        return logf(0.01) / logf(m_tc / 32768.);
    }

    uint16_t Tc_ms() {
        return TcSamples() / MS_TO_SAMPLES;
    }

    void Tc_ms(uint16_t riseMs99pct) {
        TcSamples(riseMs99pct * MS_TO_SAMPLES);
    }

    // Reset the filter to this level
    void Reset(uint16_t newSetting) {
        m_z = (newSetting << 16);
    }

private:
    uint16_t m_tc; // Filter time constant (positive)
    int32_t m_z;  // "Z" output/accumulator
};

} // ClearCore namespace
#endif // HIDE_FROM_DOXYGEN
#endif // #ifndef __IIRFILTER_H__
//                                                                            *
//*****************************************************************************