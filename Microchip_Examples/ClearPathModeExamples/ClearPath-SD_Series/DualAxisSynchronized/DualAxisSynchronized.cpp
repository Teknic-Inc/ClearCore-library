/*
 * Title: DualAxisSynchronized
 *
 * Objective:
 *    This example demonstrates control of two ClearPath-SD motors synchronously
 *    in Step and Direction mode. Use this example when two motors must follow
 *    identical commands (e.g. a mechanically connected dual-axis or gantry).
 *
 * Description:
 *    This example enables two motors then commands a repeating series of
 *    synchronized moves. Move status is printed to the USB serial port. This
 *    example commands a max travel of 25600 pulses.
 *
 * Requirements:
 * 1. Two ClearPath motors must be connected, one to Connector M-0 the other to
 *    Connector M-1.
 * 2. The connected ClearPath motors must be configured through the MSP software
 *    for Step and Direction mode (In MSP select Mode>>Step and Direction).
 * 3. The ClearPath motors must be set to use the HLFB mode "ASG-Position
 *    w/Measured Torque" with a PWM carrier frequency of 482 Hz through the MSP
 *    software (select Advanced>>High Level Feedback [Mode]... then choose
 *    "ASG-Position w/Measured Torque" from the dropdown, make sure that 482 Hz
 *    is selected in the "PWM Carrier Frequency" dropdown, and hit the OK
 *    button).
 * 4. If the two motors must spin in opposite directions (i.e. they are mounted
 *    facing different directions), check the "Reverse Direction" checkbox of
 *    one motor in MSP.
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
 *
 * 
 * Copyright (c) 2020 Teknic Inc. This work is free to use, copy and distribute under the terms of
 * the standard MIT permissive software license which can be found at https://opensource.org/licenses/MIT
 */

#include "ClearCore.h"

// Specify which motors to move.
// Options are: ConnectorM0, ConnectorM1, ConnectorM2, or ConnectorM3.
#define motor0 ConnectorM0
#define motor1 ConnectorM1

// Select the baud rate to match the target serial device
#define baudRate 9600

// Specify which serial connector to use: ConnectorUsb, ConnectorCOM0, or
// ConnectorCOM1
#define SerialPort ConnectorUsb

// This example has built-in functionality to automatically clear motor alerts, 
//	including motor shutdowns. Any uncleared alert will cancel and disallow motion.
// WARNING: enabling automatic alert handling will clear alerts immediately when 
//	encountered and return a motor to a state in which motion is allowed. Before 
//	enabling this functionality, be sure to understand this behavior and ensure 
//	your system will not enter an unsafe state. 
// To enable automatic alert handling, #define HANDLE_ALERTS (1)
// To disable automatic alert handling, #define HANDLE_ALERTS (0)
#define HANDLE_ALERTS (0)

// Define the velocity and acceleration limits to be used for each move
int32_t velocityLimit = 10000; // pulses per sec
int32_t accelerationLimit = 100000; // pulses per sec^2

// Declares user-defined helper functions.
// The definition/implementations of these functions are at the bottom of the sketch.
bool SynchronizedMove(int32_t distance);
void PrintAlerts();
void HandleAlerts();

int main() {
    // Sets the input clocking rate. This normal rate is ideal for ClearPath
    // step and direction applications.
    MotorMgr.MotorInputClocking(MotorManager::CLOCK_RATE_NORMAL);

    // Sets all motor connectors into step and direction mode.
    MotorMgr.MotorModeSet(MotorManager::MOTOR_ALL,
                          Connector::CPM_MODE_STEP_AND_DIR);

    // Put the motor connectors into the HLFB mode to read bipolar PWM (the
    // correct mode for ASG w/ Measured Torque)
    motor0.HlfbMode(MotorDriver::HLFB_MODE_HAS_BIPOLAR_PWM);
    motor1.HlfbMode(MotorDriver::HLFB_MODE_HAS_BIPOLAR_PWM);

    // Set the HFLB carrier frequencies to 482 Hz
    motor0.HlfbCarrier(MotorDriver::HLFB_CARRIER_482_HZ);
    motor1.HlfbCarrier(MotorDriver::HLFB_CARRIER_482_HZ);

    // Sets the maximum velocity for each move.
    motor0.VelMax(velocityLimit);
    motor1.VelMax(velocityLimit);

    // Sets the maximum acceleration for each move.
    motor0.AccelMax(accelerationLimit);
    motor1.AccelMax(accelerationLimit);

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

    // Enables the motors; homing will begin automatically if enabled in MSP.
    motor0.EnableRequest(true);
    SerialPort.SendLine("Motor 0 Enabled");
    motor1.EnableRequest(true);
    SerialPort.SendLine("Motor 1 Enabled");

    // Waits for both motors to finish enabling.
    uint32_t lastStatusTime = Milliseconds();
    SerialPort.SendLine("Waiting for HLFB...");
    while ( (motor0.HlfbState() != MotorDriver::HLFB_ASSERTED ||
            motor1.HlfbState() != MotorDriver::HLFB_ASSERTED) &&
            !motor0.StatusReg().bit.AlertsPresent && 
            !motor1.StatusReg().bit.AlertsPresent) {
        // Periodically prints out why the application is waiting.
        if (Milliseconds() - lastStatusTime > 1000) {
            SerialPort.SendLine("Waiting for HLFB to assert on both motors");
            lastStatusTime = Milliseconds();
        }
    }
    // Check if motor alert occurred during enabling
	// Clear alert if configured to do so 
    if (motor0.StatusReg().bit.AlertsPresent || 
		motor1.StatusReg().bit.AlertsPresent) {
		SerialPort.SendLine("Motor alert detected.");		
		PrintAlerts();
		if(HANDLE_ALERTS){
			HandleAlerts();
		} else {
			SerialPort.SendLine("Enable automatic alert handling by setting HANDLE_ALERTS to 1.");
		}
		SerialPort.SendLine("Enabling may not have completed as expected. Proceed with caution.");		
 		SerialPort.SendLine();
	} else {
		SerialPort.SendLine("Motor Ready");	
	}

    while (true) {
        // Move 6400 counts (positive direction) then wait 2000ms.
        SynchronizedMove(6400);
        Delay_ms(2000);
        // Move 19200 counts farther positive, then wait 2000ms.
        SynchronizedMove(19200);
        Delay_ms(2000);
        // Move back 12800 counts (negative direction), then wait 2000ms.
        SynchronizedMove(-12800);
        Delay_ms(2000);
        // Move back 6400 counts (negative direction), then wait 2000ms.
        SynchronizedMove(-6400);
        Delay_ms(2000);
        // Move back to the start (negative 6400 pulses), then wait 2000ms.
        SynchronizedMove(-6400);
        Delay_ms(2000);
    }
}

/*------------------------------------------------------------------------------
 * SynchronizedMove
 *
 *    Moves two motors an incremental distance synchronously.
 *    Prints the move status to the USB serial port
 *    Returns when HLFB asserts (indicating the motor has reached the commanded
 *    position)
 *
 * Parameters:
 *    int distance  - The distance, in counts, to move
 *
 * Returns:  True/False depending on whether the moves were successfully
 *    triggered.
 */
bool SynchronizedMove(int32_t distance) {
    // Check if a motor alert is currently preventing motion
	// Clear alert if configured to do so 
    if (motor0.StatusReg().bit.AlertsPresent || motor1.StatusReg().bit.AlertsPresent ) {
		SerialPort.SendLine("Motor alert detected.");		
		PrintAlerts();
		if(HANDLE_ALERTS){
			HandleAlerts();
		} else {
			SerialPort.SendLine("Enable automatic alert handling by setting HANDLE_ALERTS to 1.");
		}
		SerialPort.SendLine("Move canceled.");		
		SerialPort.SendLine();
        return false;
    }

    SerialPort.Send("Moving distance: ");
    SerialPort.SendLine(distance);

    // Move both motors the same distance.
    motor0.Move(distance);
    motor1.Move(distance);

    // Tell the user that the program will wait for HLFB to assert on both motors
    SerialPort.SendLine("Waiting for HLFB to assert on both motors");

    // Wait until both motors complete their moves.
    uint32_t lastStatusTime = Milliseconds();
    while ( (!motor0.StepsComplete() || motor0.HlfbState() != MotorDriver::HLFB_ASSERTED ||
           !motor1.StepsComplete() || motor1.HlfbState() != MotorDriver::HLFB_ASSERTED) &&
		   !motor0.StatusReg().bit.AlertsPresent && !motor1.StatusReg().bit.AlertsPresent ){
        continue;
    }

    // Check if motor alert occurred during move
    // Clear alert if configured to do so 
    if (motor0.StatusReg().bit.AlertsPresent || motor1.StatusReg().bit.AlertsPresent){
        motor0.MoveStopAbrupt();
        motor1.MoveStopAbrupt();
        SerialPort.SendLine("Motor alert detected.");		
        PrintAlerts();
        if(HANDLE_ALERTS){
            HandleAlerts();
        } else {
            SerialPort.SendLine("Enable automatic fault handling by setting HANDLE_ALERTS to 1.");
        }
        SerialPort.SendLine("Motion may not have completed as expected. Proceed with caution.");
        SerialPort.SendLine();
        return false;
    } else {
        SerialPort.SendLine("Move Done");
        return true;
    }
}


/*------------------------------------------------------------------------------
 * PrintAlerts
 *
 *    Prints active alerts.
 *
 * Parameters:
 *    requires "motor0" and "motor1" to be defined as ClearCore motor connectors
 *
 * Returns: 
 *    none
 */
 void PrintAlerts(){
	// report status of alerts on motor0
 	SerialPort.SendLine("Alerts present on motor0: ");
	if(motor0.AlertReg().bit.MotionCanceledInAlert){
		SerialPort.SendLine("    MotionCanceledInAlert "); }
	if(motor0.AlertReg().bit.MotionCanceledPositiveLimit){
		SerialPort.SendLine("    MotionCanceledPositiveLimit "); }
	if(motor0.AlertReg().bit.MotionCanceledNegativeLimit){
		SerialPort.SendLine("    MotionCanceledNegativeLimit "); }
	if(motor0.AlertReg().bit.MotionCanceledSensorEStop){
		SerialPort.SendLine("    MotionCanceledSensorEStop "); }
	if(motor0.AlertReg().bit.MotionCanceledMotorDisabled){
		SerialPort.SendLine("    MotionCanceledMotorDisabled "); }
	if(motor0.AlertReg().bit.MotorFaulted){
		SerialPort.SendLine("    MotorFaulted ");
	}
	
	// report status of alerts on motor1
 	SerialPort.SendLine("Alerts present on motor1: ");
	if(motor1.AlertReg().bit.MotionCanceledInAlert){
		SerialPort.SendLine("    MotionCanceledInAlert "); }
	if(motor1.AlertReg().bit.MotionCanceledPositiveLimit){
		SerialPort.SendLine("    MotionCanceledPositiveLimit "); }
	if(motor1.AlertReg().bit.MotionCanceledNegativeLimit){
		SerialPort.SendLine("    MotionCanceledNegativeLimit "); }
	if(motor1.AlertReg().bit.MotionCanceledSensorEStop){
		SerialPort.SendLine("    MotionCanceledSensorEStop "); }
	if(motor1.AlertReg().bit.MotionCanceledMotorDisabled){
		SerialPort.SendLine("    MotionCanceledMotorDisabled "); }
	if(motor1.AlertReg().bit.MotorFaulted){
		SerialPort.SendLine("    MotorFaulted ");
	}
 }
//------------------------------------------------------------------------------


/*------------------------------------------------------------------------------
 * HandleAlerts
 *
 *    Clears alerts, including motor faults. 
 *    Faults are cleared by cycling enable to the motor.
 *    Alerts are cleared by clearing the ClearCore alert register directly.
 *
 * Parameters:
 *    requires "motor0" and "motor1" to be defined as ClearCore motor connectors
 *
 * Returns: 
 *    none
 */
 void HandleAlerts(){
	// for each motor, if a motor fault is present, clear it by cycling enable
	if(motor0.AlertReg().bit.MotorFaulted){
		SerialPort.SendLine("Faults present on motor0. Cycling enable signal to motor to clear faults.");
		motor0.EnableRequest(false);
		Delay_ms(10);
		motor0.EnableRequest(true);
	}
	if(motor1.AlertReg().bit.MotorFaulted){
		SerialPort.SendLine("Faults present on motor1. Cycling enable signal to motor to clear faults.");
		motor1.EnableRequest(false);
		Delay_ms(10);
		motor1.EnableRequest(true);
	}
	// clear alerts
	SerialPort.SendLine("Clearing alerts on both motors.");
	motor0.ClearAlerts();
	motor1.ClearAlerts();
 }
//------------------------------------------------------------------------------
