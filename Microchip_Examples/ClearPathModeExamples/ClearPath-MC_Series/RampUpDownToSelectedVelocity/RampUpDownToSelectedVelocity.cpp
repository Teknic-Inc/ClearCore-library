/*
 * Title: RampUpDownToSelectedVelocity
 *
 * Objective:
 *    This example demonstrates control of the ClearPath-MC operational mode
 *    Ramp Up/Down To Selected Velocity.
 *
 * Description:
 *    This example enables and then moves a ClearPath motor between
 *    preprogrammed velocity selections as defined in the MSP software. During
 *    operation, various move statuses are written to the USB serial port.
 *
 * Requirements:
 * 1. A ClearPath motor must be connected to Connector M-0.
 * 2. The connected ClearPath motor must be configured through the MSP software
 *    for Ramp Up/Down to Selected Velocity mode (In MSP
 *    select Mode>>Velocity>>Ramp Up/Down to Selected Velocity, then hit the OK
 *    button).
 * 3. The ClearPath motor must be set to use the HLFB mode "ASG-Velocity
 *    w/Measured Torque" with a PWM carrier frequency of 482 Hz through the MSP
 *    software (select Advanced>>High Level Feedback [Mode]... then choose
 *    "ASG-Velocity w/Measured Torque" from the dropdown, make sure that 482 Hz
 *    is selected in the "PWM Carrier Frequency" dropdown, and hit the OK
 *    button).
 * 4. The ClearPath must have defined Velocity Selections through the MSP
 *    software (On the main MSP window check the "Velocity Selection Setup
 *    (RPM)" box and fill in the four text boxes labeled "A off B off", "A on B
 *    off", "A off B on", and "A on B on").
 * 5. Ensure the Input A & B filters in MSP are both set to 20ms (In MSP
 *    select Advanced>>Input A, B Filtering... then in the Settings box fill in
 *    the textboxes labeled "Input A Filter Time Constant (msec)" and "Input B
 *    Filter Time Constant (msec)" then hit the OK button).
 *
 * Links:
 * ** ClearCore Documentation: https://teknic-inc.github.io/ClearCore-library/
 * ** ClearCore Manual: https://www.teknic.com/files/downloads/clearcore_user_manual.pdf
 * ** ClearPath Manual (DC Power): https://www.teknic.com/files/downloads/clearpath_user_manual.pdf
 * ** ClearPath Manual (AC Power): https://www.teknic.com/files/downloads/ac_clearpath-mc-sd_manual.pdf
 * ** ClearPath Mode Informational Video: https://www.teknic.com/watch-video/#OpMode5
 *
 * 
 * Copyright (c) 2020 Teknic Inc. This work is free to use, copy and distribute under the terms of
 * the standard MIT permissive software license which can be found at https://opensource.org/licenses/MIT
 */

#include "ClearCore.h"

// The INPUT_A_B_FILTER must match the Input A, B filter setting in
// MSP (Advanced >> Input A, B Filtering...)
#define INPUT_A_B_FILTER 20

// Defines the motor's connector as ConnectorM0
#define motor ConnectorM0

// Select the baud rate to match the target device.
#define baudRate 9600

// Specify which serial to use: ConnectorUsb, ConnectorCOM0, or ConnectorCOM1.
#define SerialPort ConnectorUsb

// Declares our user-defined helper function, which is used to command different
// velocities. The definition/implementation of this function is at the bottom
// of the sketch.
bool RampToVelocitySelection(uint8_t velocityIndex);

int main() {
    // Sets all motor connectors to the correct mode for Ramp Up/Down to
    // Selected Velocity mode.
    MotorMgr.MotorModeSet(MotorManager::MOTOR_ALL,
                          Connector::CPM_MODE_A_DIRECT_B_DIRECT);

    // Set the motor's HLFB mode to bipolar PWM
    motor.HlfbMode(MotorDriver::HLFB_MODE_HAS_BIPOLAR_PWM);
    // Set the HFLB carrier frequency to 482 Hz
    motor.HlfbCarrier(MotorDriver::HLFB_CARRIER_482_HZ);

    // Enforces the state of the motor's A and B inputs before enabling
    // the motor.
    motor.MotorInAState(false);
    motor.MotorInBState(false);

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

    // Waits for HLFB to assert
    SerialPort.SendLine("Waiting for HLFB...");
    while (motor.HlfbState() != MotorDriver::HLFB_ASSERTED) {
        continue;
    }
    SerialPort.SendLine("Motor Ready");

    while (true) {
        // Move to Velocity 1 defined in MSP (Inputs A off, B off).
        // See below for the detailed function definition.
        RampToVelocitySelection(1);

        // Wait 1000ms.
        Delay_ms(1000);
        RampToVelocitySelection(2); // Inputs A on, B off
        Delay_ms(1000);
        RampToVelocitySelection(3); // Inputs A off, B on
        Delay_ms(1000);
        RampToVelocitySelection(4); // Inputs A on, B on
        Delay_ms(1000);

        // Alternatively, if you'd like to control the ClearPath motor's inputs
        // directly using ClearCore inputs consider doing something like this:
        /*
        // Sets ClearPath's InA to DI6's state
        motor.MotorInAState(ConnectorDI6.State());

        // Sets ClearPath's InB to DI7's state
        motor.MotorInBState(ConnectorDI7.State());
        */
    }
}

/*------------------------------------------------------------------------------
 * RampToVelocitySelection
 *
 *    Move to Velocity Selection number velocityIndex (defined in MSP)
 *    Prints the move status to the USB serial port
 *    Returns when HLFB asserts (indicating the motor has reached the target
 *    velocity)
 *
 * Parameters:
 *    int velocityIndex  - The velocity number to command (defined in MSP)
 *
 * Returns: True/False depending on whether the velocity selection was
 *    successfully commanded.
 */
bool RampToVelocitySelection(uint8_t velocityIndex) {
    // Check if an alert is currently preventing motion
    if (motor.StatusReg().bit.AlertsPresent) {
        SerialPort.SendLine("Motor status: 'In Alert'. Move Canceled.");
        return false;
    }

    SerialPort.Send("Moving to Velocity Selection: ");
    SerialPort.Send(velocityIndex);

    switch (velocityIndex) {
        case 1:
            // Sets Input A and B for velocity 1
            motor.MotorInAState(false);
            motor.MotorInBState(false);
            SerialPort.SendLine(" (Inputs A Off/B Off)");
            break;
        case 2:
            // Sets Input A and B for velocity 2
            motor.MotorInAState(true);
            motor.MotorInBState(false);
            SerialPort.SendLine(" (Inputs A On/B Off)");
            break;
        case 3:
            // Sets Input A and B for velocity 3
            motor.MotorInAState(false);
            motor.MotorInBState(true);
            SerialPort.SendLine(" (Inputs A Off/B On)");
            break;
        case 4:
            // Sets Input A and B for velocity 4
            motor.MotorInAState(true);
            motor.MotorInBState(true);
            SerialPort.SendLine(" (Inputs A On/B On)");
            break;
        default:
            // If this case is reached then an incorrect velocityIndex was
            // entered
            return false;
    }

    // Ensures this delay is at least 2ms longer than the Input A, B filter
    // setting in MSP
    Delay_ms(2 + INPUT_A_B_FILTER);

    // Waits for HLFB to assert (signaling the move has successfully reached its
    // target velocity)
    SerialPort.SendLine("Moving... Waiting for HLFB");
    while (motor.HlfbState() != MotorDriver::HLFB_ASSERTED) {
        continue;
    }

    SerialPort.SendLine("Move Done");
    return true;
}
//------------------------------------------------------------------------------
