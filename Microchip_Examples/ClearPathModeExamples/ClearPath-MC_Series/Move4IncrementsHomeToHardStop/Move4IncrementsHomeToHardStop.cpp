/*
 * Title: 4IncrementsHomeToHardStop
 *
 * Objective:
 *    This example demonstrates control of the ClearPath-MCPV operational mode
 *    Move Incremental Distance, 4 Increments (Home to Hard Stop).
 *
 * Description:
 *    This example enables a ClearPath motor and executes a repeating pattern of
 *    incremental moves. During operation, various move statuses are written to
 *    the USB serial port.
 *
 * Requirements:
 * 1. A ClearPath motor must be connected to Connector M-0.
 * 2. The connected ClearPath motor must be configured through the MSP software
 *    for Move Incremental Distance, 4 Increments (Home to Hard Stop) mode (In
 *    MSP select Mode>>Position>>Move Incremental Distance, then with "4
 *    Increments (Home to Hard Stop)" selected hit the OK button).
 * 3. The ClearPath motor must be set to use the HLFB mode "ASG-Position
 *    w/Measured Torque" with a PWM carrier frequency of 482 Hz through the MSP
 *    software (select Advanced>>High Level Feedback [Mode]... then choose
 *    "ASG-Position w/Measured Torque" from the dropdown, make sure that 482 Hz
 *    is selected in the "PWM Carrier Frequency" dropdown, and hit the OK
 *    button).
 * 4. The ClearPath must have defined Position Increments through the MSP
 *    software which match the #define values below (On the main MSP window
 *    check the "Position Increment Setup (cnts)" box and fill in the four text
 *    boxes labeled "A off B off", "A on B off", "A off B on", and "A on B on").
 * 5. Ensure the Trigger Pulse Time in MSP is set to 20ms. To configure, click
 *    the "Setup..." button found under the "Trigger Pulse" label on the MSP's
 *    main window, fill in the text box, and hit the OK button. Setting this to 
 *    20ms allows trigger pulses to be as long as 60ms, which will accommodate 
 *    our 25ms pulses used later.
 * 6. Ensure the Input A & B filters in MSP are both set to 20ms (In MSP
 *    select Advanced>>Input A, B Filtering... then in the Settings box fill in
 *    the textboxes labeled "Input A Filter Time Constant (msec)" and "Input B
 *    Filter Time Constant (msec)" then hit the OK button).
 *
 * ** Note: Homing is optional, and not required in this operational mode or in
 *    this example. This example makes its first move in the positive direction,
 *    assuming any homing move occurs in the negative direction
 *
 * Links:
 * ** ClearCore Documentation: https://teknic-inc.github.io/ClearCore-library/
 * ** ClearCore Manual: https://www.teknic.com/files/downloads/clearcore_user_manual.pdf
 * ** ClearPath Manual (DC Power): https://www.teknic.com/files/downloads/clearpath_user_manual.pdf
 * ** ClearPath Manual (AC Power): https://www.teknic.com/files/downloads/ac_clearpath-mc-sd_manual.pdf
 * ** ClearPath Mode Informational Video: https://www.teknic.com/watch-video/#OpMode4
 *
 * 
 * Copyright (c) 2020 Teknic Inc. This work is free to use, copy and distribute under the terms of
 * the standard MIT permissive software license which can be found at https://opensource.org/licenses/MIT
 */

#include "ClearCore.h"

// Define the motor's connector as ConnectorM0
#define motor ConnectorM0

// The TRIGGER_PULSE_TIME is set to 25ms to ensure it is within the
// Trigger Pulse Range defined in the MSP software (Default is 20-60ms)
#define TRIGGER_PULSE_TIME 25

// The INPUT_A_B_FILTER must match the Input A, B filter setting in
// MSP (Advanced >> Input A, B Filtering...)
#define INPUT_A_B_FILTER 20

// Increments defined below must be set identically to the position increments
// set in MSP
#define POSITION_INCREMENT_1 750   // Inputs A off / B off, 750 counts (CCW)
#define POSITION_INCREMENT_2 -750  // Inputs A on / B off, -750 counts (CW)
#define POSITION_INCREMENT_3 1666  // Inputs A off / B on, 1666 counts (CCW)
#define POSITION_INCREMENT_4 -1666 // Inputs A on / B on, -1666 counts (CW)

// Select the baud rate to match the target device.
#define baudRate 9600

// Specify which serial to use: ConnectorUsb, ConnectorCOM0, or ConnectorCOM1.
#define SerialPort ConnectorUsb

// Declares our user-defined helper function, which is used to send move
// commands. The definition/implementation of this function is at the bottom of
// the sketch.
bool MoveIncrements(uint32_t NumberOfIncrements, int32_t PositionIncrement);

int main() {
    // Sets all motor connectors to the correct mode for Incremental Distance
    // mode.
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
        // Move a distance equal to 1 * POSITION_INCREMENT_1 = 750 counts
        MoveIncrements(1, POSITION_INCREMENT_1);    // See below for the detailed
                                                    // function definition
        // Stay settled for 1 second before moving again.
        Delay_ms(1000);

        // Move a distance equal to 1 * POSITION_INCREMENT_2 = -750 counts
        MoveIncrements(1, POSITION_INCREMENT_2);
        Delay_ms(1000);

        // Note: If another incremental move is triggered in the same direction as
        // an active move before deceleration begins, then the moves will be
        // seamlessly combined into one continuous move.

        // Move a distance equal to 2 * POSITION_INCREMENT_3 = 3332 counts
        MoveIncrements(2, POSITION_INCREMENT_3);
        Delay_ms(1000);

        // Move a distance equal to 2 * POSITION_INCREMENT_4 = -3332 counts
        MoveIncrements(2, POSITION_INCREMENT_4);
        Delay_ms(1000);
    }
}

/*------------------------------------------------------------------------------
 * MoveIncrements
 *
 *    Triggers an incremental move of length numberOfIncrements *
 *    positionIncrement.
 *    Prints the distance and move status to the USB serial port.
 *    Returns when HLFB asserts (indicating move has successfully completed).
 *
 * Parameters:
 *    int numberOfIncrements  - The number of increments to command
 *    int positionIncrement   - The position increment commanded
 *
 * Returns: True/False depending on whether the move was successfully triggered.
 */
bool MoveIncrements(uint32_t numberOfIncrements, int32_t positionIncrement) {
    // Check if an alert is currently preventing motion
    if (motor.StatusReg().bit.AlertsPresent) {
        SerialPort.SendLine("Motor status: 'In Alert'. Move Canceled.");
        return false;
    }

    SerialPort.Send("Moving ");
    SerialPort.Send(numberOfIncrements);
    SerialPort.Send(" * ");

    switch (positionIncrement) {
        case POSITION_INCREMENT_1:
            // Sets Input A and B to position increment 1
            SerialPort.SendLine(POSITION_INCREMENT_1);
            motor.MotorInAState(false);
            motor.MotorInBState(false);
            break;
        case POSITION_INCREMENT_2:
            // Sets Input A and B to position increment 2
            SerialPort.SendLine(POSITION_INCREMENT_2);
            motor.MotorInAState(true);
            motor.MotorInBState(false);
            break;
        case POSITION_INCREMENT_3:
            // Sets Input A and B to position increment 3
            SerialPort.SendLine(POSITION_INCREMENT_3);
            motor.MotorInAState(false);
            motor.MotorInBState(true);
            break;
        case POSITION_INCREMENT_4:
            // Sets Input A and B to position increment 4
            SerialPort.SendLine(POSITION_INCREMENT_4);
            motor.MotorInAState(true);
            motor.MotorInBState(true);
            break;
        default:
            // If this case is reached then an incorrect positionIncrement was
            // entered
            return false;
    }

    // Delays 2ms longer than the Input A, B filter setting in MSP
    Delay_ms(2 + INPUT_A_B_FILTER);

    // Sends trigger pulses to the motor
    motor.EnableTriggerPulse(numberOfIncrements, TRIGGER_PULSE_TIME, true);

    // Waits for HLFB to assert (signaling the move has successfully completed)
    SerialPort.SendLine("Moving... Waiting for HLFB");
    while (motor.HlfbState() != MotorDriver::HLFB_ASSERTED) {
        continue;
    }

    SerialPort.SendLine("Move Done");
    return true;
}
//------------------------------------------------------------------------------