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

#include "StepGenerator.h"
#include <math.h>
#include <sam.h>
#include "SysTiming.h"

namespace ClearCore {

#define min(a, b) (((a) < (b)) ? (a) : (b))
#define max(a, b) (((a) > (b)) ? (a) : (b))

/*
    This is an internal function to calculate how many pulses to send to each
    motor. It tracks the current command, as well as how many steps have been
    sent, and calculates how many steps to send in the next ISR.
*/

#if !DEBUG_STEP_OUTPUT
void StepGenerator::StepsCalculated() {
    // Process current move state.
    switch (m_moveState) {
        case MS_IDLE: // Idle state, waiting for a command.
            return;
        case MS_START: // Start state, executed only once.
            // Compute move parameters
            m_accelCurrentQx = m_accelLimitQx;
            m_posnTargetQx = static_cast<int64_t>(m_stepsCommanded)
                             << FRACT_BITS;

            if (m_velocityMove) {
                m_velTargetQx = m_velMoveDirChange ? 0 : m_altVelLimitQx;
                if (m_velTargetQx) {
                    // Notify the system of the direction of the issued move
                    // if moving to a non-zero velocity
                    OutputDirection();
                }

                if (m_velCurrentQx == m_velTargetQx) {
                    // Already at the correct velocity
                    m_moveState = MS_CRUISE;
                }
                else if (m_velCurrentQx > m_velTargetQx) {
                    // Decelerate to reach the target velocity
                    m_moveState = MS_DECEL_VEL;
                }
                else {
                    // Accelerate to reach the target velocity
                    m_moveState = MS_ACCEL;
                }
            }
            else {
                // Notify the system of the direction of the issued move
                OutputDirection();

                // If the move profile is a triangle (i.e. doesn't reach
                // VelLimit), set the velocity limit to peak velocity so that
                // trapezoid logic can be used.
                // The maximum triangle move distance =
                //     VelLimit * (AccelSamples + DecelSamples) / 2 = V*V/A
                if (static_cast<int64_t>(m_velLimitQx) * m_velLimitQx /
                        m_accelLimitQx > m_posnTargetQx) {
                    // Multiplication by 2^FRACT_BITS to preserve Q-format
                    int64_t vel64 =
                        static_cast<int64_t>(sqrtf(static_cast<int64_t>(m_stepsCommanded) *
                                                   m_accelLimitQx *
                                                   static_cast<float>(1 << FRACT_BITS)));

                    m_velTargetQx = static_cast<int32_t>(min(vel64, INT32_MAX));
                }
                else {
                    m_velTargetQx = m_velLimitQx;
                }
                m_moveState = MS_ACCEL;
            }

            break;

        case MS_ACCEL: // Phase 1: Ramp up to target speed
            // Execute move
            m_posnCurrentQx += m_velCurrentQx + (m_accelCurrentQx >> 1);
            m_velCurrentQx += m_accelCurrentQx;

            // Check if we reached target velocity or velocity overflow
            if (m_velCurrentQx >= m_velTargetQx || m_velCurrentQx <= 0) {
                // If maximum velocity reached, compute the distance overshoot
                // from exceeding the velocity limit.
                // Dist Over = % of sample time past when the vel was reached
                //             * vel overshoot / 2
                uint32_t overshootQx = m_velCurrentQx - m_velTargetQx;
                uint32_t pctSampleOverQ32 =
                    ((static_cast<uint64_t>(overshootQx)) << 32) /
                    m_accelCurrentQx;
                // Build in the divide by 2
                uint32_t posnAdjQx =
                    (static_cast<uint64_t>(pctSampleOverQ32) * overshootQx) >>
                    33;

                m_velCurrentQx = m_velTargetQx;
                // Adjust position for overshoot.
                // Also subtract off the distance moved in one sample time at
                // target velocity to allow cruise state logic to determine
                // whether we should start decelerating.
                m_posnCurrentQx -= (posnAdjQx + m_velCurrentQx);
                // Calculate the decel point
                uint64_t accelDistQx = (static_cast<uint64_t>(m_velCurrentQx) *
                                        m_velCurrentQx / m_accelCurrentQx) >> 1;
                m_posnDecelQx = m_posnTargetQx - accelDistQx;
                m_moveState = MS_CRUISE;
                // Allow to fall through into cruise in case the decel
                // needs to start immediately
            }
            else {
                break;
            }
        // Fall through

        case MS_CRUISE: // Phase 2: Continue at the current velocity
            m_posnCurrentQx += m_velCurrentQx;

            // Velocity moves don't need to decelerate in the typical way,
            // just stay cruising
            if (m_velocityMove) {
                // If cruising at zero velocity, the move has ended
                if (!m_velCurrentQx) {
                    m_moveState = MS_END;
                }
                break;
            }
            // Check if we reached target decel position or position overflow
            if (m_posnCurrentQx >= m_posnDecelQx || m_posnCurrentQx <= 0) {
                // If the decel position is reached, compute the distance
                // overshoot from where we needed to start ramping.
                // Dist Over = % of sample time past when to decel
                //             * vel change during that time / 2
                uint64_t overshootQx = m_posnCurrentQx - m_posnDecelQx;
                uint32_t pctSampleOverQ32 =
                    (overshootQx << 32) / m_velCurrentQx;
                uint32_t velAdjQx = (static_cast<uint64_t>(pctSampleOverQ32) *
                                     m_accelCurrentQx) >> 32;
                // Build in the divide by 2
                uint32_t posnAdjQx =
                    (static_cast<uint64_t>(pctSampleOverQ32) * velAdjQx) >> 33;

                m_posnCurrentQx -= posnAdjQx;
                m_velCurrentQx -= velAdjQx;
                // Check for done condition: if we overshot target position or
                // decel overshot zero velocity or position overflow
                if ((m_posnCurrentQx >= m_posnTargetQx) ||
                        (m_velCurrentQx <= 0) || (m_posnCurrentQx <= 0)) {
                    // If done, enforce final position.
                    m_accelCurrentQx = 0;
                    m_velCurrentQx = 0;
                    m_posnCurrentQx = m_posnTargetQx;
                    m_moveState = MS_END;
                }
                else {
                    m_moveState = MS_DECEL;
                }
            }
            break;

        case MS_DECEL: // Phase 3: Ramp down to stopped
            // Execute move
            m_posnCurrentQx += m_velCurrentQx - (m_accelCurrentQx >> 1);
            m_velCurrentQx -= m_accelCurrentQx;

            // Check for done condition: if we overshot target position or
            // decel overshot zero velocity or position overflow
            if ((m_posnCurrentQx >= m_posnTargetQx) || (m_velCurrentQx <= 0) ||
                    (m_posnCurrentQx <= 0)) {
                // If done, enforce final position.
                m_accelCurrentQx = 0;
                m_velCurrentQx = 0;
                m_posnCurrentQx = m_posnTargetQx;
                m_moveState = MS_END;
            }
            break;

        case MS_DECEL_VEL: // Velocity moves deceleration state
            m_posnCurrentQx += m_velCurrentQx - (m_accelCurrentQx >> 1);
            m_velCurrentQx -= m_accelCurrentQx;

            // Check if we reached target velocity
            if (m_velCurrentQx <= m_velTargetQx) {
                // If target velocity reached, compute the distance overshoot
                // from exceeding the velocity limit.
                // Dist Over = % of sample time past when the vel was reached
                //             * vel overshoot / 2
                uint32_t overshootQx = m_velTargetQx - m_velCurrentQx;
                uint32_t pctSampleOverQ32 =
                    ((static_cast<uint64_t>(overshootQx)) << 32) /
                    m_accelCurrentQx;
                // Build in the divide by 2
                uint32_t posnAdjQx =
                    (static_cast<uint64_t>(pctSampleOverQ32) * overshootQx) >>
                    33;

                m_velCurrentQx = m_velTargetQx;
                // Adjust position for overshoot.
                m_posnCurrentQx += posnAdjQx;
                // If there's no sign change between moves, head to cruise.
                // Otherwise return to start to begin the new move in the opposite
                // direction of the first move.
                if (m_velMoveDirChange) {
                    m_moveState = MS_START;
                    m_velMoveDirChange = false;
                }
                else {
                    m_moveState = MS_CRUISE;
                }
            }
            break;

        case MS_END: // Clean up after the move completes
        default:
            m_posnCurrentQx = 0;
            m_velCurrentQx = 0;
            m_stepsSent = 0;
            m_stepsPrevious = 0;
            m_moveState = MS_IDLE;
            m_velocityMove = false;
            return;
    }

    // Compute burst value
    m_stepsPrevious = (m_posnCurrentQx >> FRACT_BITS) - m_stepsSent;

    // Update accumulated integer position
    m_stepsSent += m_stepsPrevious;

    // Check move direction and increment absolute position
    m_posnAbsolute += m_direction ? -m_stepsPrevious : m_stepsPrevious;
}
#endif

/*
    Default constructor
*/
StepGenerator::StepGenerator()
    : m_stepsPrevious(0),
      m_stepsPerSampleMax(0),
      m_moveState(MS_IDLE),
      m_direction(false),
      m_posnAbsolute(0),
      m_stepsCommanded(0),
      m_stepsSent(0),
      m_velocityMove(false),
      m_velMoveDirChange(false),
      m_velLimitQx(1),
      m_altVelLimitQx(0),
      m_accelLimitQx(2),
      m_posnCurrentQx(0),
      m_velCurrentQx(0),
      m_accelCurrentQx(0),
      m_posnTargetQx(0),
      m_velTargetQx(0),
      m_posnDecelQx(0) {}

/*
    This function clears the current move and puts the motor in a
    move idle state without disabling it or clearing the position.

    This may cause an abrupt stop.
*/
void StepGenerator::MoveStopAbrupt() {
    // Block the interrupt while changing the command
    __disable_irq();
    m_posnCurrentQx = 0;
    m_velCurrentQx = 0;
    m_stepsSent = 0;
    m_moveState = MS_IDLE;
    m_velocityMove = false;
    m_stepsCommanded = 0;
    m_stepsPrevious = 0;
    __enable_irq();
}

/*
    This function commands a directional move.
    If there is a current move it will NOT be overwritten.

    The function will return true if the move was accepted.
*/
bool StepGenerator::Move(int32_t dist, bool absolute) {
    if (!StepsComplete() || (dist == 0 && !absolute) || m_velocityMove) {
        return false;
    }

    // Block the interrupt while changing the command
    __disable_irq();
    if (absolute) {
        // Invert the absolute position so it's easier to use and think about
        // in code
        int32_t posn = -m_posnAbsolute;
        m_direction = (dist < posn);
        m_stepsCommanded = m_direction ? posn - dist : dist - posn;
    }
    else {
        m_direction = (dist < 0);
        m_stepsCommanded = m_direction ? -dist : dist;
        m_posnCurrentQx = 0;
    }

    m_velCurrentQx = 0;
    m_stepsSent = 0;
    m_moveState = MS_START;
    __enable_irq();

    return true;
}

/*
    This function commands a velocity move.
    If there is a current move, it will be overwritten.
*/
bool StepGenerator::MoveVelocity(int32_t velocity) {
    // Block the interrupt while changing the command
    __disable_irq();
    bool lastDir = m_direction;
    m_direction = (velocity < 0);
    m_velMoveDirChange = velocity && m_velCurrentQx && m_direction != lastDir;
    m_velocityMove = true;

    int32_t velAbsolute = abs(velocity);
    AltVelMax(velAbsolute);
    m_stepsCommanded = INT32_MAX;
    m_posnCurrentQx = 0;
    m_stepsSent = 0;

    m_moveState = MS_START;
    __enable_irq();

    return true;
}

/*
    This function takes the velocity in step pulses/sec
    and sets VelLimitQx in step pulses/sample time.
*/
void StepGenerator::VelMax(int32_t velMax) {
    // Convert from step pulses/sec to step pulses/sample
    int64_t velLim64 =
        (static_cast<int64_t>(velMax) << FRACT_BITS) / SampleRateHz;
    // Enforce the max steps per sample time
    velLim64 =
        min(velLim64, static_cast<int64_t>(m_stepsPerSampleMax) << FRACT_BITS);
    // Ensure we didn't overflow 32-bit int
    velLim64 = min(velLim64, INT32_MAX);
    // Enforce minimum velocity of 1 step pulse/sample
    m_velLimitQx = max(velLim64, 1);
}

/*
    This function takes the velocity in step pulses/sec
    and sets AltVelLimitQx in step pulses/sample time.
*/
void StepGenerator::AltVelMax(int32_t velMax) {
    // Convert from step pulses/sec to step pulses/sample
    int64_t velLim64 =
        (static_cast<int64_t>(velMax) << FRACT_BITS) / SampleRateHz;
    // Enforce the max steps per sample time
    velLim64 =
        min(velLim64, static_cast<int64_t>(m_stepsPerSampleMax) << FRACT_BITS);
    // Ensure we didn't overflow 32-bit int
    m_altVelLimitQx = min(velLim64, INT32_MAX);
}

/*
    This function takes the acceleration in step pulses/sec^2
    and sets AccLimitQx in step pulses/sample^2.
*/
void StepGenerator::AccelMax(int32_t accelMax) {
    // Convert from step pulses/sec/sec to step pulses/sample/sample
    int64_t accelLim64 = ((static_cast<int64_t>(accelMax) << FRACT_BITS) /
                          (SampleRateHz * SampleRateHz));
    // Ensure we didn't overflow 32-bit int
    m_accelLimitQx = min(accelLim64, INT32_MAX);
    // Since accel has to be divided by 2 when calculating position increments,
    // make sure it is even
    m_accelLimitQx &= ~1L;
    // Enforce minimum acceleration of 2 step pulses/sample^2
    if (m_accelLimitQx < 2) {
        m_accelLimitQx = 2;
    }
}

/*
    This function limits the velocity to the maximum that the step output
    can provide.
*/
void StepGenerator::StepsPerSampleMaxSet(uint32_t maxSteps) {
    MoveStopAbrupt();
    m_stepsPerSampleMax = maxSteps;
    // Recalculate maximum velocity limit
    int64_t velLim64 = static_cast<int64_t>(m_stepsPerSampleMax) << FRACT_BITS;
    // Ensure we didn't overflow 32-bit int
    velLim64 = min(velLim64, INT32_MAX);
    // Enforce minimum velocity of 1 step pulse/sample
    velLim64 = max(velLim64, 1);
    // Clip velocity limit if higher than max velocity limit
    m_velLimitQx = min(velLim64, m_velLimitQx);
}

} // ClearCore namespace