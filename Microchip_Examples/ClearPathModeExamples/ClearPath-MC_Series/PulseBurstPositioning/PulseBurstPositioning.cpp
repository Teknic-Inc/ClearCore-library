/*
 * Title: PulseBurstPositioning
 *
 * Objective:
 *    This example demonstrates control of the ClearPath-MCPV operational mode
 *    Pulse Burst Positioning.
 *
 * Description:
 *    This example enables a ClearPath motor and executes a repeating pattern of
 *    positional move commands. During operation, various move statuses are
 *    written to the USB serial port.
 *
 * Requirements:
 * 1. A ClearPath motor must be connected to Connector M-0.
 * 2. The connected ClearPath motor must be configured through the MSP software
 *    for Pulse Burst Positioning mode (In MSP select Mode>>Position>>Pulse
 *    Burst Positioning, then hit the OK button).
 * 3. The ClearPath motor must be set to use the HLFB mode "ASG-Position
 *    w/Measured Torque" with a PWM carrier frequency of 482 Hz through the MSP
 *    software (select Advanced>>High Level Feedback [Mode]... then choose
 *    "ASG-Position w/Measured Torque" from the dropdown, make sure that 482 Hz
 *    is selected in the "PWM Carrier Frequency" dropdown, and hit the OK
 *    button).
 * 4. Ensure the Trigger Pulse Time in MSP is set to 20ms. To configure, click
 *    the "Setup..." button found under the "Trigger Pulse" label on the MSP's
 *    main window, fill in the text box, and hit the OK button. Setting this to 
 *    20ms allows trigger pulses to be as long as 60ms, which will accommodate 
 *    our 25ms pulses used later.
 *
 * ** Note: Homing is optional, and not required in this operational mode or in
 *    this example. This example makes its first move in the positive direction,
 *    assuming any homing move occurs in the negative direction.
 *
 * ** Note: Set the Input Resolution in MSP the same as your motor's Positioning
 *    Resolution spec if you'd like the pulses sent by ClearCore to command a
 *    move of the same number of Encoder Counts, a 1:1 ratio.
 *
 * Links:
 * ** ClearCore Documentation: https://teknic-inc.github.io/ClearCore-library/
 * ** ClearCore Manual: https://www.teknic.com/files/downloads/clearcore_user_manual.pdf
 * ** ClearPath Manual (DC Power): https://www.teknic.com/files/downloads/clearpath_user_manual.pdf
 * ** ClearPath Manual (AC Power): https://www.teknic.com/files/downloads/ac_clearpath-mc-sd_manual.pdf
 * ** ClearPath Mode Informational Video: https://www.teknic.com/watch-video/#OpMode10
 *
 * 
 * Copyright (c) 2020 Teknic Inc. This work is free to use, copy and distribute under the terms of
 * the standard MIT permissive software license which can be found at https://opensource.org/licenses/MIT
 */

#include "ClearCore.h"

// Defines the motor's connector as ConnectorM0
#define motor ConnectorM0

// The TRIGGER_PULSE_TIME is set to 25ms to ensure it is within the
// Trigger Pulse Range defined in the MSP software (Default is 20ms, which 
// allows a range of pulses up to 60ms)
#define TRIGGER_PULSE_TIME 25

// Select the baud rate to match the target device.
#define baudRate 9600

// Specify which serial to use: ConnectorUsb, ConnectorCOM0, or ConnectorCOM1.
#define SerialPort ConnectorUsb

// Declares our user-defined helper function, which is used to command moves to
// the motor. The definition/implementation of this function is at the bottom
// of the example.
bool MoveDistance(int32_t pulseNum);

int main() {
    // To command for Pulse Burst Positioning, use the step and direction
    // interface with the acceleration and velocity limits set to their
    // maximum values. The ClearPath will then take the pulses and enforce
    // the motion profile constraints.

    // Sets all motor connectors into step and direction mode.
    MotorMgr.MotorModeSet(MotorManager::MOTOR_ALL,
                          Connector::CPM_MODE_STEP_AND_DIR);

    // Set the motor's HLFB mode to bipolar PWM
    motor.HlfbMode(MotorDriver::HLFB_MODE_HAS_BIPOLAR_PWM);
    // Set the HFLB carrier frequency to 482 Hz
    motor.HlfbCarrier(MotorDriver::HLFB_CARRIER_482_HZ);

    // Sets the maximum velocity and acceleration for each command.
    // The move profile in this mode is determined by ClearPath, so the two
    // lines below should be left as is. Set your desired speed and accel in MSP
    motor.VelMax(INT32_MAX);
    motor.AccelMax(INT32_MAX);

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

    // Enables the motor; homing will begin automatically if homing is enabled
    // in MSP.
    motor.EnableRequest(true);
    SerialPort.SendLine("Motor Enabled");

    // Waits for HLFB to assert (waits for homing to complete if applicable)
    SerialPort.SendLine("Waiting for HLFB...");
    while (motor.HlfbState() != MotorDriver::HLFB_ASSERTED) {
        continue;
    }
    SerialPort.SendLine("Motor Ready");

    while (true) {
        // Move 6400 counts (positive direction) then wait 1000ms
        MoveDistance(6400);
        Delay_ms(1000);
        // Move 19200 counts farther positive, then wait 1000ms
        MoveDistance(19200);
        Delay_ms(1000);

        // Generates a trigger pulse on the enable line so the next move uses the
        // Alt Speed Limit.
        motor.EnableTriggerPulse(1, TRIGGER_PULSE_TIME, true);

        // Move back 12800 counts (negative direction), then wait 1000ms
        MoveDistance(-12800);
        Delay_ms(1000);
        // Move back 6400 counts (negative direction), then wait 1000ms
        MoveDistance(-6400);
        Delay_ms(1000);
        // Move back to the start (negative 6400 pulses), then wait 1000ms
        MoveDistance(-6400);
        Delay_ms(1000);
    }
}

/*------------------------------------------------------------------------------
 * MoveDistance
 *
 *    Command "distance" number of step pulses away from the current position
 *    Prints the move status to the USB serial port
 *    Returns when HLFB asserts (indicating the motor has reached the commanded
 *    position)
 *
 * Parameters:
 *    int distance  - The distance, in step pulses, to move
 *
 * Returns: True/False depending on whether the move was successfully triggered.
 */
bool MoveDistance(int32_t distance) {
    // Check if an alert is currently preventing motion
    if (motor.StatusReg().bit.AlertsPresent) {
        SerialPort.SendLine("Motor status: 'In Alert'. Move Canceled.");
        return false;
    }

    SerialPort.Send("Commanding ");
    SerialPort.Send(distance);
    SerialPort.SendLine(" pulses");

    // Command the move of incremental distance
    motor.Move(distance);

    // Add a short delay to allow HLFB to update
    Delay_ms(2);

    // Waits for HLFB to assert (signaling the move has successfully completed)
    SerialPort.SendLine("Moving.. Waiting for HLFB");
    while (!motor.StepsComplete() || motor.HlfbState() != MotorDriver::HLFB_ASSERTED) {
        continue;
    }

    SerialPort.SendLine("Move Done");
    return true;
}
//------------------------------------------------------------------------------
