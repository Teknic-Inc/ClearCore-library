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

void StepGenerator::StepsCalculated() {
    
    // Perform setup for a newly issued move.
    // This is handled separately from the main state machine to determine
    // determine the proper entry state and begin executing without delaying
    // until the next sample.
    if (m_moveState == MS_START) {
        // Compute move parameters
        m_accelCurrentQx = m_accelLimitQx;
        m_posnTargetQx = static_cast<int64_t>(m_stepsCommanded)
        << FRACT_BITS;

        if (m_velocityMove) {
            m_velTargetQx = m_moveDirChange ? 0 : m_altVelLimitQx;
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
                m_accelLimitQx * static_cast<float>(1 << FRACT_BITS)));

                m_velTargetQx = static_cast<int32_t>(min(vel64, INT32_MAX));
            }
            else {
                m_velTargetQx = m_velLimitQx;
            }
            if (m_moveDirChange){
                m_moveState = MS_DECEL_VEL;
                m_velTargetQx = 0;
            } else {
                m_moveState = MS_ACCEL;
            }
        }
    }

    // Process the current move state.
    switch (m_moveState) {
        case MS_IDLE: // Idle state, waiting for a command.
            return;
        case MS_START: // Start state, this case was handled above 
            break;

        case MS_ACCEL: // Ramp up to target speed
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

        case MS_CRUISE: // Continue at the current velocity
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

        case MS_DECEL: // Ramp down to stopped
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

        case MS_DECEL_VEL: // Velocity move deceleration state
            // When decreasing velocity, target a new velocity, not a position
            // During decel, we still need to accumulate the steps that we are
            // taking.
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

                // Velocity may be slightly off of the target velocity due to
                // discrete sampling. Force velocity to snap to the target
                m_velCurrentQx = m_velTargetQx;
                // Adjust position for overshoot.
                m_posnCurrentQx += posnAdjQx;
                // If there's no sign change between moves, head to cruise.
                // Otherwise return to start to begin the new move in the opposite
                // direction of the first move.
                if (m_moveDirChange) {
                    // When a direction change occurs, the goal is to slow down
                    // as quickly as possible (accel limit). During this slow
                    // down period, we are moving in the direction that we were
                    // previously and steps are accumulating in that direction.
                    // In order to still meet the position target, we need to
                    // add these (wrong direction) steps to the  user entered
                    // commanded steps.

                    // We need to flop directions, so do so.
                    m_direction = !m_direction;
                    // We went past where the command was issued, we have to
                    // now go the original distance plus how far we went slowing
                    if (m_moveOvershoot) {
                        m_stepsCommanded = m_stepsSent - m_stepsCommanded;
                        } else {
                        m_stepsCommanded += m_stepsSent;
                    }
                    
                    // Zero previous move
                    m_stepsSent = 0;
                    m_posnCurrentQx = m_posnCurrentQx & ~(UINT64_MAX << FRACT_BITS);

                    m_moveState = MS_START;
                    m_moveDirChange = false;
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
            m_stepsCommanded = 0;
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
      m_moveDirChange(false),
      m_moveOvershoot(false),
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

    The function will return true if the move was accepted.
*/
bool StepGenerator::Move(int32_t dist, bool absolute, bool immediate) {
    // Check if the move overwrites a current move and it shouldn't
    if (!immediate && !StepsComplete() ) {
        return false;
    }

    // Make relative moves be based off of current position during a velocity
    // move
    if (m_velocityMove){
        m_stepsCommanded = 0;
    }

    // Block the interrupt while changing the command
    __disable_irq();
    bool lastDir = m_direction;
    bool newDir;
    if (absolute) {
        m_stepsCommanded = dist - m_posnAbsolute;
    }
    else {
        //m_stepsCommanded += (newDir ? -dist : dist);
        // Since the steps scale is relative to start of move to prevent 
        // overflow, the scale shifts by the number of steps taken
        // So account for this, the current steps should be taken off of the
        // previous commanded amount, then the new command should be added
        // The steps send are in the direction of the commanded steps, subtract
        // that first. Steps taken is always less than commanded, result (+)
        m_stepsCommanded -= m_stepsSent;
        // Convert magnitude + direction format to signed int
        m_stepsCommanded = m_direction ? -m_stepsCommanded : m_stepsCommanded ;
        // Now stepsCommanded and distance are signed and in the global 
        // direction. Add them
        m_stepsCommanded += dist;
        // Steps commanded and dir will be calculated later.
    }

    // Zero out the steps and integer portion of current position to
    // reduce chance of overflow
    m_stepsSent = 0;
    // Zero the integer portion of the current position. We want to keep
    // partial steps so movement is smooth. 
    m_posnCurrentQx = m_posnCurrentQx & ~(UINT64_MAX << FRACT_BITS);

    // Determine the direction of the movements.
    newDir = m_stepsCommanded < 0;
    
    // Steps commanded now needs to be a positive value. 
    m_stepsCommanded = abs(m_stepsCommanded);

    // Determine if we are moving too quickly to stop in time
    // Use the current velocity (+), target velocity of 0, and a of our limit
    // Use constant acceleration eqns: V=Vo+a*t and x=Vo*t+1/2*a*t
    // Start with time to accelerate to 0 velocity
    // 0=Vo+a*t
    // t = -Vo/a
    // Plug in to find distance to stop
    // x = Vo*(-Vo/a) + 1/2*a*(-Vo/a)^2
    // x = -(Vo)^2/a + 1/2*Vo^2/a
    // x = -(Vo)^2/a
    // a is negative since we want to slow down, cancels negative on top
    int32_t distToStop = (static_cast<int64_t>(m_velCurrentQx)*m_velCurrentQx/
                         m_accelLimitQx) >> FRACT_BITS;
    // The distance to stop is how many steps it will take to slow to 0 velocity
    // If the number of commanded steps is less than that, we cannot stop in 
    // time and must overshoot and come back. This only applies if the commanded
    // steps are in the direction that we are currently going. 
    m_moveOvershoot = (m_stepsCommanded < distToStop) && (newDir == lastDir);

    // Determine if there is a direction change. If the movement has stopped
    // (vel == 0), then a direction change can safely happen. Otherwise, compare
    // current and previous direction to see if a change happened. A direction
    // change is also required if we go past out target and overshoot.
    m_moveDirChange = m_velCurrentQx && ((newDir != lastDir) || m_moveOvershoot);

    // If there was a direction change, we need to keep going in the same 
    // direction to safely come to a stop, then move the other way.
    m_direction = m_moveDirChange ? lastDir : newDir;

    m_velocityMove = false;
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
    bool newDir = (velocity < 0);
    m_moveDirChange = velocity && m_velCurrentQx && newDir != lastDir;

    // If there was a direction change, we need to keep going in the same
    // direction to safely come to a stop, then move the other way.
    m_direction = m_moveDirChange ? lastDir : newDir;

    m_velocityMove = true;

    int32_t velAbsolute = abs(velocity);
    AltVelMax(velAbsolute);
    m_stepsCommanded = INT32_MAX;
    m_posnCurrentQx &= ~(UINT64_MAX << FRACT_BITS);
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

int32_t StepGenerator::VelocityRefCommanded() {
    // Reverse the calculation in AltVelMax to get the velocity in the same
    // units that the user put in. Add half a decimal for rounding.
    int32_t velTemp = ((static_cast<int64_t>(m_velCurrentQx)*SampleRateHz +
    (1 << (FRACT_BITS-1))) >> FRACT_BITS);
    return m_direction ? -velTemp : velTemp;
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