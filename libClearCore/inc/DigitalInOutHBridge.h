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
    \file DigitalInOutHBridge.h

    \brief DigitalInOutHBridge Connector class for IO4 and IO5.

    Uses different hardware than the other IO connectors.
    Utilizes DigitalInOut for Input, Output, Output_PWM.
    Adds HBridge functionality, such as tone generation.
    The enable pin on the HBridge chip is treated the same as the output pin
    for the other connectors.
**/

#ifndef __DIGITALINOUTHBRIDGE_H__
#define __DIGITALINOUTHBRIDGE_H__

#include <stdint.h>
#include "Connector.h"
#include "DigitalInOut.h"
#include "PeripheralRoute.h"
#include "ShiftRegister.h"
#include "StatusManager.h"

namespace ClearCore {

/**
    \brief ClearCore H-Bridge digital output connector class.

    This manages a connector on the ClearCore board that supports H-Bridge
    digital output functionality, including sine tone generation. This connector
    can also be configured as a digital input or digital output.

    The following connector instances support H-Bridge digital output
    functionality:
    - #ConnectorIO4
    - #ConnectorIO5

    For more detailed information on the ClearCore Connector interface, check
    out the \ref ConnectorMain informational page.

    \note If overloaded, these connectors will need to be reset using the
    StatusManager::HBridgeReset function. This applies when running in H-bridge
    mode or digital input/output modes.
**/
class DigitalInOutHBridge : public DigitalInOut {
    friend class StatusManager;
    friend class SysManager;

public:
    /**
       \enum ToneState

       \brief Possible states of the tone generator.
       \note IO-4 and IO-5 each have their own tone generators, and are the
       only connectors capable of tone generation on the ClearCore.
    **/
    typedef enum {
        /**
            No tone is currently active.
        **/
        TONE_OFF = 0,
        /**
            A tone is playing indefinitely.
        **/
        TONE_CONTINUOUS,
        /**
            A tone is playing that will end after a specified duration.
        **/
        TONE_TIMED,
        /**
            A periodic tone is playing and the tone is currently sounding (in
            the "on" phase of the tone output cycle).
        **/
        TONE_PERIODIC_ON,
        /**
            A periodic tone is playing and the tone is currently silent (in the
            "off" phase of the tone output cycle).
        **/
        TONE_PERIODIC_OFF,
    } ToneState;

#ifndef HIDE_FROM_DOXYGEN
    /**
        \brief Default constructor so this connector can be a global and
        constructed by SysManager.

        \note Should not be called by anything other than SysManager
    **/
    DigitalInOutHBridge() {};
#endif

    /**
        \brief Get the connector's operational mode.

        \code{.cpp}
        if (ConnectorIO4.Mode() == Connector::OUTPUT_PWM) {
            // IO-4 is currently a PWM output.
        }
        \endcode

        \return The connector's current operational mode.
    **/
    virtual ConnectorModes Mode() override {
        return Connector::Mode();
    }

    /**
        \brief Set the connector's operational mode.

        \code{.cpp}
        // Configure IO-4 for tone output
        ConnectorIO4.Mode(Connector::OUTPUT_TONE);
        \endcode

        \param[in] newMode The new mode to be set.
        The valid modes for this connector type are:
        - #INPUT_DIGITAL
        - #OUTPUT_DIGITAL
        - #OUTPUT_PWM
        - #OUTPUT_H_BRIDGE
        - #OUTPUT_TONE
        - #OUTPUT_WAVE.
        \return Returns false if the mode is invalid or setup fails.

        \note If connector is in fault, attempts to set the mode will return
        false, but the mode that was commanded will be applied once the
        connector is no longer in a fault mode. Useful for getting out of a bad
        mode.
    **/
    bool Mode(ConnectorModes newMode) override;

    /**
        \brief Get connector type.

        \code{.cpp}
        if (ConnectorAlias.Type() == Connector::H_BRIDGE_TYPE) {
            // This generic connector variable is a DigitalInOutHBridge
            // connector
        }
        \endcode

        \return The type of the current connector.
    **/
    Connector::ConnectorTypes Type() override {
        return Connector::H_BRIDGE_TYPE;
    }

    /**
        \brief Set the amplitude of a PWM output for tone generation.

        \code{.cpp}
        // Set IO-4's amplitude to 1/10 of the max
        ConnectorIO4.ToneAmplitude(INT16_MAX / 10);
        \endcode

        \param[in] amplitude The amplitude of the tone (0 to INT16_MAX)
    **/
    void ToneAmplitude(int16_t amplitude);

    /**
        \brief Output a continuous tone from the H-Bridge.

        \code{.cpp}
        // Start a continuous 100Hz tone on IO-4
        ConnectorIO4.ToneContinuous(100);
        \endcode

        \param[in] frequency The frequency of the tone (Hz)
    **/
    void ToneContinuous(uint16_t frequency);

    /**
        \brief Output a tone from the H-Bridge for the specified duration.

        \code{.cpp}
        // Start a 50Hz, 500ms tone on IO-4 that does not block further code
        // execution
        ConnectorIO4.ToneTimed(50, 500);
        \endcode

        \param[in] frequency The frequency of the tone (Hz)
        \param[in] duration How long to play the tone (ms). Specifying a
        \a duration of 0 will result in a continuous tone (equivalent to
        calling #ToneContinuous(uint16_t frequency) with the supplied
        \a frequency).
        \param[in] blocking (optional) Sets whether the function will block all
        other operations until the tone has finished playing. Default: false.
        \param[in] forceDuration (optional) If true, the tone will be made to
        sound for the full \a duration specified, regardless of any subsequent
        tone function calls made before the \a duration has elapsed.
        Default: false.

        \note Duration is only accurate to the SysTickRate.
        \note The blocking and forceDuration parameters do not apply when
        duration = 0.
    **/
    void ToneTimed(uint16_t frequency, uint32_t duration,
                   bool blocking = false, bool forceDuration = false);

    /**
        \brief Output a periodic tone from the H-Bridge.

        \code{.cpp}
        // Start a 50Hz, 150ms on/25ms off tone on IO-4 that continues until
        // stopped by a call to ToneStop().
        ConnectorIO4.ToneTimed(50, 150, 25);
        \endcode

        \param[in] frequency The frequency of the tone (Hz)
        \param[in] timeOn Periodic on time (ms)
        \param[in] timeOff Periodic off time (ms)
    **/
    void TonePeriodic(uint16_t frequency, uint32_t timeOn, uint32_t timeOff);

    /**
        Stop the tone output.

        \code{.cpp}
        // Stop a tone playing on IO-4
        ConnectorIO4.ToneStop();
        \endcode
    **/
    void ToneStop();

    /**
        \brief Accessor for the state of the tone currently active on the
        H-Bridge

        \code{.cpp}
        if (ConnectorIO5.ToneActiveState() ==
            DigitalInOutHBridge::TONE_CONTINUOUS) {
            // An endless tone is currently playing on IO-5
        }
        \endcode

        \return The type of tone currently playing
    **/
    volatile const ToneState &ToneActiveState() {
        return m_toneState;
    }

    /**
        \brief Get connector's last sampled value.

        \code{.cpp}
        // In this example, IO-4 has been configured for digital input
        if (ConnectorIO4.State()) {
            // IO-4's input is currently high
        }
        \endcode

        \code{.cpp}
        // In this example, IO-4 has been configured for PWM output
        if (ConnectorIO4.State() < UINT8_MAX / 4) {
            // IO-4 is outputting PWM at a duty cycle less than 25%
        }
        \endcode

        \code{.cpp}
        // In this example, IO-4 has been configured for H-Bridge output
        if (ConnectorIO4.State() < INT16_MIN / 2) {
            // IO-4 is outputting differential PWM in the negative direction
            // at a duty cycle greater than 50% (between -50% and -100%)
        }
        \endcode
        \return The latest sample
    **/
    int16_t State() override;

    /**
        \brief Set the state of the Connector

        \code{.cpp}
        if (ConnectorIO5.Mode() == Connector::OUTPUT_DIGITAL) {
            // Assert IO-5's digital output
            ConnectorIO5.State(true);
        }
        if (ConnectorIO5.Mode() == Connector::OUTPUT_PWM) {
            // Set IO-5's digital PWM output to 50% duty
            // Valid range is 0 to UINT8_MAX
            ConnectorIO5.State(UINT8_MAX / 2);
        }
        if (ConnectorIO5.Mode() == Connector::OUTPUT_H_BRIDGE) {
            // Set IO-5's H-Bridge output to -25% duty
            // Valid range is -INT16_MAX to INT16_MAX
            ConnectorIO5.State(-INT16_MAX / 4);
        }
        \endcode

        \param[in] newState New state to set for the connector.

        \return success.
    **/
    bool State(int16_t newState) override;

    /**
        \brief Get R/W status of the connector.

        \code{.cpp}
        if (ConnectorIO4.IsWritable()) {
            // IO-4 is in an output mode
        }
        \endcode

        \return True if in #OUTPUT_DIGITAL, #OUTPUT_PWM, #OUTPUT_H_BRIDGE,
        #OUTPUT_TONE, or #OUTPUT_WAVE mode, false otherwise.
    **/
    bool IsWritable() override;

#ifndef HIDE_FROM_DOXYGEN
    /**
        Update the tone sine wave output value
    **/
    void ToneUpdate();
#endif

private:
    // Tone values
    int16_t m_amplitude;
    int16_t m_sinStep;
    int16_t m_angle;
    uint32_t m_toneStartTick;
    uint32_t m_toneOnTicks;
    uint32_t m_toneOffTicks;
    ToneState m_toneState;

    //Port, Pin, and Timer/Counter values
    const PeripheralRoute *m_pwmAInfo;
    const PeripheralRoute *m_pwmBInfo;

    Tcc *m_tcc;
    IRQn_Type m_tccIrq;

    bool m_inFault;
    bool m_forceToneDuration;

    /**
        Initialize hardware and/or internal state.
    **/
    void Initialize(ClearCorePins clearCorePin) override;

    /**
        \brief Update connector's state.

        Poll the underlying connector for new state update.

        This is typically called from a timer or main loop to update the
        underlying value.
    **/
    void Refresh() override;

    /**
        \brief Set the frequency for a PWM output.

        \param[in] frequency the frequency of the tone (Hz)
    **/
    inline void ToneFrequency(uint16_t frequency);

    /**
        \brief Sets the fault flag and disables the H-Bridge output when faulted

        During fault, the HBridge I/O pin will be disabled.
    **/
    void FaultState(bool isFaulted);

    /**
        Construct and wire in LED bit number
    **/
    DigitalInOutHBridge(ShiftRegister::Masks ledMask,
                        const PeripheralRoute *inputInfo,
                        const PeripheralRoute *outputInfo,
                        const PeripheralRoute *pwmAInfo,
                        const PeripheralRoute *pwmBInfo,
                        IRQn_Type tccIrq,
                        bool invertDigitalLogic);
}; // DigitalInOutHBridge

} // ClearCore namespace

#endif // __HBRIDGE_H__