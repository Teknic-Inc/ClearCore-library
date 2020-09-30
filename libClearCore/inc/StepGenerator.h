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
    \file
    ClearCore Step & Direction motion profile generator

    A StepGenerator is activated by creating an instance of the StepGenerator
    class. There can be several instances of StepGenerator, however, each must
    be attached to different connectors.
**/

#ifndef __STEPGENERATOR_H__
#define __STEPGENERATOR_H__

#include <stdint.h>

namespace ClearCore {

/** All of the motor position, velocity, and acceleration parameters are signed
    and in Q format, with all arithmetic performed in fixed point.
    This defines the Q value - the number of bits that are treated as
    fractional values (15). **/
#define FRACT_BITS 15

/**
    \class StepGenerator
    \brief ClearCore motor motion generator class

    This class manages the generation and communication of movement profiles for
    the MotorDriver connectors.

    For more detailed information on the ClearCore Motor Control and Motion
    Generation systems, check out the \ref MotorDriverMain and \ref MoveGen
    informational pages.
**/
class StepGenerator {
    friend class MotorManager;
    friend class TestIO;

public:
#ifndef HIDE_FROM_DOXYGEN
    StepGenerator();
#endif

    typedef enum {
        MOVE_TARGET_ABSOLUTE,
        MOVE_TARGET_REL_END_POSN,
    } MoveTarget;

    /**
        \brief Issues a positional move for the specified distance.

        \note When making absolute moves, ClearCore tracks the current position
        based on the zero position at program-start. If there is a move in
        progress when a new Move is issued, the target position will be adjusted
        according to the moveTarget parameter, the new acceleration and velocity
        limits will be applied, and the new move is merged seamlessly with the
        previous motion. If you want to make sure that the previous move fully
        completes without being merged with a new command, wait for StepsComplete
        to return true.

        \code{.cpp}
        // Interrupt any on-going move and move to the absolute position 5000
        ConnectorM0.Move(5000, StepGenerator::MOVE_TARGET_ABSOLUTE);
        \endcode

        \param[in] dist The distance of the move in step pulses
        \param[in] moveTarget (optional) Specify the type of movement that
        should be done. Absolute or relative to the end position of the current
        move. Invalid will result in move relative to the end position.
        Default: MOVE_TARGET_REL_END_POSN
    **/
    virtual bool Move(int32_t dist,
                      MoveTarget moveTarget = MOVE_TARGET_REL_END_POSN);

    /**
        \brief Issues a velocity move at the specified velocity.

        \code{.cpp}
        // Perform a velocity move
        ConnectorM0.MoveVelocity(500);
        \endcode

        \note Any existing move will be immediately overwritten with the new
        velocity.

        \param[in] velocity The velocity of the move in step pulses/second.
    **/
    virtual bool MoveVelocity(int32_t velocity);

    /**
        Interrupts the current move; the motor may stop abruptly.

        \code{.cpp}
        // Command an abrupt stop
        ConnectorM0.MoveStopAbrupt();
        \endcode
    **/
    void MoveStopAbrupt();

    /**
        Interrupts the current move; Stops the motor at the maximum of the
        active decel rate or estop decel rate.

        \code{.cpp}
        // Stop the motor at the deceleration rate.
        ConnectorM0.MoveStop();
        \endcode

        \param[in] decelMax The new deceleration limit, in counts/sec^2.
        Passing 0 keeps the previous deceleration rate.
    **/
    void MoveStop();

    /**
        Interrupts the current move; Stops the motor at the maximum of the
        active decel rate or estop decel rate.to hit the specified target.

        \code{.cpp}
        // Stop the motor at the target
        ConnectorM0.MoveStopTarget(15000);
        \endcode

        \param[in] target The position target, in counts.
    **/
    void MoveStopTarget(int32_t target);

    /**
        \brief Sets the absolute commanded position to the given value.

        \code{.cpp}
        // Set the StepGenerator's position reference to -5000
        ConnectorM0.PositionRefSet(-5000);
        \endcode

        \param[in] posn The new position to be set.
    **/
    void PositionRefSet(int32_t posn) {
        m_posnAbsolute = posn;
    }

    /**
        \brief Accessor for the StepGenerator's position reference

        \code{.cpp}
        if (ConnectorM0.PositionRefCommanded() > 5000) {
            // M-0's position reference is above 5000
        }
        \endcode

        \return Returns the absolute commanded position.
    **/
    volatile const int32_t &PositionRefCommanded() {
        return m_posnAbsolute;
    }

    /**
        \brief Accessor for the StepGenerator's momentary velocity

        \code{.cpp}
        if (ConnectorM0.VelocityRefCommanded() > 1000) {
            // M-0's current velocity is above 1000
        }
        \endcode

        \return Returns the momentary commanded velocity.
        /note Velocity changes as the motor accelerates and decelerates, this
        should not be used to track the motion of the motor
    **/
    int32_t VelocityRefCommanded();

    /**
        \brief Sets the maximum velocity in step pulses per second.

        Value will be clipped if out of bounds

        \code{.cpp}
        // Set the StepGenerator's maximum velocity to 1200 step pulses/sec
        ConnectorM0.VelMax(1200);
        \endcode

        \param[in] velMax The new velocity limit
    **/
    void VelMax(uint32_t velMax);

    /**
        \brief Sets the maximum acceleration in step pulses per second^2.

        Value will be clipped if out of bounds

        \code{.cpp}
        // Set the StepGenerator's maximum velocity to 15000 step pulses/sec^2
        ConnectorM0.AccelMax(15000);
        \endcode

        \param[in] accelMax The new acceleration limit
    **/
    void AccelMax(uint32_t accelMax);

    /**
        \brief Sets the maximum deceleration in step pulses per second^2.

        Value will be clipped if out of bounds

        \code{.cpp}
        // Set the StepGenerator's maximum deceleration to 15000 step pulses/sec^2
        ConnectorM0.DecelMax(15000);
        \endcode

        \param[in] decelMax The new deceleration limit
    **/
    void DecelMax(uint32_t decelMax);

    /**
        \brief Sets the maximum deceleration for E-stop Deceleration in
        step pulses per second^2. This is only for MoveStopDecel.

        Value will be clipped if out of bounds

        \code{.cpp}
        // Set the StepGenerator's maximum velocity to 15000 step pulses/sec^2
        ConnectorM0.AccelMax(15000);
        \endcode

        \param[in] decelMax The new deceleration limit
    **/
    void EStopDecelMax(uint32_t decelMax);

    /**
        \brief Function to check if no steps are currently being commanded to
        the motor.

        \code{.cpp}
        if (ConnectorM0.StepsComplete()) {
            // No more steps are being commanded
        }
        \endcode

        \return Returns true if there is no valid current command.
        \note The motor may still be moving after steps are done being sent.
    **/
    bool StepsComplete() {
        return MoveStateGet() == MS_IDLE;
    }

    /**
        \brief Function to check if the commanded move is at the cruising
        velocity - Acceleration portion of movement has finished.

        \code{.cpp}
        if (ConnectorM0.CruiseVelocityReached) {
            // The commanded move is at the cruising velocity
        }
        \endcode

        \return Returns true if there the move is in the cruise state
        \note The motor will still need to decelerate after cruising
    **/
    bool CruiseVelocityReached() {
        return MoveStateGet() == MS_CRUISE;
    }

    /**
        Get the last target (absolute) position.

        \return Target absolute position in counts.
    **/
    volatile const int32_t& TargetPosition() {
        return m_targetPosition;
    }

    /**
        Get the last target velocity.

        \return Target velocity in counts/sec.
    **/
    volatile const int32_t& TargetVelocity() {
        return m_targetVelocity;
    }

protected:
    struct LimitStatus {
        uint32_t InLimit            : 1;    // True if we are in a limit
        uint32_t LimitRampPos       : 1;    // True if we are ramping into the positive limit
        uint32_t LimitRampNeg       : 1;    // True if we are ramping into the negative limit
        uint32_t EnterHWLimit       : 1;    // True when entering HW limits
        uint32_t InPosHWLimit       : 1;    // True if we are in the positive HW limit
        uint32_t InNegHWLimit       : 1;    // True if we are in the negative HW limit
        uint32_t InPosHWLimitLast   : 1;
        uint32_t InNegHWLimitLast   : 1;
        uint32_t HomingMoveActive   : 1;

    public:
        LimitStatus()
            : InLimit(0),
              LimitRampPos(0),
              LimitRampNeg(0),
              EnterHWLimit(0),
              InPosHWLimit(0),
              InNegHWLimit(0),
              InPosHWLimitLast(0),
              InNegHWLimitLast(0),
              HomingMoveActive(0) {}
    };

    typedef enum {
        MS_IDLE,
        MS_START,
        MS_ACCEL,
        MS_CRUISE,
        MS_DECEL,
        MS_DECEL_VEL,
        MS_END,
        MS_CHANGE_DIR,
    } MoveStates;

    uint32_t m_stepsPrevious;
    int32_t m_stepsPerSampleMax;
    int64_t m_stepsPerSampleMaxQx;
    MoveStates m_moveState;
    bool m_direction;
    // True if the last move commanded was a positional move (latched)
    bool m_lastMoveWasPositional;

    int32_t m_posnAbsolute;
    int32_t m_targetPosition; // The last commanded absolute position
    int32_t m_targetVelocity; // The last commanded velocity
    bool m_velocityMove;      // A velocity move is active

    int32_t m_stepsCommanded;
    int32_t m_stepsSent;      // Accumulated integer position

    LimitStatus m_limitInfo;

    bool m_isFollowing;       // This motor is currently following another axis
                              // or encoder.

    volatile const bool &Direction() {
        return m_direction;
    }

    volatile const MoveStates &MoveStateGet() {
        return m_moveState;
    }

    void StepsCalculated();

    uint32_t StepsPrevious() {
        return m_stepsPrevious;
    }

    bool CheckTravelLimits();

	bool LimitSwitchCheck();

    void PosLimitActive(bool isActive) {
        m_limitInfo.InPosHWLimit = isActive;
    }

    void NegLimitActive(bool isActive) {
        m_limitInfo.InNegHWLimit = isActive;
    }


private:
    bool m_eStopDecelMove;    // An e-stop deceleration is active
    bool m_moveDirChange;     // The move is changing direction
    bool m_dirCommanded;      // The direction of the commanded move

    // All of the position, velocity and acceleration parameters are signed and
    // in Q format, with all arithmetic performed in fixed point.
    // FRACT_BITS defines the Q value - the number of bits that are treated as
    // fractional values.

    int32_t m_velLimitQx;     // Velocity limit
    int32_t m_jogVelLimitQx;  // Velocity move Velocity limit
    int32_t m_accelLimitQx;   // Acceleration limit
    int32_t m_decelLimitQx;   // Deceleration limit
    int32_t m_stopDecelLimitQx;// E-Stop Deceleration limit
    int64_t m_posnCurrentQx;  // Current position
    int32_t m_velCurrentQx;   // Current velocity
    int32_t m_accelCurrentQx; // Current acceleration
    int32_t m_decelCurrentQx; // Current deceleration
    int64_t m_posnTargetQx;   // Move length
    int32_t m_velTargetQx;    // Adjusted velocity limit
    int64_t m_posnDecelQx;    // Position to start decelerating
    int32_t m_estopDecelLimitQx; // E-stop deceleration limit

    // Pending velocity and acceleration parameters that shouldn't be applied
    // until a Move function is called again
    int32_t m_velLimitPendingQx;     // Velocity limit
    int32_t m_accelLimitPendingQx;   // Acceleration limit
    int32_t m_decelLimitPendingQx;   // Deceleration limit
    int32_t m_stopDecelLimitPendingQx;// E-Stop Deceleration limit

    virtual void OutputDirection() = 0;
    void StepsPerSampleMaxSet(uint32_t maxSteps);

    void SetJogVelocity(int32_t velMax);

    /**
        \brief Private helper function for Move functions to call that
        updates the internal vel/accel limits to those set by the user.

        Used to latch limits so a move followed immediate by a limit change
        is not used until the next move
    **/
    void UpdatePendingMoveLimits() {
        m_velLimitQx = m_velLimitPendingQx;
        m_accelLimitQx = m_accelLimitPendingQx;
        m_decelLimitQx = m_decelLimitPendingQx ? m_decelLimitPendingQx
                                               : m_accelLimitPendingQx;
        m_stopDecelLimitQx = (m_stopDecelLimitPendingQx > m_decelLimitQx) ?
                              m_stopDecelLimitPendingQx : m_decelLimitQx;
    }
};

} // ClearCore namespace

#endif // __STEPGENERATOR_H__
