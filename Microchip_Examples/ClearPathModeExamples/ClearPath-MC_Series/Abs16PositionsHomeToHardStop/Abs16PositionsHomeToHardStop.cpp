/*
 * Title: 16PositionsHomeToHardStop
 *
 * Objective:
 *    This example demonstrates control of the ClearPath-MCPV operational mode
 *    Move To Absolute Position, 16 Positions (Home to Hard Stop).
 *
 * Description:
 *    This example enables, homes, and then moves a ClearPath motor between
 *    preprogrammed absolute positions as defined in the MSP software. During
 *    operation, various move statuses are written to the USB serial port.
 *
 * Requirements:
 * 1. A ClearPath motor must be connected to Connector M-0.
 * 2. The connected ClearPath motor must be configured through the MSP software
 *    for Move To Absolute Position, 16 Positions (Home to Hard Stop) mode (In
 *    MSP select Mode>>Position>>Move to Absolute Position, then with "16
 *    Positions (Home to Hard Stop)" selected hit the OK button).
 * 3. The ClearPath motor must be set to use the HLFB mode "ASG-Position
 *    w/Measured Torque" with a PWM carrier frequency of 482 Hz through the MSP
 *    software (select Advanced>>High Level Feedback [Mode]... then choose
 *    "ASG-Position w/Measured Torque" from the dropdown, make sure that 482 Hz
 *    is selected in the "PWM Carrier Frequency" dropdown, and hit the OK
 *    button).
 * 4. The ClearPath must have defined Absolute Position Selections through
 *    the MSP software (On the main MSP window fill in the textboxes labeled
 *    1-16 found under "Position Selection Setup (cnts)").
 * 5. Homing must be configured in the MSP software for your mechanical
 *    system (e.g. homing direction, torque limit, etc.). To configure, click
 *    the "Setup..." button found under the "Homing" label on the MSP's main
 *    window.
 *
 * Links:
 * ** ClearCore Documentation: https://teknic-inc.github.io/ClearCore-library/
 * ** ClearCore Manual: https://www.teknic.com/files/downloads/clearcore_user_manual.pdf
 * ** ClearPath Manual (DC Power): https://www.teknic.com/files/downloads/clearpath_user_manual.pdf
 * ** ClearPath Manual (AC Power): https://www.teknic.com/files/downloads/ac_clearpath-mc-sd_manual.pdf
 * ** ClearPath Mode Informational Video: https://www.teknic.com/watch-video/#OpMode14
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

// Declares our user-defined helper function, which is used to send move
// commands. The definition/implementation of this function is at the bottom of
// the sketch.
bool MoveToPosition(uint8_t positionNum);

int main() {
    // Sets all motor connectors to the correct mode for Absolute Position mode
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

    // Enables the motor; homing will begin automatically
    motor.EnableRequest(true);
    SerialPort.SendLine("Motor Enabled");

    // Waits for HLFB to assert (waits for homing to complete if applicable)
    SerialPort.SendLine("Waiting for HLFB...");
    while (motor.HlfbState() != MotorDriver::HLFB_ASSERTED) {
        continue;
    }
    SerialPort.SendLine("Motor Ready");

    while (true) {
        // Move to Position 1 defined in MSP.
        MoveToPosition(1);    // See below for the detailed function definition.
        // Wait 1000ms.
        Delay_ms(1000);
        MoveToPosition(2);
        Delay_ms(1000);
        MoveToPosition(3);
        Delay_ms(1000);
        MoveToPosition(4);
        Delay_ms(1000);

        MoveToPosition(5);
        Delay_ms(1000);
        MoveToPosition(6);
        Delay_ms(1000);
        MoveToPosition(7);
        Delay_ms(1000);
        MoveToPosition(8);
        Delay_ms(1000);

        MoveToPosition(9);
        Delay_ms(1000);
        MoveToPosition(10);
        Delay_ms(1000);
        MoveToPosition(11);
        Delay_ms(1000);
        MoveToPosition(12);
        Delay_ms(1000);

        MoveToPosition(13);
        Delay_ms(1000);
        MoveToPosition(14);
        Delay_ms(1000);
        MoveToPosition(15);
        Delay_ms(1000);
        MoveToPosition(16);
        Delay_ms(1000);
    }
}

/*------------------------------------------------------------------------------
 * MoveToPosition
 *
 *    Move to position number positionNum (defined in MSP)
 *    Prints the move status to the USB serial port
 *    Returns when HLFB asserts (indicating the motor has reached the commanded
 *    position)
 *
 * Parameters:
 *    int positionNum  - The position number to command (defined in MSP)
 *
 * Returns: True/False depending on whether the position was successfully
 *    commanded.
 */
bool MoveToPosition(uint8_t positionNum) {
    // Check if an alert is currently preventing motion
    if (motor.StatusReg().bit.AlertsPresent) {
        SerialPort.SendLine("Motor status: 'In Alert'. Move Canceled.");
        return false;
    }

    SerialPort.Send("Moving to position: ");
    SerialPort.SendLine(positionNum);

    if (positionNum < 17 && positionNum > 0) {
        // Sends pulses on Input B based on positionNum
        for (uint8_t i = 0; i < positionNum; i++) {
            motor.MotorInBState(true);
            Delay_ms(INPUT_A_B_FILTER);
            motor.MotorInBState(false);
            Delay_ms(INPUT_A_B_FILTER);
        }

        // Triggers the command
        motor.MotorInAState(true);
        Delay_ms(INPUT_A_B_FILTER);
        motor.MotorInAState(false);
    }
    else {
        // If an invalid positionNum has been entered, returns a failure
        return false;
    }

    // Ensures this delay is at least 2ms longer than the Input A, B filter
    // setting in MSP
    Delay_ms(2 + INPUT_A_B_FILTER);

    // Waits for HLFB to assert (signaling the move has successfully completed)
    SerialPort.SendLine("Moving... Waiting for HLFB");
    while (motor.HlfbState() != MotorDriver::HLFB_ASSERTED) {
        continue;
    }

    SerialPort.SendLine("Move Done");
    return true;
}
//------------------------------------------------------------------------------
