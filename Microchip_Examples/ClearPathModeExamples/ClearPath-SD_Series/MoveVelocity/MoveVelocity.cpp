/*
 * Title: MoveVelocity
 *
 * Objective:
 *    This example demonstrates control of a ClearPath motor in Step and
 *    Direction mode, making velocity moves.
 *
 * Description:
 *    This example enables a ClearPath then commands a series of repeating
 *    velocity moves to the motor.
 *
 * Requirements:
 * 1. A ClearPath motor must be connected to Connector M-0.
 * 2. The connected ClearPath motor must be configured through the MSP software
 *    for Step and Direction mode (In MSP select Mode>>Step and Direction).
 * 3. The ClearPath motor must be set to use the HLFB mode "ASG-Position
 *    w/Measured Torque" with a PWM carrier frequency of 482 Hz through the MSP
 *    software (select Advanced>>High Level Feedback [Mode]... then choose
 *    "ASG-Position w/Measured Torque" from the dropdown, make sure that 482 Hz
 *    is selected in the "PWM Carrier Frequency" dropdown, and hit the OK
 *    button).
 * 4. Set the Input Format in MSP for "Step + Direction".
 *
 * ** Note: Set the Input Resolution in MSP the same as your motor's Positioning
 *    Resolution spec if you'd like the pulse frequency sent by ClearCore to 
 *    command the same frequency in motor encoder counts/sec, a 1:1 ratio.
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

// Specifies which motor to move.
// Options are: ConnectorM0, ConnectorM1, ConnectorM2, or ConnectorM3.
#define motor ConnectorM0

// Select the baud rate to match the target serial device
#define baudRate 9600

// Specify which serial to use: ConnectorUsb, ConnectorCOM0, or ConnectorCOM1.
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

// Define the acceleration limit to be used for each move
int32_t accelerationLimit = 100000; // pulses per sec^2

// Declares user-defined helper functions.
// The definition/implementations of these functions are at the bottom of the sketch.
bool MoveAtVelocity(int32_t velocity);
void PrintAlerts();
void HandleAlerts();

int main() {
    // Sets the input clocking rate. This normal rate is ideal for ClearPath
    // step and direction applications.
    MotorMgr.MotorInputClocking(MotorManager::CLOCK_RATE_NORMAL);

    // Sets all motor connectors into step and direction mode.
    MotorMgr.MotorModeSet(MotorManager::MOTOR_ALL,
                          Connector::CPM_MODE_STEP_AND_DIR);

    // Set the motor's HLFB mode to bipolar PWM
    motor.HlfbMode(MotorDriver::HLFB_MODE_HAS_BIPOLAR_PWM);
    // Set the HFLB carrier frequency to 482 Hz
    motor.HlfbCarrier(MotorDriver::HLFB_CARRIER_482_HZ);

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

    // Enables the motor; homing will begin automatically if enabled
    motor.EnableRequest(true);
    SerialPort.SendLine("Motor Enabled");

    // Waits for HLFB to assert (waits for homing to complete if applicable)
    SerialPort.SendLine("Waiting for HLFB...");
    while (motor.HlfbState() != MotorDriver::HLFB_ASSERTED &&
			!motor.StatusReg().bit.AlertsPresent) {
        continue;
    }
	// Check if motor alert occurred during enabling
	// Clear alert if configured to do so 
    if (motor.StatusReg().bit.AlertsPresent) {
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
        // Move at 1,000 steps/sec for 2000ms
        MoveAtVelocity(1000);
        Delay_ms(2000);
        // Move at -5,000 steps/sec for 2000ms
        MoveAtVelocity(-5000);
        Delay_ms(2000);
        // Move at 10,000 steps/sec for 2000ms
        MoveAtVelocity(10000);
        Delay_ms(2000);
        // Move at -10,000 steps/sec for 2000ms
        MoveAtVelocity(-10000);
        Delay_ms(2000);
        // Command a 0 steps/sec velocity to stop motion for 2000ms
        MoveAtVelocity(0);
        Delay_ms(2000);
    }
}

/*------------------------------------------------------------------------------
 * MoveAtVelocity
 *
 *    Command the motor to move at the specified "velocity", in steps/second.
 *    Prints the move status to the USB serial port
 *
 * Parameters:
 *    int velocity  - The velocity, in step steps/sec, to command
 *
 * Returns: None
 */
bool MoveAtVelocity(int32_t velocity) {
    // Check if a motor alert is currently preventing motion
	// Clear alert if configured to do so 
    if (motor.StatusReg().bit.AlertsPresent) {
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

    SerialPort.Send("Commanding velocity: ");
    SerialPort.SendLine(velocity);

    // Command the velocity move
    motor.MoveVelocity(velocity);

    // Waits for the step command to ramp up/down to the commanded velocity. 
    // This time will depend on your Acceleration Limit.
    SerialPort.SendLine("Ramping to speed...");
    while (!motor.StatusReg().bit.AtTargetVelocity) {
        continue;
    }
	// Check if motor alert occurred during move
	// Clear alert if configured to do so 
    if (motor.StatusReg().bit.AlertsPresent) {
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
//------------------------------------------------------------------------------


/*------------------------------------------------------------------------------
 * PrintAlerts
 *
 *    Prints active alerts.
 *
 * Parameters:
 *    requires "motor" to be defined as a ClearCore motor connector
 *
 * Returns: 
 *    none
 */
 void PrintAlerts(){
	// report status of alerts
 	SerialPort.SendLine("Alerts present: ");
	if(motor.AlertReg().bit.MotionCanceledInAlert){
		SerialPort.SendLine("    MotionCanceledInAlert "); }
	if(motor.AlertReg().bit.MotionCanceledPositiveLimit){
		SerialPort.SendLine("    MotionCanceledPositiveLimit "); }
	if(motor.AlertReg().bit.MotionCanceledNegativeLimit){
		SerialPort.SendLine("    MotionCanceledNegativeLimit "); }
	if(motor.AlertReg().bit.MotionCanceledSensorEStop){
		SerialPort.SendLine("    MotionCanceledSensorEStop "); }
	if(motor.AlertReg().bit.MotionCanceledMotorDisabled){
		SerialPort.SendLine("    MotionCanceledMotorDisabled "); }
	if(motor.AlertReg().bit.MotorFaulted){
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
 *    requires "motor" to be defined as a ClearCore motor connector
 *
 * Returns: 
 *    none
 */
 void HandleAlerts(){
	if(motor.AlertReg().bit.MotorFaulted){
		// if a motor fault is present, clear it by cycling enable
		SerialPort.SendLine("Faults present. Cycling enable signal to motor to clear faults.");
		motor.EnableRequest(false);
		Delay_ms(10);
		motor.EnableRequest(true);
	}
	// clear alerts
	SerialPort.SendLine("Clearing alerts.");
	motor.ClearAlerts();
 }
//------------------------------------------------------------------------------

 

