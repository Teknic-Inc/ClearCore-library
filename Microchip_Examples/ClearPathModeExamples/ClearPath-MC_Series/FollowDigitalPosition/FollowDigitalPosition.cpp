/*
 * Title: FollowDigitalPosition
 *
 * Objective:
 *    This example demonstrates control of the ClearPath-MCPV operational mode
 *    Follow Digital Position Command, Unipolar PWM Command.
 *
 * Description:
 *    This example enables and then moves a ClearPath motor between various
 *    positions within a range defined in the MSP software based on the state
 *    of an analog input. During operation, various move statuses are written
 *    to the USB serial port.
 *    To achieve better positioning resolution (i.e. more commandable positions),  
 *    consider using the ClearPath operational modes "Pulse Burst Positioning"
 *    or "Move Incremental Distance" instead.
 *
 * Requirements:
 * 1. A ClearPath motor must be connected to Connector M-0.
 * 2. The connected ClearPath motor must be configured through the MSP software
 *    for Follow Digital Position Command, Unipolar PWM Command mode (In MSP
 *    select Mode>>Position>>Follow Digital Position Command, then with
 *    "Unipolar PWM Command" selected hit the OK button).
 * 3. The ClearPath motor must be set to use the HLFB mode "ASG-Position
 *    w/Measured Torque" with a PWM carrier frequency of 482 Hz through the MSP
 *    software (select Advanced>>High Level Feedback [Mode]... then choose
 *    "ASG-Position w/Measured Torque" from the dropdown, make sure that 482 Hz
 *    is selected in the "PWM Carrier Frequency" dropdown, and hit the OK
 *    button).
 * 4. The ClearPath must have defined positions for 0% and 100% PWM (On the
 *    main MSP window check the "Position Range Setup (cnts)" box and fill in
 *    the two text boxes labeled "Posn at 0% PWM" and "Posn at 100% PWM").
 *    Change the "PositionZeroPWM" and "PositionMaxPWM" variables in the example
 *    below to match.
 * 5. Homing must be configured in the MSP software for your mechanical
 *    system (e.g. homing direction, torque limit, etc.). This example does
 *    not use the ClearPath's Input A as a homing sensor, although that may
 *    be configured in this mode through MSP.
 * 6. An analog input source (0-10V) connected to Connector A-9 to control motor 
 *    position.
 * 7. (Optional) An input source, such as a switch, connected to DI-6 to control
 *    the Command Lock or Home Sensor (configured in MSP).
 *
 * Links:
 * ** ClearCore Documentation: https://teknic-inc.github.io/ClearCore-library/
 * ** ClearCore Manual: https://www.teknic.com/files/downloads/clearcore_user_manual.pdf
 * ** ClearPath Manual (DC Power): https://www.teknic.com/files/downloads/clearpath_user_manual.pdf
 * ** ClearPath Manual (AC Power): https://www.teknic.com/files/downloads/ac_clearpath-mc-sd_manual.pdf
 * ** ClearPath Mode Informational Video: https://www.teknic.com/watch-video/#OpMode12
 *
 * 
 * Copyright (c) 2020 Teknic Inc. This work is free to use, copy and distribute under the terms of
 * the standard MIT permissive software license which can be found at https://opensource.org/licenses/MIT
 */

#include "ClearCore.h"

// Defines the motor's connector
#define motor ConnectorM0

// Defines the command lock sensor connector
#define LockSensor ConnectorDI6

// Defines the analog input to control commanded position
#define AnalogSensor ConnectorA9

// Select the baud rate to match the target device.
#define baudRate 9600

// Specify which serial to use: ConnectorUsb, ConnectorCOM0, or ConnectorCOM1.
#define SerialPort ConnectorUsb

// Defines the bounds for our position range. These positions cannot actually be
// commanded, only values inside this range.
double positionZeroPWM = 0;
double positionMaxPWM = 10000;

// Declares our user-defined functions, which are used to pass the Command Lock
// sensor state and send position commands to the motor. The implementations of
// these functions are at the bottom of the sketch.
void LockSensorCallback();
bool CommandPosition(int32_t commandedPosition);

int main() {
    // Set up an analog sensor to control commanded position.
    AnalogSensor.Mode(Connector::INPUT_ANALOG);

    // Sets all motor connectors to the correct mode for Follow Digital
    // Position mode.
    MotorMgr.MotorModeSet(MotorManager::MOTOR_ALL,
                          Connector::CPM_MODE_A_DIRECT_B_PWM);

    // Set the motor's HLFB mode to bipolar PWM
    motor.HlfbMode(MotorDriver::HLFB_MODE_HAS_BIPOLAR_PWM);
    // Set the HFLB carrier frequency to 482 Hz
    motor.HlfbCarrier(MotorDriver::HLFB_CARRIER_482_HZ);

    // This section attaches the interrupt callback to the locking sensor pin,
    // set to trigger on any change of sensor state.
    LockSensor.Mode(Connector::INPUT_DIGITAL);
    LockSensor.InterruptHandlerSet(LockSensorCallback, InputManager::CHANGE, true);
    // Set input A to match the initial state of the sensor.
    motor.MotorInAState(LockSensor.State());

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

    // Enables the motor; homing will begin automatically if applicable
    motor.EnableRequest(true);
    SerialPort.SendLine("Motor Enabled");

    // Waits for HLFB to assert (waits for homing to complete if applicable)
    SerialPort.SendLine("Waiting for HLFB...");
    while (motor.HlfbState() != MotorDriver::HLFB_ASSERTED) {
        continue;
    }
    SerialPort.SendLine("Motor Ready");

    while (true) {
        // Read the voltage on the analog sensor (0-10V).
        float analogVoltage = AnalogSensor.AnalogVoltage();
        // Convert the voltage measured to a position within the valid range.
        int32_t commandedPosition =
            static_cast<int32_t>(round(analogVoltage / 10 * positionMaxPWM));
        CommandPosition(commandedPosition);    // See below for the detailed function definition.
    }
}

/*------------------------------------------------------------------------------
 * CommandPosition
 *
 *    Move to position number commandedPosition (counts in MSP)
 *    Prints the move status to the USB serial port
 *    Returns whether the command has been updated.
 *
 * Parameters:
 *    int commandedPosition  - The position, in counts, to command
 *
 */
bool CommandPosition(int32_t commandedPosition) {
    if (abs(commandedPosition) > abs(positionMaxPWM) ||
        abs(commandedPosition) < abs(positionZeroPWM)) {
        SerialPort.SendLine("Move rejected, invalid position requested");
        return false;
    }

    // Check if an alert is currently preventing motion
    if (motor.StatusReg().bit.AlertsPresent) {
        SerialPort.SendLine("Motor status: 'In Alert'. Move Canceled.");
        return false;
    }

    SerialPort.Send("Moving to position: ");
    SerialPort.SendLine(commandedPosition);

    // Find the scaling factor of our position range mapped to the PWM duty
    // cycle range (255 is the max duty cycle).
    double scaleFactor = 255 / abs(positionMaxPWM - positionZeroPWM);

    // Scale the position command to our duty cycle range.
    uint8_t dutyRequest = abs(commandedPosition - positionZeroPWM) * scaleFactor;

    // Command the move.
    motor.MotorInBDuty(dutyRequest);

    return true;
}
//------------------------------------------------------------------------------

/*------------------------------------------------------------------------------
 * LockSensorCallback
 *
 *    Reads the state of the locking sensor and passes the state to the motor.
 *
 * Parameters:
 *    None
 *
 * Returns: None
 */
void LockSensorCallback() {
    // A 1 ms delay is required in order to pass the correct filtered sensor
    // state.
    Delay_ms(1);
    motor.MotorInAState(LockSensor.State());
}
//------------------------------------------------------------------------------
