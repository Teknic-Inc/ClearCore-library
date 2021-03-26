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
    \brief ClearCore Position Sensor Decoder.

    Provides position and velocity information from external quadrature and
    index signals. Use the Encoder Input Adapter Board (PN: CL-ENCDR-DFIN) to
    wire an external encoder to ClearCore.

    \note When using the Encoder Input Adapter Board, ClearCore's DI-6/DI-7/DI-8
    inputs will be unavailable. Refer to the ClearCore User Manual for specs
    and wiring information.
**/
class EncoderInput {
    friend class SysManager;
public:

#ifndef HIDE_FROM_DOXYGEN

    /**
        Construct
    **/
    EncoderInput();

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

    /**
        \brief Index interrupt helper function.

        Store the location of the index to be processed in the next update.
    **/
    void IndexDetected(int16_t posn) {
        m_hwIndex = posn;
        m_processIndex = true;
    }
#endif

    /**
        \brief Read the current position of the encoder

        \code{.cpp}
        if (EncoderIn.Position() > 1000) {
            // Position passed 1000, do something.
        }
        \endcode

        \return The position count of the Encoder Input module.

        \note Available with software version 1.1 or greater. See
        \ref InstallationInstructions for information on updating library
        versions.
    **/
    int32_t Position();

    /**
        \brief Set the current position of the encoder

        \code{.cpp}
        // Zero the encoder position
        EncoderIn.Position(0);
        \endcode

        \return The number of counts that the encoder position was shifted.

        \note Available with software version 1.1 or greater. See
        \ref InstallationInstructions for information on updating library
        versions.
    **/
    int32_t Position(int32_t newPosn);

    /**
        \brief Adjust the current position of the encoder

        \code{.cpp}
        // Shift the encoder numberspace upwards by 500 counts
        EncoderIn.AddToPosition(500);
        \endcode

        \note Available with software version 1.1 or greater. See
        \ref InstallationInstructions for information on updating library
        versions.
    **/
    void AddToPosition(int32_t posnAdjust);

    /**
        \brief Read the last index position of the encoder

        \code{.cpp}
        static int32_t lastIndex = 0;
        if (EncoderIn.IndexPosition() != lastIndex) {
            // A new index pulse was seen, do something.

            // Update the position of the last index pulse seen
            lastIndex = EncoderIn.IndexPosition();
        }
        \endcode

        \return The position count of the last Encoder index pulse.

        \note Available with software version 1.1 or greater. See
        \ref InstallationInstructions for information on updating library
        versions.
    **/
    int32_t IndexPosition();

    /**
        \brief Set whether the encoder input should be active or not.

        \code{.cpp}
        // Before using the Encoder Input, it has to be enabled.
        EncoderIn.Enable(true);
        \endcode

        \note Available with software version 1.1 or greater. See
        \ref InstallationInstructions for information on updating library
        versions.
    **/
    void Enable(bool isEnabled);

    /**
        \brief Swap the sense of positive and negative encoder directions.

        \code{.cpp}
        // Set the encoder counting direction to match the wiring and code.
        EncoderIn.SwapDirection(true);
        \endcode

        \note Available with software version 1.1 or greater. See
        \ref InstallationInstructions for information on updating library
        versions.
    **/
    void SwapDirection(bool isSwapped);

    /**
        \brief Read the velocity of the encoder input (counts per second)

        \code{.cpp}
        // Read the current encoder velocity
        int32_t encoderSpeed = EncoderIn.Velocity();
        \endcode

        \return The encoder input velocity in counts per second.

        \note Available with software version 1.1 or greater. See
        \ref InstallationInstructions for information on updating library
        versions.
    **/
    volatile const int32_t& Velocity() {
        return m_velocity;
    }

    /**
        \brief Check if there was an index pulse in the last sample time.

        \code{.cpp}
        // Check for an index pulse
        bool hadIndex = EncoderIn.IndexDetected();
        \endcode

        \return True if the index transitioned from deasserted to asserted
        in the last sample time.

        \note Available with software version 1.1 or greater. See
        \ref InstallationInstructions for information on updating library
        versions.
    **/
    volatile const bool& IndexDetected() {
        return m_indexDetected;
    }

    /**
        \brief Invert the edge that the index detection triggers on.

        The index nominally triggers when the digital input value rises.
        This setting allows the index to trigger on the falling edge.

        \code{.cpp}
        // Set the index pulse to trigger on the falling input edge.
        bool hadIndex = EncoderIn.IndexInverted(true);
        \endcode

        \note Available with software version 1.1 or greater. See
        \ref InstallationInstructions for information on updating library
        versions.
    **/
    void IndexInverted(bool invert);

    /**
        \brief Query for a quadrature error

        \code{.cpp}
        // Check for a quadrature error
        bool quadratureError = EncoderIn.QuadratureError();
        \endcode

        \return The current state of the quadrature error flag in the position
        decoder module.

        \note Available with software version 1.1 or greater. See
        \ref InstallationInstructions for information on updating library
        versions.
    **/
    bool QuadratureError();

    /**
        \brief Clear a quadrature error

        \code{.cpp}
        // Clear a quadrature error if there is one
        if (EncoderIn.QuadratureError()) {
            EncoderIn.ClearQuadratureError();
        }
        \endcode

        \note Available with software version 1.1 or greater. See
        \ref InstallationInstructions for information on updating library
        versions.
    **/
    void ClearQuadratureError();

    /**
        \brief Get the number of encoder steps received in the last sample time.

        \code{.cpp}
        if (EncoderIn.StepsLastSample() == 0) {
            // Do something in the case that we haven't received steps
        }
        \endcode

        \return The number of steps received in the last sample time.

        \note Available with software version 1.1 or greater. See
        \ref InstallationInstructions for information on updating library
        versions.
    **/
    volatile const int16_t& StepsLastSample() {
        return m_stepsLast;
    }

private:
    const PeripheralRoute *m_aInfo;
    const PeripheralRoute *m_bInfo;
    const PeripheralRoute *m_indexInfo;
    int32_t m_curPosn;
    int32_t m_offsetAdjustment;
    int32_t m_velocity;
    int16_t m_hwPosn;
    int32_t m_posnHistory[VEL_EST_SAMPLES];
    uint8_t m_posnHistoryIndex;
    bool m_enabled;
    bool m_processIndex;
    int16_t m_hwIndex;
    int32_t m_indexPosn;
    bool m_indexDetected;
    bool m_indexInverted;
    int16_t m_stepsLast;

    void Initialize();

    void Update();

}; // EncoderInput

} // ClearCore namespace

#endif /* __ENCODER_INPUT_H__ */