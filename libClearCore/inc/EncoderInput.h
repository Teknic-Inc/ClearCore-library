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
    \file EncoderInput.h
    \brief ClearCore encoder input object.

    Provides position information from quadrature and index signals.
**/


#ifndef __ENCODER_INPUT_H__
#define __ENCODER_INPUT_H__

#include "PeripheralRoute.h"

/// Number of encoder samples to use for velocity calculation
#define VEL_EST_SAMPLES 50

namespace ClearCore {

typedef void (*voidFuncPtr)(void);

/**
    \brief ClearCore Position Decoder.

    Provides consolidated access to the input state of all of the ClearCore
    connectors.
**/
class EncoderInput {
    friend class SysManager;
public:

#ifndef HIDE_FROM_DOXYGEN
    /**
        Public accessor for singleton instance.
    **/
    static EncoderInput &Instance();

    /**
        \brief Debug function to monitor the PDEC peripheral

        \code{.cpp}
        if (EncoderIn.HwPosition() > 1000) {
            // Position passed 1000, do something.
        }
        \endcode

        \return The position count of the PDEC peripheral.
    **/
    volatile const int16_t& HwPosition() {
        return m_hwPosn;
    }
#endif

    /**
        \brief Read the current position of the encoder

        \code{.cpp}
        if (EncoderIn.Position() > 1000) {
            // Position passed 1000, do something.
        }
        \endcode

        \return position count of the Encoder Input module.
    **/
    int32_t Position();

    /**
        \brief Set the current position of the encoder

        \code{.cpp}
        // Zero the encoder position
        EncoderIn.Position(0);
        \endcode

        \return The number of counts that the encoder position was shifted.
    **/
    int32_t Position(int32_t newPosn);

    /**
        \brief Adjust the current position of the encoder

        \code{.cpp}
        // Shift the encoder numberspace upwards by 500 counts
        EncoderIn.AddToPosition(500);
        \endcode
    **/
    void AddToPosition(int32_t posnAdjust);
    
    /**
        \brief Set whether the encoder input should be active or not.

        \code{.cpp}
        // Before using the Encoder Input, it has to be enabled.
        EncoderIn.Enable(true);
        \endcode
    **/
    void Enable(bool isEnabled);
    
    /**
        \brief Swap the sense of positive and negative encoder directions.

        \code{.cpp}
        // Set the encoder counting direction to match the wiring and code.
        EncoderIn.SwapDirection(true);
        \endcode
    **/
    void SwapDirection(bool isSwapped);
    
    /**
        \brief Read the velocity of the encoder input (counts per second)

        \code{.cpp}
        // Read the current encoder velocity
        int32_t encoderSpeed = EncoderIn.Velocity();
        \endcode

        \return The encoder input velocity in counts per second.
    **/
    volatile const int32_t& Velocity() {
        return m_velocity;
    }
    
private:
    
    const PeripheralRoute *m_aInfo;
    const PeripheralRoute *m_bInfo;
    const PeripheralRoute *m_indexInfo;
    int32_t m_curPosn;
    int32_t m_offsetAdjustment;
    int32_t m_velocity;
    int16_t m_hwPosn;
    int16_t m_posnHistory[VEL_EST_SAMPLES];
    uint8_t m_posnHistoryIndex;
    bool m_enabled;

    /**
        Construct
    **/
    EncoderInput();

    void Initialize();

    void Update();

}; // EncoderInput

} // ClearCore namespace

#endif /* __ENCODER_INPUT_H__ */