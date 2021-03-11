/* 
 * Title: FollowDigitalVelocity
 *
 * Objective:
 *    This example demonstrates control of the ClearPath-MC operational mode
 *    Follow Digital Velocity Command, Unipolar PWM Command.
 *
 * Description:
 *    This example enables a ClearPath motor and executes velocity moves based
 *    on the state of an analog input sensor. During operation, various move
 *    statuses are written to the USB serial port.
 *    Consider using Manual Velocity Control mode instead if you do not wish to 
 *    use an analog sensor to command velocity, if you require greater velocity 
 *    command resolution (i.e. more commandable positions), or if HLFB is needed 
 *    for "move done/at speed" status feedback.
 *
 * Requirements:
 * 1. A ClearPath motor must be connected to Connector M-0.
 * 2. The connected ClearPath motor must be configured through the MSP software
 *    for Follow Digital Velocity Command, Unipolar PWM Command mode (In MSP
 *    select Mode>>Velocity>>Follow Digital Velocity Command, then with
 *    "Unipolar PWM Command" selected hit the OK button).
 * 3. The ClearPath must have a defined Max Speed configured through the MSP
 *    software (On the main MSP window fill in the "Max Speed (RPM)" box with
 *    your desired maximum speed). Ensure the value of maxSpeed below matches
 *    this Max Speed.
 * 4. Ensure the "Invert PWM Input" checkbox found on the MSP's main window is
 *    unchecked.
 * 5. Ensure the Input A filter in MSP is set to 20ms, (In MSP
 *    select Advanced>>Input A, B Filtering... then in the Settings box fill in
 *    the textbox labeled "Input A Filter Time Constant (msec)" then hit the OK
 *    button).
 * 6. An analog input source (0-10V) connected to ConnectorA9 to control
 *    motor velocity.
 *
 *
 * Links:
 * ** ClearCore Documentation: https://teknic-inc.github.io/ClearCore-library/
 * ** ClearCore Manual: https://www.teknic.com/files/downloads/clearcore_user_manual.pdf
 * ** ClearPath Manual (DC Power): https://www.teknic.com/files/downloads/clearpath_user_manual.pdf
 * ** ClearPath Manual (AC Power): https://www.teknic.com/files/downloads/ac_clearpath-mc-sd_manual.pdf
 * ** ClearPath Mode Informational Video: https://www.teknic.com/watch-video/#OpMode8
 *
 * 
 * Copyright (c) 2020 Teknic Inc. This work is free to use, copy and distribute under the terms of
 * the standard MIT permissive software license which can be found at https://opensource.org/licenses/MIT
 */

#include "ClearCore.h"

// Defines the motor's connector as ConnectorM0
#define motor ConnectorM0

// Defines the analog input to control commanded velocity
#define AnalogSensor ConnectorA9

// The INPUT_A_FILTER must match the Input A filter setting in MSP
// (Advanced >> Input A, B Filtering...)
#define INPUT_A_FILTER 20

// Select the baud rate to match the target device.
#define baudRate 9600

// Specify which serial to use: ConnectorUsb, ConnectorCOM0, or ConnectorCOM1.
#define SerialPort ConnectorUsb

// This is the commanded speed limit in RPM (must match the MSP value). This speed
// cannot actually be commanded, so use something slightly higher than your real
// max speed here and in MSP
double maxSpeed = 510;

// Declares our user-defined helper function, which is used to command speed and
// direction. The definition/implementation of this function is at the bottom of
// the sketch.
bool CommandVelocity(int32_t commandedVelocity);

int main() {
    // Set up an analog sensor to control commanded velocity.
    AnalogSensor.Mode(Connector::INPUT_ANALOG);

    // Sets all motor connectors to the correct mode for Follow Digital
    // Velocity, Unipolar PWM mode.
    MotorMgr.MotorModeSet(MotorManager::MOTOR_ALL,
                          Connector::CPM_MODE_A_DIRECT_B_PWM);

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

    while (true) {
        // Read the voltage on the analog sensor (0-10V).
        float analogVoltage = AnalogSensor.AnalogVoltage();
        // Convert the voltage measured to a velocity within the valid range.
        int32_t commandedVelocity =
            static_cast<int32_t>(round(analogVoltage / 10 * maxSpeed));

        // Move at the commanded velocity.
        CommandVelocity(commandedVelocity);    // See below for the detailed function definition.
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
    if (abs(commandedVelocity) >= abs(maxSpeed)) {
        SerialPort.SendLine("Move rejected, requested velocity at or over the limit.");
        return false;
    }

    // Check if an alert is currently preventing motion
    if (motor.StatusReg().bit.AlertsPresent) {
        SerialPort.SendLine("Motor status: 'In Alert'. Move Canceled.");
        return false;
    }

    SerialPort.Send("Commanding velocity: ");
    SerialPort.SendLine(commandedVelocity);

    // Change ClearPath's Input A state to change direction.
    // Note: this section of code was included so this commandVelocity function 
    // could be used to command negative (opposite direction) velocity. However the 
    // analog signal used by this example only commands positive velocities.
    if (commandedVelocity >= 0) {
        motor.MotorInAState(false);
    }
    else {
        motor.MotorInAState(true);
    }

    // Delays to send the correct filtered direction.
    Delay_ms(2 + INPUT_A_FILTER);

    // Find the scaling factor of our velocity range mapped to the PWM duty
    // cycle range (255 is the max duty cycle).
    double scaleFactor = 255 / maxSpeed;

    // Scale the velocity command to our duty cycle range.
    uint8_t dutyRequest = abs(commandedVelocity) * scaleFactor;

    // Command the move.
    motor.MotorInBDuty(dutyRequest);

    return true;
}
//------------------------------------------------------------------------------
