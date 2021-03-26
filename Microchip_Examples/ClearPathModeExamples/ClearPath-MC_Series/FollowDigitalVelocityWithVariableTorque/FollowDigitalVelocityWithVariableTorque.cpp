/*
 * Title: FollowDigitalVelocityWithVariableTorque
 *
 * Objective:
 *    This example demonstrates control of the ClearPath-MC operational mode
 *    Follow Digital Velocity Command, Bipolar PWM Command with Variable Torque.
 *
 * Description:
 *    This example enables a ClearPath motor and executes a repeating pattern of
 *    bidirectional velocity moves and torque limits. During operation, various
 *    move statuses are written to the USB serial port.
 *    The resolution for PWM outputs is 8-bit, meaning only 256 discrete speeds
 *    and torque limits can be commanded. The motor's actual commanded speed
 *    and torque limit may differ slightly from what you input below because
 *    of this.
 *    Consider using Manual Velocity Control mode if greater velocity command
 *    resolution is required, or if HLFB is needed for "move done/at speed"
 *    status feedback.
 *
 * Requirements:
 * 1. A ClearPath motor must be connected to Connector M-0.
 * 2. The connected ClearPath motor must be configured through the MSP software
 *    for Follow Digital Velocity Command, Bipolar PWM Command with Variable
 *    Torque mode (In MSP select Mode>>Velocity>>Follow Digital Velocity
 *    Command, then with "Bipolar PWM Command w/ Variable Torque")
 * 3. The ClearPath must have a defined Max Speed configured through the MSP
 *    software (On the main MSP window fill in the "Max Speed (RPM)" box with
 *    your desired maximum speed). Ensure the value of maxSpeed below matches
 *    this Max Speed.
 * 4. Set the PWM Deadband in MSP to 2.
 * 5. In MSP, ensure the two checkboxes for "Invert Torque PWM Input" and
 *    "Invert Speed PWM Input" are unchecked.
 * 6. A primary Torque Limit and Alternate Torque Limit must be defined using
 *    the Torque Limit setup window through the MSP software (To configure,
 *    click the "Setup..." button found under the "Torque Limit" label. Then
 *    fill in the textbox labeled "Alt Torque Limit (% of max)" and hit the
 *    Apply button). Use only symmetric limits. These limits must match the
 *    "torqueLimit" and "torqueLimitAlternate" variables defined below.
 *
 * Links:
 * ** ClearCore Documentation: https://teknic-inc.github.io/ClearCore-library/
 * ** ClearCore Manual: https://www.teknic.com/files/downloads/clearcore_user_manual.pdf
 * ** ClearPath Manual (DC Power): https://www.teknic.com/files/downloads/clearpath_user_manual.pdf
 * ** ClearPath Manual (AC Power): https://www.teknic.com/files/downloads/ac_clearpath-mc-sd_manual.pdf
 *
 * 
 * Copyright (c) 2020 Teknic Inc. This work is free to use, copy and distribute under the terms of
 * the standard MIT permissive software license which can be found at https://opensource.org/licenses/MIT
 */

#include "ClearCore.h"

// Defines the motor's connector as ConnectorM0
#define motor ConnectorM0

// Select the baud rate to match the target device.
#define baudRate 9600

// Specify which serial to use: ConnectorUsb, ConnectorCOM0, or ConnectorCOM1.
#define SerialPort ConnectorUsb

// This is the commanded speed limit (must match the MSP value). This speed
// cannot actually be commanded, so use something slightly higher than your real
// max speed here and in MSP.
double maxVelocity = 2000;

// Defines the default torque limit and the alternate torque limit
// (must match MSP values)
double torqueLimit = 100.0;
double torqueLimitAlternate = 10.0;

// A PWM deadband of 2% prevents signal jitter from effecting a 0 RPM command
// (must match MSP value)
double pwmDeadBand = 2.0;

// Declares our user-defined helper functions, which are used to command
// velocity and limit torque. The definitions/implementations of these functions
// are at the bottom of the sketch.
bool CommandVelocity(int32_t commandedVelocity);
bool LimitTorque(double limit);

int main() {
    // Sets all motor connectors to the correct mode for Follow Digital
    // Velocity, Bipolar PWM mode.
    MotorMgr.MotorModeSet(MotorManager::MOTOR_ALL,
                          Connector::CPM_MODE_A_PWM_B_PWM);

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

    // Enables the motor
    motor.EnableRequest(true);
    SerialPort.SendLine("Motor Enabled");

    // Waits for 5 seconds to allow the motor to come up to speed
    SerialPort.SendLine("Waiting for motor to reach speed...");
    startTime = Milliseconds();
    while (Milliseconds() - startTime < timeout) {
        continue;
    }
    SerialPort.SendLine("Motor Ready");

    while (true) {
        // Move at +100 RPM (CCW).
        CommandVelocity(100);    // See below for the detailed function definition.
        // Wait 5000ms
        Delay_ms(5000);

        CommandVelocity(300);    // Move at +300 RPM (CCW).
        Delay_ms(5000);

        // Limit the torque to 70%.
        LimitTorque(70);         // See below for the detailed function definition.
        CommandVelocity(-200);   // Move at -200 RPM (CW).
        Delay_ms(5000);

        LimitTorque(15);         // Limit the torque to 15%.
        CommandVelocity(-300);   // Move at -300 RPM (CW).
        Delay_ms(5000);

        LimitTorque(100);        // Increase torque limit to 100%.
        CommandVelocity(100);    // Move at +100 RPM (CCW).
        Delay_ms(5000);
    }
}

/*------------------------------------------------------------------------------
 * CommandVelocity
 *
 *    Command the motor to move using a velocity of commandedVelocity
 *    Prints the move status to the USB serial port
 *
 * Parameters:
 *    int commandedVelocity  - The velocity to command
 *
 * Returns: True/False depending on whether the velocity was successfully
 *    commanded.
 */
bool CommandVelocity(int32_t commandedVelocity) {
    if (abs(commandedVelocity) > maxVelocity) {
        SerialPort.SendLine("Move rejected, invalid velocity requested.");
        return false;
    }

    // Check if an alert is currently preventing motion
    if (motor.StatusReg().bit.AlertsPresent) {
        SerialPort.SendLine("Motor status: 'In Alert'. Move Canceled.");
        return false;
    }

    SerialPort.Send("Commanding velocity: ");
    SerialPort.SendLine(commandedVelocity);

    // If there is a deadband defined, the range of the PWM scale is reduced.
    double rangeUnsigned = 127.5 - (pwmDeadBand / 100 * 255);

    // Find the scaling factor of our velocity range mapped to the PWM duty cycle
    // range (the PWM to the ClearPath is bipolar, so the range starts at a 50%
    // duty cycle).
    double scaleFactor = rangeUnsigned / maxVelocity;

    // Scale the velocity command to our duty cycle range.
    double dutyRequest;
    if (commandedVelocity < 0) {
        dutyRequest = 127.5 - (pwmDeadBand / 100 * 255)
                        + (commandedVelocity * scaleFactor);
    }
    else if (commandedVelocity > 0) {
        dutyRequest = 127.5 + (pwmDeadBand / 100 * 255)
                        + (commandedVelocity * scaleFactor);
    }
    else {
        dutyRequest = 128.0;
    }

    // Command the move.
    motor.MotorInBDuty(dutyRequest);

    SerialPort.SendLine("Velocity Commanded");
    return true;
}
//------------------------------------------------------------------------------

/*------------------------------------------------------------------------------
 * LimitTorque
 *
 *    Command the motor to limit the maximum applied torque to (limit)%
 *    Prints the move status to the USB serial port
 *
 * Parameters:
 *    int limit  - The torque level to command
 *
 * Returns: True/False depending on whether the torque limit was successfully
 * commanded.
 */
bool LimitTorque(double limit) {
    if (limit > torqueLimit || limit < torqueLimitAlternate) {
        SerialPort.SendLine("Torque limiting rejected, invalid torque requested.");
        return false;
    }
    SerialPort.Send("Limit torque to: ");
    SerialPort.Send(limit);
    SerialPort.SendLine("%.");

    // Find the scaling factor of our torque range mapped to the PWM duty cycle
    // range (255 is the max duty cycle).
    double scaleFactor = 255 / (torqueLimit - torqueLimitAlternate);

    // Scale the torque limit command to our duty cycle range.
    uint8_t dutyRequest = (torqueLimit - limit) * scaleFactor;

    // Command the new torque limit.
    motor.MotorInADuty(dutyRequest);

    return true;
}
//------------------------------------------------------------------------------