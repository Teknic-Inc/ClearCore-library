/*
 * Title: StepAndDirection
 *
 * Objective:
 *    This example demonstrates control of a third party Step and
 *    Direction motor using a ClearCore motor connector.
 *    This example is NOT intended to be used with ClearPath servos.
 *    There are other examples created specifically for ClearPath.
 *
 * Description:
 *    This example enables a motor then commands a series of repeating
 *    moves to the motor.
 *
 * Requirements:
 * 1. A motor capable of step and direction must be connected to Connector M-0.
 * 2. The motor may optionally be connected to the MotorDriver's HLFB line if
 *    the motor has a "servo on" type feedback feature.
 *
 * Links:
 * ** ClearCore Documentation: https://teknic-inc.github.io/ClearCore-library/
 * ** ClearCore Manual: https://www.teknic.com/files/downloads/clearcore_user_manual.pdf
 *
 * 
 * Copyright (c) 2020 Teknic Inc. This work is free to use, copy and distribute under the terms of
 * the standard MIT permissive software license which can be found at https://opensource.org/licenses/MIT
 */

#include "ClearCore.h"

// Specifies which motor to move.
// Options are: ConnectorM0, ConnectorM1, ConnectorM2, or ConnectorM3.
#define motor ConnectorM0

// Select the baud rate to match the target serial device
#define baudRate 9600

// Specify which serial to use: ConnectorUsb, ConnectorCOM0, or ConnectorCOM1.
#define SerialPort ConnectorUsb

// Define the velocity and acceleration limits to be used for each move
int32_t velocityLimit = 10000; // pulses per sec
int32_t accelerationLimit = 100000; // pulses per sec^2

// Declares our user-defined helper function, which is used to command moves to
// the motor. The definition/implementation of this function is at the  bottom
// of the example.
void MoveDistance(int32_t distance);

int main() {
    // Sets the input clocking rate.
    MotorMgr.MotorInputClocking(MotorManager::CLOCK_RATE_LOW);

    // Sets all motor connectors into step and direction mode.
    MotorMgr.MotorModeSet(MotorManager::MOTOR_ALL,
                          Connector::CPM_MODE_STEP_AND_DIR);

    // These lines may be uncommented to invert the output signals of the
    // Enable, Direction, and HLFB lines. Some motors may have input polarities
    // that are inverted from the ClearCore's polarity.
    //motor.PolarityInvertSDEnable(true);
    //motor.PolarityInvertSDDirection(true);
    //motor.PolarityInvertSDHlfb(true);

    // Sets the maximum velocity for each move
    motor.VelMax(velocityLimit);

    // Set the maximum acceleration for each move
    motor.AccelMax(accelerationLimit);

    // Sets up serial communication and waits up to 5 seconds for a port to open.
    // Serial communication is not required for this example to run.
    SerialPort.Mode(Connector::USB_CDC);
    SerialPort.Speed(baudRate);
    uint32_t timeout = 5000;
    uint32_t startTime = Milliseconds();
    SerialPort.PortOpen();
    while (!SerialPort && Milliseconds() - startTime < timeout) {
        continue;
    }

    // Enables the motor.
    motor.EnableRequest(true);

    // Waits for HLFB to assert. Uncomment these lines if your motor has a
    // "servo on" feature and it is wired to the HLFB line on the connector.
    //SerialPort.SendLine("Waiting for HLFB...");
    //while (motor.HlfbState() != MotorDriver::HLFB_ASSERTED) {
    //    continue;
    //}
    SerialPort.SendLine("Motor Ready");

    while (true) {
        // Move 6400 counts (positive direction), then wait 2000ms
        MoveDistance(6400);
        Delay_ms(2000);
        // Move 19200 counts farther positive, then wait 2000ms
        MoveDistance(19200);
        Delay_ms(2000);
        // Move back 12800 counts (negative direction), then wait 2000ms
        MoveDistance(-12800);
        Delay_ms(2000);
        // Move back 6400 counts (negative direction), then wait 2000ms
        MoveDistance(-6400);
        Delay_ms(2000);
        // Move back to the start (negative 6400 pulses), then wait 2000ms
        MoveDistance(-6400);
        Delay_ms(2000);
    }
}

/*------------------------------------------------------------------------------
 * MoveDistance
 *
 *    Command "distance" number of step pulses away from the current position
 *    Prints the move status to the USB serial port
 *    Returns when step pulses have completed
 *
 * Parameters:
 *    int distance  - The distance, in step pulses, to move
 *
 * Returns: None
 */
void MoveDistance(int32_t distance) {
    SerialPort.Send("Moving distance: ");
    SerialPort.SendLine(distance);

    // Command the move of incremental distance
    motor.Move(distance);

    // Waits for all step pulses to output
    SerialPort.SendLine("Moving... Waiting for the step output to finish...");
    while (!motor.StepsComplete()) {
        continue;
    }

    SerialPort.SendLine("Steps Complete");
}
//------------------------------------------------------------------------------
