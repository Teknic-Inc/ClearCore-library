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
        status.
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
        Verify that the motor is in a good state before sending a move command.

        \return True if the motor is ready for a move command; false if there
        is a configuration setting or error that would (or should) prevent motion.
    **/
    bool ValidateMove();

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
    **/
    bool MotorInAState();

    /**
        \brief Function to set the state of the motor's Input A

        \code{.cpp}
        // Turn off M-0's ClearPath motor's input A
        ConnectorM0.MotorInAState(false);
        \endcode

        \param[in] value The boolean state to be passed to the input
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
    **/
    bool MotorInBState();

    /**
        \brief Function to set the value of the motor's Input B

        \code{.cpp}
        // Turn off M-0's ClearPath motor's input B
        ConnectorM0.MotorInBState(false);
        \endcode

        \param[in] value The boolean state to be passed to the input
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
    bool m_hlfbPwmReadingPending;
    uint16_t m_hlfbStateChangeCounter;

    // Inversion mask of actual enable, direction, and HLFB state
    PolarityInversionsSD m_polarityInversions;

    bool m_enableRequestedState;
    bool m_enableTriggerActive;
    uint32_t m_enableTriggerPulseStartMs;
    uint16_t m_enableTriggerPulseCount;
    uint32_t m_enableTriggerPulseLenMs;

    uint16_t m_aDutyCnt;
    uint16_t m_bDutyCnt;

    bool m_inFault;

    StatusRegMotor m_statusRegMotor;
    StatusRegMotor m_statusRegMotorRisen;
    StatusRegMotor m_statusRegMotorFallen;
    StatusRegMotor m_statusRegMotorLast;

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
    bool m_initialized;

    // Internal fields used for the IsReady field of the motor status reg
    bool m_isEnabling;
    bool m_isEnabled;
    bool m_hlfbCarrierLost;
    int32_t m_enableCounter;
    bool m_shiftRegEnableReq;

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

}; // MotorDriver

} // ClearCore namespace

#endif // __MOTORDRIVER_H__
