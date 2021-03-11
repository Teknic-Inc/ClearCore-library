/*
 * Title: 2PositionsHomeToSwitch
 *
 * Objective:
 *    This example demonstrates control of the ClearPath-MC operational mode
 *    Move To Absolute Position, 2 Positions (Home to Switch).
 *
 * Description:
 *    This example enables, homes, and then moves a ClearPath motor between
 *    preprogrammed absolute positions as defined in the MSP software. During
 *    operation, various move statuses are written to the USB serial port.
 *
 * Requirements:
 * 1. A ClearPath motor must be connected to Connector M-0.
 * 2. The connected ClearPath motor must be configured through the MSP software
 *    for Move To Absolute Position, 2 Positions (Home to Switch) mode (In MSP
 *    select Mode>>Position>>Move to Absolute Position, then with "2 Positions
 *    (Home to Switch)" selected hit the OK button).
 * 3. Homing must be configured in the MSP software for your mechanical system
 *    (e.g. homing direction, switch polarity, etc.). To configure, click the
 *    "Setup..." button found under the "Homing" label on the MSP's main window.
 * 4. The ClearPath motor must be set to use the HLFB mode "ASG-Position
 *    w/Measured Torque" with a PWM carrier frequency of 482 Hz through the MSP
 *    software (select Advanced>>High Level Feedback [Mode]... then choose
 *    "ASG-Position w/Measured Torque" from the dropdown, make sure that 482 Hz
 *    is selected in the "PWM Carrier Frequency" dropdown, and hit the OK
 *    button).
 * 5. Wire the homing sensor to Connector DI-6.
 * 6. The ClearPath must have defined Absolute Position Selections set up in the
 *    MSP software (On the main MSP window check the "Position Selection Setup
 *    (cnts)" box and fill in the two text boxes labeled "A off" and "A on").
 * 7. Ensure the Input A & B filters in MSP are both set to 20ms (In MSP
 *    select Advanced>>Input A, B Filtering... then in the Settings box fill in
 *    the textboxes labeled "Input A Filter Time Constant (msec)" and "Input B
 *    Filter Time Constant (msec)" then hit the OK button).
 *
 * Links:
 * ** ClearCore Documentation: https://teknic-inc.github.io/ClearCore-library/
 * ** ClearCore Manual: https://www.teknic.com/files/downloads/clearcore_user_manual.pdf
 * ** ClearPath Manual (DC Power): https://www.teknic.com/files/downloads/clearpath_user_manual.pdf
 * ** ClearPath Manual (AC Power): https://www.teknic.com/files/downloads/ac_clearpath-mc-sd_manual.pdf
 * ** ClearPath Mode Informational Video: https://www.teknic.com/watch-video/#OpMode2
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

// Specifies the home sensor connector
#define HomingSensor ConnectorDI6

// Select the baud rate to match the target device.
#define baudRate 9600

// Specify which serial to use: ConnectorUsb, ConnectorCOM0, or ConnectorCOM1.
#define SerialPort ConnectorUsb

// Declares our user-defined functions, which are used to pass the state of the
// home sensor to the motor, and to send move commands. The
// definitions/implementations of these functions are at the bottom of the sketch.
void HomingSensorCallback();
bool MoveToPosition(uint8_t positionNum);

int main() {
    // This section attaches the interrupt callback to the homing sensor pin,
    // set to trigger on any change of sensor state.
    HomingSensor.Mode(Connector::INPUT_DIGITAL);
    HomingSensor.InterruptHandlerSet(HomingSensorCallback, InputManager::CHANGE);

    // Sets all motor connectors to the correct mode for Absolute Position mode
    MotorMgr.MotorModeSet(MotorManager::MOTOR_ALL,
                          Connector::CPM_MODE_A_DIRECT_B_DIRECT);

    // Set the motor's HLFB mode to bipolar PWM
    motor.HlfbMode(MotorDriver::HLFB_MODE_HAS_BIPOLAR_PWM);
    // Set the HFLB carrier frequency to 482 Hz
    motor.HlfbCarrier(MotorDriver::HLFB_CARRIER_482_HZ);

    // Enforces the state of the motor's Input A before enabling the motor
    motor.MotorInAState(false);
    // Set input B to match the initial state of the sensor.
    motor.MotorInBState(HomingSensor.State());

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
 * Returns: True/False depending on whether a valid position was
 *    successfully commanded and reached.
 */
bool MoveToPosition(uint8_t positionNum) {
    // Check if an alert is currently preventing motion
    if (motor.StatusReg().bit.AlertsPresent) {
        SerialPort.SendLine("Motor status: 'In Alert'. Move Canceled.");
        return false;
    }

    SerialPort.Send("Moving to position: ");
    SerialPort.Send(positionNum);

    switch (positionNum) {
        case 1:
            // Sets Input A "off" for position 1
            motor.MotorInAState(false);
            SerialPort.SendLine(" (Input A Off)");
            break;
        case 2:
            // Sets Input A "on" for position 2
            motor.MotorInAState(true);
            SerialPort.SendLine(" (Input A On)");
            break;
        default:
            // If this case is reached then an incorrect positionNum was entered
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

/*------------------------------------------------------------------------------
 * HomingSensorCallback
 *
 *    Reads the state of the homing sensor and passes the state to the motor.
 */
void HomingSensorCallback() {
    // A 1 ms delay is required in order to pass the correct filtered sensor
    // state.
    Delay_ms(1);
    motor.MotorInBState(HomingSensor.State());
}
//------------------------------------------------------------------------------
