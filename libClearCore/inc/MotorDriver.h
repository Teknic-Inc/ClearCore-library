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
    \file MotorDriver.h
    ClearCore stepper/servo motor Connector class
**/
#ifndef __MOTORDRIVER_H__
#define __MOTORDRIVER_H__

#include <stdint.h>
#include "Connector.h"
#include "DigitalIn.h"
#include "PeripheralRoute.h"
#include "ShiftRegister.h"
#include "StatusManager.h"
#include "StepGenerator.h"
#include "SysUtils.h"
#include "SysManager.h"

#define HLFB_CARRIER_LOSS_ERROR_LIMIT (0)
#define HLFB_CARRIER_LOSS_STATE_CHANGE_MS_45_HZ (25)
#define HLFB_CARRIER_LOSS_STATE_CHANGE_MS_482_HZ (4)

namespace ClearCore {

/** The amount of HLFB captures to hold onto.
    This should remain 2, as we only care about 1 capture back due to clipping
    of the last PWM to assert move done/shutdown. **/
#define CPM_HLFB_CAP_HISTORY 2

/**
    Delay before the motor is considered to be enabled after an enable request.
**/
#define CPM_ENABLE_DELAY 1250

/** Default enable trigger pulse width, in milliseconds. **/
#define DEFAULT_TRIGGER_PULSE_WIDTH_MS  25

extern SysManager SysMgr;

/**
    \brief ClearCore motor connector class.

    This class manages a motor connector on the ClearCore board.

    The following connector instances support motor functionality:
    - #ConnectorM0
    - #ConnectorM1
    - #ConnectorM2
    - #ConnectorM3

    For more detailed information on the ClearCore Motor Control and Motion
    Generation systems, check out the \ref MotorDriverMain and \ref MoveGen
    informational pages.

    For more detailed information on the ClearCore Connector interface, check
    out the \ref ConnectorMain informational page.
**/
class MotorDriver : public DigitalIn, public StepGenerator {
    friend class MotorManager;
    friend class StatusManager;
    friend class SysManager;

public:
    /**
        \union PolarityInversionsSD

        \brief A small register with bit and field views. This allows easy
        configuration for steppers that use an inverted signal on one of the
        main input signals. See #PolarityInvertSDEnable(bool invert),
        #PolarityInvertSDDirection(bool invert), and
        #PolarityInvertSDHlfb(bool invert).
    **/
    union PolarityInversionsSD {
        /**
            Broad access to the whole register
        **/
        int16_t reg;

        /**
            Field access to the motor polarity inversions.
        **/
        struct {
            /**
                Invert the sense of the motor's enable input.
            **/
            uint32_t enableInverted : 1;
            /**
                Invert the sense of the motor's direction.
            **/
            uint32_t directionInverted : 1;
            /**
                Invert the sense of the motor's HLFB output.
            **/
            uint32_t hlfbInverted : 1;
        } bit;

        /**
            PolarityInversionsSD default constructor
        **/
        PolarityInversionsSD() {
            reg = 0;
        }

        /**
            PolarityInversionsSD constructor with initial value
        **/
        PolarityInversionsSD(int16_t val) {
            reg = val;
        }
    };

    /**
        Constant returned when HLFB duty cannot be determined.
    **/
    static const int16_t HLFB_DUTY_UNKNOWN = -9999;

    /**
        \brief Return state when HLFB state is requested.

        \see HlfbMode for setting operational mode of HLFB to accommodate
        PWM measurement modes versus static state modes.
    **/
    typedef enum {
        /**
            HLFB is de-asserted.
        **/
        HLFB_DEASSERTED,
        /**
            HLFB is asserted.
        **/
        HLFB_ASSERTED,
        /**
            For HLFB with PWM modes, this would signal that the #HlfbPercent
            function has a new update.
        **/
        HLFB_HAS_MEASUREMENT,
        /**
            Unknown state
        **/
        HLFB_UNKNOWN
    } HlfbStates;

    /**
        \brief Setup the HLFB query to match the ClearPath&trade; Motor's HLFB
        signaling format.
    **/
    typedef enum {
        /**
            Use the current digital state of the HLFB input.

            \note Applicable ClearPath HLFB modes:
            - Servo On
            - In Range
            - All Systems Go (ASG)
            - ASG - Latched
        **/
        HLFB_MODE_STATIC,
        /**
            The HLFB signal may have a 0-100% PWM component. If
            #HLFB_HAS_MEASUREMENT occurred since the last query,
            invoke the #HlfbPercent() function to get the last period measured.

            \note If there is no more PWM signaling the #HlfbState() function
            will return the digital state until another PWM is signaled.

            \note Applicable ClearPath HLFB modes:
            - Speed Output
        **/
        HLFB_MODE_HAS_PWM,
        /**
            The HLFB signal may have a -100% to +100% PWM component. If
            #HLFB_HAS_MEASUREMENT occurred since the last query,
            invoke the #HlfbPercent() function to get the last period measured.

            \note If there is no more PWM signaling the #HlfbState() function
            will return the digital state until another PWM is signaled.

            \note Applicable ClearPath HLFB modes:
            - Measured Torque
            - ASG - w/Measured Torque
            - ASG - Latched - w/Measured Torque
        **/
        HLFB_MODE_HAS_BIPOLAR_PWM
    } HlfbModes;

    /**
        \brief High-Level Feedback (HLFB) carrier frequency: 45 Hz or 482 Hz
    **/
    typedef enum {
        HLFB_CARRIER_45_HZ,
        HLFB_CARRIER_482_HZ
    } HlfbCarrierFrequency;

    /**
        \enum MotorReadyStates

        \brief Motor readiness states.

        \note This is a field in the StatusRegMotor.
    **/
    typedef enum {
        /**
            The motor is not enabled.
        **/
        MOTOR_DISABLED = 0,
        /**
            The motor is in the process of enabling.
        **/
        MOTOR_ENABLING,
        /**
            The motor is enabled and not moving, but HLFB is not asserted.
        **/
        MOTOR_FAULTED,
        /**
            The motor is enabled and HLFB is asserted.
        **/
        MOTOR_READY,
        /**
            The motor is enabled and moving.
        **/
        MOTOR_MOVING
    } MotorReadyStates;

    /**
        \union StatusRegMotor

        \brief Register access for information about the motor's operating
        status. Intended for use in Step and Direction mode.
    **/
    union StatusRegMotor {
        /**
            Broad access to the whole register
        **/
        uint32_t reg;

        /**
            Field access to the motor status register
        **/
        struct {
            /**
                TRUE if the commanded position equals the target position and
                the HLFB is asserted
            **/
            uint32_t AtTargetPosition : 1;
            /**
                TRUE if the commanded velocity is nonzero
            **/
            uint32_t StepsActive : 1;
            /**
                TRUE if the commanded velocity equals the target velocity
            **/
            uint32_t AtTargetVelocity : 1;
            /**
                Direction of the most recent move
                TRUE if the last motion was in the positive direction
                Latches until start of new move
            **/
            uint32_t MoveDirection : 1;
            /**
                TRUE if the HLFB is deasserted AND the enable output is asserted
                When set, any currently executing motion will get canceled
            **/
            uint32_t MotorInFault : 1;
            /**
                TRUE if the motor's enable output is asserted AND the HLFB
                is NOT deasserted.
            **/
            uint32_t Enabled : 1;
            /**
                TRUE if the last commanded move was a positional move.
            **/
            uint32_t PositionalMove : 1;
            /**
                Reflects the state of the HLFB.
            **/
            uint32_t HlfbState : 2;
            /**
                Alerts Present.
            **/
            uint32_t AlertsPresent : 1;
            /**
                Motor ready state
            **/
            MotorReadyStates ReadyState : 3;
            /**
                TRUE if enable trigger pulses are being sent
            **/
            uint32_t Triggering : 1;
            /**
                Reflects the state of the associated positive limit connector.
            **/
            uint32_t InPositiveLimit : 1;
            /**
                Reflects the state of the associated negative limit connector.
            **/
            uint32_t InNegativeLimit : 1;
            /**
                Reflects the state of the associated E-stop sensor connector.
            **/
            uint32_t InEStopSensor : 1;
        } bit;

        /**
            StatusRegMotor default constructor
        **/
        StatusRegMotor() {
            reg = 0;
        }

        /**
            StatusRegMotor constructor with initial value
        **/
        StatusRegMotor(uint32_t val) {
            reg = val;
        }
    };

    /**
        \union AlertRegMotor

        \brief Accumulating register of alerts that have occurred on this motor.
         Intended for use in Step and Direction mode.
    **/
    union AlertRegMotor {
        /**
            Broad access to the whole register
        **/
        uint32_t reg;

        /**
            Field access to the motor alert register
        **/
        struct {
            /**
                TRUE whenever a command is rejected due to an existing alert
                register bit being asserted.
            **/
            uint16_t MotionCanceledInAlert : 1;
            /**
                TRUE whenever executing motion is canceled due to a positive
                limit switch being asserted or whenever a command is rejected
                while in the limit.
            **/
            uint16_t MotionCanceledPositiveLimit : 1;
            /**
                TRUE whenever executing motion is canceled due to a negative
                limit switch being asserted or whenever a command is rejected
                while in the limit.
            **/
            uint16_t MotionCanceledNegativeLimit : 1;
            /**
                TRUE whenever executing motion is canceled due to an E-Stop
                triggered by the specified E-Stop sensor or whenever a command
                is rejected while the E-Stop is asserted.
            **/
            uint16_t MotionCanceledSensorEStop : 1;
            /**
                TRUE whenever executing motion is canceled due to the enable
                output deasserting or whenever a command is rejected while the
                enable output is deasserted.
            **/
            uint16_t MotionCanceledMotorDisabled : 1;
            /**
                TRUE whenever the MotorInFault status is set in the motor status
                register.
            **/
            uint16_t MotorFaulted : 1;
        } bit;

        /**
            AlertRegMotor default constructor
        **/
        AlertRegMotor() {
            reg = 0;
        }

        /**
            AlertRegMotor constructor with initial value
        **/
        AlertRegMotor(uint32_t val) {
            reg = val;
        }
    };

    /**
        Verify that the motor is in a good state before sending a move command.

        \return True if the motor is ready for a move command; false if there
        is a configuration setting or error that would (or should) prevent
        motion.

        \note For use with Step and Direction mode.

        \note Available with software version 1.1 or greater. See
        \ref InstallationInstructions for information on updating library
        versions.
    **/
    bool ValidateMove(bool negDirection);

    /**
        \copydoc StepGenerator::Move()
    **/
    virtual bool Move(int32_t dist,
                      MoveTarget moveTarget = MOVE_TARGET_REL_END_POSN) override;

    /**
        \copydoc StepGenerator::MoveVelocity()
    **/
    virtual bool MoveVelocity(int32_t velocity) override;

    /**
        \brief Sets the filter length in samples. The default is 3 samples.

        Restarts any in progress filtering.

        \code{.cpp}
        // Set M-0's HLFB filter to 5 samples (1ms)
        ConnectorM0.HlfbFilterLength(5);
        \endcode

        \note One sample time is 200 microseconds.

        \param[in] samples The length of the filter to set in samples
    **/
    void HlfbFilterLength(uint16_t samples) {
        DigitalIn::FilterLength(samples);
    }

    /**
        \brief Get connector type.

        \code{.cpp}
        if (ConnectorAlias.Type() == Connector::CPM_TYPE) {
            // This generic connector variable is a MotorDriver connector
        }
        \endcode

        \return The type of the current connector.
    **/
    Connector::ConnectorTypes Type() override {
        return Connector::CPM_TYPE;
    }

    /**
        \brief Get R/W status of the connector.

        \code{.cpp}
        if (ConnectorAlias.IsWritable()) {
            // This generic connector variable is writable
        }
        \endcode

        \return True because the connector is always writable
    **/
    bool IsWritable() override {
        return true;
    }

    /**
        \brief Accessor for the state of the motor's Input A

        \code{.cpp}
        if (ConnectorM0.MotorInAState()) {
            // M-0's ClearPath motor has its input A raised
        }
        \endcode

        \return The current state of Input A

        \note For use with ClearPath-MC.
    **/
    bool MotorInAState();

    /**
        \brief Function to set the state of the motor's Input A

        \code{.cpp}
        // Turn off M-0's ClearPath motor's input A
        ConnectorM0.MotorInAState(false);
        \endcode

        \param[in] value The boolean state to be passed to the input

        \note For use with ClearPath-MC.
    **/
    bool MotorInAState(bool value);

    /**
        \brief Accessor for the state of the motor's Input B

        \code{.cpp}
        if (ConnectorM0.MotorInBState()) {
            // M-0's ClearPath motor has its input B raised
        }
        \endcode

        \return The current state of Input B

        \note For use with ClearPath-MC.
    **/
    bool MotorInBState();

    /**
        \brief Function to set the value of the motor's Input B

        \code{.cpp}
        // Turn off M-0's ClearPath motor's input B
        ConnectorM0.MotorInBState(false);
        \endcode

        \param[in] value The boolean state to be passed to the input

        \note For use with ClearPath-MC.
    **/
    bool MotorInBState(bool value);

    /**
        \brief Accessor for the enable request state of the motor

        \code{.cpp}
        // Save the state of M-0's enable line
        bool enableState = ConnectorM0.EnableRequest();
        \endcode

        \return The current state of the motor's Enable input
    **/
    bool EnableRequest() {
        return m_enableRequestedState;
    }

    /**
        \brief Function to request the motor to enable or disable

        \code{.cpp}
        // Send an enable request to M-0's ClearPath motor
        ConnectorM0.EnableRequest(true);
        \endcode

        \note Any active step and direction moves on this MotorDriver
        connector will be terminated if \a value is false.
        \param[in] value The boolean state to be passed to the motor's Enable
        input
    **/
    void EnableRequest(bool value);

    /**
        \brief Function to set the duty cycle of a PWM signal being sent to the
        motor's Input A

        \code{.cpp}
        // Send a max duty cycle on M-0's input A line
        ConnectorM0.MotorInADuty(255);
        \endcode

        \param[in] duty The PWM duty cycle

        \note For use with ClearPath-MC.
    **/
    bool MotorInADuty(uint8_t duty);

    /**
        \brief Function to set the duty cycle of a PWM signal being sent to the
        motor's Input B

        \code{.cpp}
        // Send a max duty cycle on M-0's input B line
        ConnectorM0.MotorInBDuty(255);
        \endcode

        \param[in] duty The PWM duty cycle

        \note For use with ClearPath-MC.
    **/
    bool MotorInBDuty(uint8_t duty);

    /**
        \brief Sends trigger pulse(s) to a connected ClearPath&trade; motor by
        de-asserting the enable signal for \a time_ms milliseconds.

        The pulse duration (\a time_ms) must be within the Trigger Pulse range
        set in the MSP software. The default trigger pulse will not suffice if
        not changed within MSP. If the pulse duration is too short, the pulse
        will be ignored. If the pulse duration is too long, the motor will
        momentarily disable.

        This function can be used with the following ClearPath&trade; operating
        modes:
        - Move Incremental Distance
        - Pulse Burst Positioning
        - Multiple Sensor Positioning

        \code{.cpp}
        // Send a single trigger pulse of 25ms on M-0's enable that blocks
        // further code execution until the pulse is finished.
        ConnectorM0.EnableTriggerPulse(1, 25, true);
        \endcode

        \note The time specified by \a time_ms is checked at the SysTick rate.
        \note If the motor EnableRequest is false when this function is called,
        the function will return without doing anything.
        \note \a time_ms specifies both the asserted and de-asserted pulse
        widths.
        The total time for sending the pulses is 2 * \a pulseCount * \a time_ms.

        \param[in] pulseCount (optional) The number of times to pulse the enable
        signal. Default: 1.
        \param[in] time_ms (optional) The amount of time to pull the enable
        signal low, in milliseconds. Default: #DEFAULT_TRIGGER_PULSE_WIDTH_MS.
        \param[in] blockUntilDone (optional) If true, block further code
        execution until the pulses have finished being sent. Default: false.
    **/
    void EnableTriggerPulse(uint16_t pulseCount = 1,
                            uint32_t time_ms = DEFAULT_TRIGGER_PULSE_WIDTH_MS,
                            bool blockUntilDone = false);

    /**
        \brief Check to see if enable trigger pulses are actively being sent.

        When using the non-blocking option of EnableTriggerPulse, this function
        allows you to check if the trigger pulse sequence is still being sent.

        \code{.cpp}
        // Begin a train of five pulses on M-0's enable that do not block
        // further code execution until the pulse train is finished.
        ConnectorM0.EnableTriggerPulse(5);
        // Perform other processing here that does not have to wait for the
        // trigger pulses to be sent...

        while (ConnectorM0.EnableTriggerPulseActive()) {
            continue;   // Wait for the trigger pulses to be sent
        }
        // Trigger pulses have all been sent.
        // Now it is safe to change inputs, etc.
        \endcode
    **/
    volatile const bool &EnableTriggerPulseActive() {
        return m_enableTriggerActive;
    }

    /**
        \brief Return the latest HLFB state information.

        \code{.cpp}
        if (ConnectorM0.HlfbState() == MotorDriver::HLFB_ASSERTED) {
            // M-0's HLFB is fully asserted
        }
        \endcode

        \return When configured in #HLFB_MODE_STATIC mode, this returns the
        current HLFB state.

        \return For #HLFB_MODE_HAS_PWM, the #HLFB_ASSERTED or #HLFB_DEASSERTED
        state is returned if no PWM has been detected for 2 milliseconds.
    **/
    volatile const HlfbStates &HlfbState() {
        return m_hlfbState;
    }

    /**
        \brief Returns the percent of Peak Torque/Max Speed based on the current
        HLFB PWM duty cycle

        \code{.cpp}
        // Save the current duty cycle of M-0's HLFB
        float currentDuty = ConnectorM0.HlfbPercent();
        \endcode

        \note This function is only applicable when the #HlfbMode is set to
        #HLFB_MODE_HAS_PWM or #HLFB_MODE_HAS_BIPOLAR_PWM.

        \return Returns #HLFB_DUTY_UNKNOWN number if no update has been
        detected.

        \note The correct HLFB carrier frequency must be set using
        #HlfbCarrier().
    **/
    volatile const float &HlfbPercent() {
        return m_hlfbDuty;
    }

    /**
        \brief Sets operational mode of the HLFB to match up with the HLFB
        configuration of a ClearPath&trade; motor.

        \code{.cpp}
        // Set M-0's HLFB mode to bipolar PWM
        ConnectorM0.HlfbMode(MotorDriver::HLFB_MODE_HAS_BIPOLAR_PWM);
        \endcode

        \param[in] newMode HLFB mode to switch to
    **/
    void HlfbMode(HlfbModes newMode) {
        if (m_hlfbMode == newMode) {
            return;
        }
        m_hlfbMode = newMode;
        m_hlfbCarrierLost = true;
        m_hlfbDuty = HLFB_DUTY_UNKNOWN;
    }

    /**
        \brief Accessor for current HLFB operational mode

        \code{.cpp}
        if (ConnectorM0.HlfbMode() == MotorDriver::HLFB_MODE_STATIC) {
            // M-0's HLFB mode is set to static
        }
        \endcode

        \return Current HLFB mode
    **/
    HlfbModes HlfbMode() {
        return m_hlfbMode;
    }

    /**
        \brief Clear on read accessor for HLFB rising edge detection.

        \code{.cpp}
        if (ConnectorM0.HlfbHasRisen()) {
            // M-0 HLFB rising edge detected
        }
        \endcode

        \return True if the HLFB input state has risen since last poll
    **/
    bool HlfbHasRisen() {
        return DigitalIn::InputRisen();
    }

    /**
        \brief Clear on read accessor for HLFB falling edge detection.

        \code{.cpp}
        if (ConnectorM0.HlfbHasFallen()) {
            // M-0 HLFB falling edge detected
        }
        \endcode

        \return True if the HLFB input state has fallen since last poll
    **/
    bool HlfbHasFallen() {
        return DigitalIn::InputFallen();
    }

    /**
        \brief Set the HLFB carrier frequency signal.

        \code{.cpp}
        // Set motor M-0 to use the higher HFLB carrier frequency (482 Hz)
        ConnectorM0.HlfbCarrier(MotorDriver::HLFB_CARRIER_482_HZ);
        \endcode

        \return True if the HLFB carrier frequency was correctly set

        \note Available with software version 1.1 or greater. See
        \ref InstallationInstructions for information on updating library
        versions.
    **/
    bool HlfbCarrier(HlfbCarrierFrequency freq) {
        switch (freq) {
            case HLFB_CARRIER_45_HZ:
                m_hlfbCarrierLossStateChange_ms =
                    HLFB_CARRIER_LOSS_STATE_CHANGE_MS_45_HZ;
                break;
            case HLFB_CARRIER_482_HZ:
                m_hlfbCarrierLossStateChange_ms =
                    HLFB_CARRIER_LOSS_STATE_CHANGE_MS_482_HZ;
                break;
            default:
                return false;
        }
        m_hlfbCarrierFrequency = freq;
        return true;
    }

    /**
        \brief This motor's HLFB carrier frequency.

        \code{.cpp}
        // Do work that depends on the current HLFB carrier frequency
        switch (ConnectorM0.HlfbCarrier()) {
            case MotorDriver::HLFB_CARRIER_45_HZ:
                // Slow HLFB carrier. Do something.
                break;
            case MotorDriver::HLFB_CARRIER_482_HZ:
            default:
                // Fast HLFB carrier. Do something else.
                break;
        }
        \endcode

        \return The HLFB carrier frequency.

        \note Available with software version 1.1 or greater. See
        \ref InstallationInstructions for information on updating library
        versions.
    **/
    HlfbCarrierFrequency HlfbCarrier() {
        return m_hlfbCarrierFrequency;
    }

    /**
        \brief Check whether the connector is in a hardware fault state.

        \code{.cpp}
        if (ConnectorM0.IsInHwFault()) {
            // M-0 is in a fault state
        }
        \endcode

        \return Connector is in fault
    **/
    bool IsInHwFault() override {
        return (volatile bool &)m_inFault;
    }
    /**
        \brief Accessor for the current Motor Status Register

        \code{.cpp}
        if (ConnectorM0.StatusReg().bit.StepsActive) {
            // M-0 is currently in motion
        }
        \endcode
    **/
    volatile const StatusRegMotor &StatusReg() {
        return m_statusRegMotor;
    }

    /**
        \brief Clear on read accessor for Motor Status Register rising edge
        detection.

        \code{.cpp}
        if (ConnectorM0.StatusRegRisen().bit.Enabled) {
            // M-0's requested enable state went from "off" to "on" since
            // the last status poll.
        }
        \endcode
    **/
    StatusRegMotor StatusRegRisen();

    /**
        \brief Clear on read accessor for Motor Status Register falling edge
        detection.

        \code{.cpp}
        if (ConnectorM0.StatusRegFallen().bit.Enabled) {
            // M-0's requested enable state went from "on" to "off" since
            // the last status poll.
        }
        \endcode
    **/
    StatusRegMotor StatusRegFallen();

    /**
        \brief Accessor for the current Motor Alert Register

        \code{.cpp}
        if (ConnectorM0.AlertReg().bit.MotionCanceledMotorDisabled) {
            // Motion on M-0 was canceled because the motor was disabled
        }
        \endcode
    **/
    volatile const AlertRegMotor &AlertReg() {
        return m_alertRegMotor;
    }

    /**
        \brief Clear the Motor Alert Register. Motion will be prevented if any
        Alert Register bits are set.

        \code{.cpp}
        // Clear any alerts that have accumulated for M-0.
        ConnectorM0.ClearAlerts();
        \endcode
    **/
    void ClearAlerts(uint32_t mask = UINT32_MAX) {
        atomic_and_fetch(&m_alertRegMotor.reg, ~mask);
    }

    /**
        \brief Function to invert the default polarity of the enable signal of
        this motor.

        \code{.cpp}
        if (ConnectorM0.PolarityInvertSDEnable(true)) {
            // M-0's enable signal was successfully inverted
        }
        \endcode

        \note This inversion function is only usable in Step & Direction mode.

        \param[in] invert If true, signal inversion will be turned on
    **/
    bool PolarityInvertSDEnable(bool invert);
    /**
        \brief Function to invert the default polarity of the direction signal
        of this motor.

        \code{.cpp}
        if (ConnectorM0.PolarityInvertSDDirection(true)) {
            // M-0's direction signal was successfully inverted
        }
        \endcode

        \note This inversion function is only usable in Step & Direction mode.

        \param[in] invert If true, signal inversion will be turned on
    **/
    bool PolarityInvertSDDirection(bool invert);
    /**
        \brief Function to invert the default polarity of the HLFB signal of
        this motor.

        \code{.cpp}
        if (ConnectorM0.PolarityInvertSDHlfb(true)) {
            // M-0's HLFB signal was successfully inverted
        }
        \endcode

        \note This inversion function is only usable in Step & Direction mode.

        \param[in] invert If true, signal inversion will be turned on
    **/
    bool PolarityInvertSDHlfb(bool invert);

    /**
        \brief Set the associated brake output connector.

        Brake output mode uses HLFB readings from a connected ClearPath motor
        to energize or de-energize a connected brake. HLFB must be configured
        for either "ASG with Measured Torque" or "Servo On" for the automatic
        brake to function correctly. The motor connectors M-0 through M-3
        can be mapped to any of the ClearCore outputs IO-0 through IO-5, or to
        any attached CCIO-8 output pin.

        \code{.cpp}
        if (ConnectorM0.BrakeOutput(CLEARCORE_PIN_IO2)) {
            // M-0's brake output is now set to IO-2 and enabled.
        }
        \endcode

        \code{.cpp}
        if (ConnectorM0.BrakeOutput(CLEARCORE_PIN_INVALID)) {
            // M-0's brake output is now disabled.
        }
        \endcode

        \param[in] pin The pin representing the connector to use as the brake
        output for this motor. If CLEARCORE_PIN_INVALID is supplied, brake
        output is disabled.

        \return True if the brake output was successfully set and enabled, or
        successfully disabled; false if a pin other than CLEARCORE_PIN_INVALID
        was supplied that isn't a valid digital output pin.

        \note Available with software version 1.1 or greater. See
        \ref InstallationInstructions for information on updating library
        versions.
    **/
    bool BrakeOutput(ClearCorePins pin);

    /**
        \brief Get the associated brake output connector.

        Brake output mode uses HLFB readings from a connected ClearPath motor
        to energize or de-energize a connected brake. HLFB must be configured
        for either "ASG with Measured Torque" or "Servo On" for the automatic
        brake to function correctly. The motor connectors M-0 through M-3
        can be mapped to any of the ClearCore outputs IO-0 through IO-5, or to
        any attached CCIO-8 output pin.

        \code{.cpp}
        if (ConnectorM0.BrakeOutput() == CLEARCORE_PIN_IO2) {
            // M-0's brake output is currently set to IO-2 and enabled.
        }
        \endcode

        \code{.cpp}
        if (ConnectorM0.BrakeOutput() == CLEARCORE_PIN_INVALID) {
            // M-0's brake output is currently disabled.
        }
        \endcode

        \return The pin representing the digital output connector configured to
        be this motor's brake output, or CLEARCORE_PIN_INVALID if no such
        connector has been configured.

        \note Available with software version 1.1 or greater. See
        \ref InstallationInstructions for information on updating library
        versions.
    **/
    ClearCorePins BrakeOutput() {
        return m_brakeOutputPin;
    }

    /**
        \brief Set the associated positive limit switch connector.

        When the input is deasserted (LED off) on the connector associated with
        this limit, all motion in the positive direction will be stopped (i.e.
        use a Normally Closed (NC) switch on this connector).

        \code{.cpp}
        if (ConnectorM0.LimitSwitchPos(CLEARCORE_PIN_IO2)) {
            // M-0's positive limit switch is now set to IO-2 and enabled.
        }
        \endcode

        \code{.cpp}
        if (ConnectorM0.LimitSwitchPos(CLEARCORE_PIN_INVALID)) {
            // M-0's positive limit switch is now disabled.
        }
        \endcode

        \param[in] pin The pin representing the connector to use as the
        positive limit switch for this motor. If CLEARCORE_PIN_INVALID
        is supplied, the positive limit is disabled.

        \return True if the positive limit switch was successfully set and
        enabled, or  successfully disabled; false if a pin other than
        CLEARCORE_PIN_INVALID was supplied that isn't a valid digital
        input pin.

        \note For use with Step and Direction mode.

        \note Available with software version 1.1 or greater. See
        \ref InstallationInstructions for information on updating library
        versions.
    **/
    bool LimitSwitchPos(ClearCorePins pin);

    /**
        \brief Get the associated positive limit switch output connector.

        When the input is deasserted (LED off) on the connector associated with
        this limit, all motion in the positive direction will be stopped (i.e.
        use a Normally Closed (NC) switch on this connector).

        \code{.cpp}
        if (ConnectorM0.LimitSwitchPos() == CLEARCORE_PIN_IO2) {
            // M-0's positive limit switch is currently set to IO-2 and enabled.
        }
        \endcode

        \code{.cpp}
        if (ConnectorM0.LimitSwitchPos() == CLEARCORE_PIN_INVALID) {
            // M-0's positive limit switch is currently disabled.
        }
        \endcode

        \return The pin representing the digital output connector configured to
        be this motor's positive limit, or CLEARCORE_PIN_INVALID if no such
        connector has been configured.

        \note For use with Step and Direction mode.

        \note Available with software version 1.1 or greater. See
        \ref InstallationInstructions for information on updating library
        versions.
    **/
    ClearCorePins LimitSwitchPos() {
        return m_limitSwitchPos;
    }

    /**
        \brief Set the associated negative limit switch connector.

        When the input is deasserted (LED off) on the connector associated with
        this limit, all motion in the negative direction will be stopped (i.e.
        use a Normally Closed (NC) switch on this connector).

        \code{.cpp}
        if (ConnectorM0.LimitSwitchNeg(CLEARCORE_PIN_IO2)) {
            // M-0's negative limit switch is now set to IO-2 and enabled.
        }
        \endcode

        \code{.cpp}
        if (ConnectorM0.LimitSwitchNeg(CLEARCORE_PIN_INVALID)) {
            // M-0's negative limit switch is now disabled.
        }
        \endcode

        \param[in] pin The pin representing the connector to use as the
        negative limit switch for this motor. If CLEARCORE_PIN_INVALID
        is supplied, the negative limit is disabled.

        \return True if the negative limit switch was successfully set and
        enabled, or  successfully disabled; false if a pin other than
        CLEARCORE_PIN_INVALID was supplied that isn't a valid digital
        input pin.

        \note For use with Step and Direction mode.

        \note Available with software version 1.1 or greater. See
        \ref InstallationInstructions for information on updating library
        versions.
    **/
    bool LimitSwitchNeg(ClearCorePins pin);

    /**
        \brief Get the associated negative limit switch output connector.

        When the input is deasserted (LED off) on the connector associated with
        this limit, all motion in the negative direction will be stopped (i.e.
        use a Normally Closed (NC) switch on this connector).

        \code{.cpp}
        if (ConnectorM0.LimitSwitchNeg() == CLEARCORE_PIN_IO2) {
            // M-0's negative limit switch is currently set to IO-2 and enabled.
        }
        \endcode

        \code{.cpp}
        if (ConnectorM0.LimitSwitchNeg() == CLEARCORE_PIN_INVALID) {
            // M-0's negative limit switch is currently disabled.
        }
        \endcode

        \return The pin representing the digital output connector configured to
        be this motor's negative limit, or CLEARCORE_PIN_INVALID if no such
        connector has been configured.

        \note For use with Step and Direction mode.

        \note Available with software version 1.1 or greater. See
        \ref InstallationInstructions for information on updating library
        versions.
    **/
    ClearCorePins LimitSwitchNeg() {
        return m_limitSwitchNeg;
    }

    /**
        \brief Get the connector's operational mode.

        \code{.cpp}
        if (ConnectorM0.Mode() == Connector::CPM_MODE_A_DIRECT_B_PWM) {
            // M-0 is in A-Direct, B-PWM mode
        }
        \endcode

        \return The connector's current operational mode.
    **/
    virtual ConnectorModes Mode() override {
        return Connector::Mode();
    }

    /**
        Set the digital input connector used to control the state of the enable
        signal.

        \code{.cpp}
        if (ConnectorM0.EnableConnector(CLEARCORE_PIN_DI6)) {
            // Connector DI-6 was successfully configured to control the enable
            // signal for motor M-0.
        }
        \endcode

        \param[in] pin The pin representing the digital input connector that
        will control the state of this motor's enable signal.

        \return True if the enable connector was configured successfully.

        \note Available with software version 1.1 or greater. See
        \ref InstallationInstructions for information on updating library
        versions.
    **/
    bool EnableConnector(ClearCorePins pin);

    /**
        Get the digital input connector used to control the state of the enable
        signal.

        \code{.cpp}
        if (ConnectorM0.EnableConnector() == CLEARCORE_PIN_DI8) {
            // Connector DI-8 is currently configured to control the enable
            // signal for motor M-0.
        }
        \endcode

        \return The pin representing the digital input connector configured to
        control this motor's enable signal, or CLEARCORE_PIN_INVALID if no such
        connector has been configured.

        \note Available with software version 1.1 or greater. See
        \ref InstallationInstructions for information on updating library
        versions.
    **/
    ClearCorePins EnableConnector() {
        return m_enableConnector;
    }

    /**
        Set the digital input connector used to control the state of the Input A
        signal.

        \code{.cpp}
        if (ConnectorM0.InputAConnector(CLEARCORE_PIN_DI6)) {
            // Connector DI-6 was successfully configured to control the Input A
            // signal for motor M-0.
        }
        \endcode

        \param[in] pin The pin representing the digital input connector that
        will control the state of this motor's Input A signal.

        \return True if the Input A connector was configured successfully.

        \note For use with ClearPath-MC.

        \note Available with software version 1.1 or greater. See
        \ref InstallationInstructions for information on updating library
        versions.
    **/
    bool InputAConnector(ClearCorePins pin);

    /**
        Get the digital input connector used to control the state of the Input A
        signal.

        \code{.cpp}
        if (ConnectorM0.InputAConnector() == CLEARCORE_PIN_DI8) {
            // Connector DI-8 is currently configured to control the Input A
            // signal for motor M-0.
        }
        \endcode

        \return The pin representing the digital input connector configured to
        control this motor's Input A signal, or CLEARCORE_PIN_INVALID if no such
        connector has been configured.

        \note For use with ClearPath-MC.

        \note Available with software version 1.1 or greater. See
        \ref InstallationInstructions for information on updating library
        versions.
    **/
    ClearCorePins InputAConnector() {
        return m_inputAConnector;
    }

    /**
        Set the digital input connector used to control the state of the Input B
        signal.

        \code{.cpp}
        if (ConnectorM0.InputBConnector(CLEARCORE_PIN_DI6)) {
            // Connector DI-6 was successfully configured to control the Input B
            // signal for motor M-0.
        }
        \endcode

        \param[in] pin The pin representing the digital input connector that
        will control the state of this motor's Input B signal.

        \return True if the Input B connector was configured successfully.

        \note For use with ClearPath-MC.

        \note Available with software version 1.1 or greater. See
        \ref InstallationInstructions for information on updating library
        versions.
    **/
    bool InputBConnector(ClearCorePins pin);

    /**
        Get the digital input connector used to control the state of the Input B
        signal.

        \code{.cpp}
        if (ConnectorM0.InputBConnector() == CLEARCORE_PIN_DI8) {
            // Connector DI-8 is currently configured to control the Input B
            // signal for motor M-0.
        }
        \endcode

        \return The pin representing the digital input connector configured to
        control this motor's Input B signal, or CLEARCORE_PIN_INVALID if no such
        connector has been configured.

        \note For use with ClearPath-MC.

        \note Available with software version 1.1 or greater. See
        \ref InstallationInstructions for information on updating library
        versions.
    **/
    ClearCorePins InputBConnector() {
        return m_inputBConnector;
    }

    /**
        Set the digital input connector used as an E-Stop signal.

        \code {.cppp}
        if (ConnectorM0.EStopConnector(CLEARCORE_PIN_DI6)) {
            // Connector DI-6 was successfully configured to act as an E-Stop
            // input signal for motor M-0.
        }
        \endcode

        \param[in] pin The pin representing the digital input connector that
        will act as an E-Stop signal for this motor.

        \return True if the E-Stop connector was configured successfully.

        \note For use with Step and Direction mode.

        \note Available with software version 1.1 or greater. See
        \ref InstallationInstructions for information on updating library
        versions.
    **/
    bool EStopConnector(ClearCorePins pin);

    /**
        Get the digital input connector used to control the E-Stop input for
        this motor.

        \code{.cpp}
        if (ConnectorM0.EStopConnector() == CLEARCORE_PIN_DI6) {
            // Connector DI-6 is currently configured as an E-Stop input for
            // motor M-0.
        }
        \endcode

        \return The Pin representing the digital input connector configured as
        an E-Stop input for this motor, or CLEARCORE_PIN_INVALID if no such
        connector has been configured.

        \note For use with Step and Direction mode.

        \note Available with software version 1.1 or greater. See
        \ref InstallationInstructions for information on updating library
        versions.
    **/
    ClearCorePins EStopConnector() {
        return m_eStopConnector;
    }

    /**
        Get the HLFB input status.

        \return True if the HLFB state is currently asserted or is actively
        detecting a PWM signal.
    **/
    bool HlfbInputStatus() {
        return m_hlfbState == HLFB_ASSERTED ||
               m_hlfbState == HLFB_HAS_MEASUREMENT;
    }

    /**
        Set the active level for the Enable signal. The default is active low.

        \param[in] activeLevel True for active high; false for active low.
    **/
    void EnableActiveLevel(bool activeLevel) {
        m_polarityInversions.bit.enableInverted = activeLevel;
    }

    /**
        Get the active level for the Enable signal. The default is active low.

        \return True if the enable signal is configured to be active high;
        false if configured to be active low.
    **/
    bool EnableActiveLevel() {
        return m_polarityInversions.bit.enableInverted;
    }

    /**
        Set the active level for the HLFB signal. The default is active high.

        \param[in] activeLevel True for active high; false for active low.
    **/
    void HlfbActiveLevel(bool activeLevel) {
        m_polarityInversions.bit.hlfbInverted = !activeLevel;
    }

    /**
        Get the active level for the HLFB signal. The default is active high.

        \return True if the HLFB signal is configured to be active high;
        false if configured to be active low.
    **/
    bool HlfbActiveLevel() {
        return !m_polarityInversions.bit.hlfbInverted;
    }

#ifndef HIDE_FROM_DOXYGEN

    virtual void OutputDirection() override {
        if (m_mode == Connector::CPM_MODE_STEP_AND_DIR &&
                m_polarityInversions.bit.directionInverted) {
            DATA_OUTPUT_STATE(m_aInfo->gpioPort, m_aDataMask, Direction());
        }
        else {
            DATA_OUTPUT_STATE(m_aInfo->gpioPort, m_aDataMask, !Direction());
        }
    }

    void ClearFaults(uint32_t disableTime_ms, uint32_t waitForHlfbTime_ms = 0) {
        EnableTriggerPulse(1, disableTime_ms);
        m_clearFaultHlfbTimer = waitForHlfbTime_ms;
        m_clearFaultState = CLEAR_FAULT_PULSE_ENABLE;
    }

    bool ClearFaultsActive() {
        return m_clearFaultState != CLEAR_FAULT_IDLE;
    }

    /**
        A helper function to determine whether the pin supplied is a valid
        digital input connector.

        \param[in] pin The pin to validate as a digital input.

        \return True if the pin is a digital input connector; false otherwise.
    **/
    static bool IsValidInputPin(ClearCorePins pin);

    /**
        A helper function to determine whether the pin supplied is a valid
        digital output connector.

        \param[in] pin The pin to validate as a digital output.

        \return True if the pin is a digital output connector; false otherwise.
    **/
    static bool IsValidOutputPin(ClearCorePins pin);

    /**
        \brief Function to set the on time of a PWM signal being sent to the
        motor's Input A

        \param[in] count The PWM on time

        \note For use with ClearPath-MC.
    **/
    bool MotorInACount(uint16_t count);

    /**
        \brief Function to set the on time of a PWM signal being sent to the
        motor's Input B

        \param[in] count The PWM on time

        \note For use with ClearPath-MC.
    **/
    bool MotorInBCount(uint16_t count);

    /**
        \brief Default constructor so this connector can be a global and
        constructed by SysManager.
    **/
    MotorDriver() {};
#endif
protected:
    // Enable bit associated with this CPM
    ShiftRegister::Masks m_enableMask;

    // Mapping to all the pin information
    const PeripheralRoute *m_aInfo;
    const PeripheralRoute *m_bInfo;
    const PeripheralRoute *m_hlfbInfo;

    // Keep some commonly-used bits from the Info structures
    uint32_t m_aDataMask;
    volatile uint32_t *m_aTccBuffer;
    uint32_t m_bDataMask;
    volatile uint32_t *m_bTccBuffer;
    uint32_t m_aTccSyncMask;
    volatile uint32_t *m_aTccSyncReg;
    uint32_t m_bTccSyncMask;
    volatile uint32_t *m_bTccSyncReg;

    // Enable, InA, InB connector pairing
    ClearCorePins m_enableConnector;
    ClearCorePins m_inputAConnector;
    ClearCorePins m_inputBConnector;

    // - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // HLFB State
    // - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Timer/Counter index associated with HLFB input
    uint8_t m_hlfbTcNum;
    // Assigned channel in EVSYS
    uint8_t m_hlfbEvt;
    // HLFB measurement mode
    HlfbModes m_hlfbMode;
    // HLFB width, period raw measurements
    uint16_t m_hlfbWidth[CPM_HLFB_CAP_HISTORY];
    uint16_t m_hlfbPeriod[CPM_HLFB_CAP_HISTORY];
    // HLFB measurement count, used to show lack of PWM
    uint16_t m_hlfbNoPwmSampleCount;
    HlfbCarrierFrequency m_hlfbCarrierFrequency;
    uint32_t m_hlfbCarrierLossStateChange_ms;
    // The last board time (in milliseconds) when PWM carrier was detected
    uint32_t m_hlfbLastCarrierDetectTime;
    // HLFB last duty cycle
    float m_hlfbDuty;
    // HLFB state return
    HlfbStates m_hlfbState;
    bool m_lastHlfbInputValue;
    bool m_hlfbPwmReadingPending;
    uint16_t m_hlfbStateChangeCounter;

    // Inversion mask of actual enable, direction, and HLFB state
    PolarityInversionsSD m_polarityInversions;

    bool m_enableRequestedState;
    bool m_enableTriggerActive;
    uint32_t m_enableTriggerPulseStartMs;
    uint32_t m_enableTriggerPulseCount;
    uint32_t m_enableTriggerPulseLenMs;

    uint16_t m_aDutyCnt;
    uint16_t m_bDutyCnt;

    bool m_inFault;

    StatusRegMotor m_statusRegMotor;
    StatusRegMotor m_statusRegMotorRisen;
    StatusRegMotor m_statusRegMotorFallen;
    StatusRegMotor m_statusRegMotorLast;

    AlertRegMotor m_alertRegMotor;

    /**
        Initialize hardware and/or internal state.
    **/
    void Initialize(ClearCorePins clearCorePin) override;

    /**
        Function to toggle the enable state of the motor.

        Used internally to generate trigger pulses on the enable line.
    **/
    void ToggleEnable();

private:

    enum ClearFaultState {
        CLEAR_FAULT_IDLE,
        CLEAR_FAULT_PULSE_ENABLE,
        CLEAR_FAULT_WAIT_FOR_HLFB
    };

    bool m_initialized;

    // Internal fields used for the IsReady field of the motor status reg
    bool m_isEnabling;
    bool m_isEnabled;
    bool m_hlfbCarrierLost;
    int32_t m_enableCounter;

    // Brake Output Feature
    ClearCorePins m_brakeOutputPin;

    // Limit Switch Feature
    ClearCorePins m_limitSwitchNeg;
    ClearCorePins m_limitSwitchPos;

    // Hardware E-Stop Sensor Feature
    ClearCorePins m_eStopConnector;
    bool m_motionCancellingEStop;

    bool m_shiftRegEnableReq;
    ClearFaultState m_clearFaultState;
    uint32_t m_clearFaultHlfbTimer;

    /**
        Construct, wire in pads and LED Shift register object
    **/
    MotorDriver(enum ShiftRegister::Masks enableMask,
                const PeripheralRoute *aInfo,
                const PeripheralRoute *bInfo,
                const PeripheralRoute *hlfbInfo,
                uint16_t hlfbTc,
                uint16_t hlfbEvt);

    void UpdateADuty();
    void UpdateBDuty();

    /**
          Refresh the Motor on the SysTick time.
    **/
    void RefreshSlow();

    // Poll electrical connector state and update the internal state.
    void Refresh() override;

    /**
        \brief Sets/Clears the fault flag and halts/restores the motor.

        During fault, the HBridge will be disabled, any output will be halted.
        Any attempt to change state will be applied after fault clears.
        Clearing the fault flag will restore the connector to pre-fault state.

        \param[in] isFaulted The desired value for the fault flag.
    **/
    void FaultState(bool isFaulted);

    /**
        \brief Set the motor's operational mode.

        \param[in] newMode The new operational mode to be set.
        The valid modes for this connector type are:
        - Connector#CPM_MODE_STEP_AND_DIR
        - Connector#CPM_MODE_A_DIRECT_B_DIRECT
        - Connector#CPM_MODE_A_DIRECT_B_PWM
        - Connector#CPM_MODE_A_PWM_B_PWM.
        \return Returns false if the mode is invalid or setup fails.
    **/
    bool Mode(Connector::ConnectorModes newMode) override;

    /**
        A helper function to wire in one of the connectors to control a motor
        digital input or reflect the state of a motor digital output.

        \param[in] pin The pin value requested to be assigned to the
        \a memberPin member variable.
        \param[in] memberPin A reference to the member variable that is to be
        set to the value of \a pin.
        \param[in] input True if the \a memberPin requires a digital input pin
        (default) or false if it requires a digital output pin. Used to validate
        the \a pin value supplied.

        \return True if the pin supplied is valid and successfully configured;
        false if the pin supplied was invalid.
    **/
    bool SetConnector(ClearCorePins pin, ClearCorePins &memberPin,
                      bool input = true);

    /**
        A helper function to check if the E-Stop sensor is valid and/or
        currently active (low).
    **/
    bool CheckEStopSensor();

}; // MotorDriver

} // ClearCore namespace

#endif // __MOTORDRIVER_H__
