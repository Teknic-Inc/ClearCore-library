/*
 * Title: MotorStatusRegister
 *
 * Objective:
 *    This example demonstrates how to read and display bits in the ClearCore's
 *    MotorDriver status register.
 *
 * Description:
 *    This example gets a snapshot of the status register for each MotorDriver
 *    connector with an attached motor. Then, the state of the status register
 *    bits is printed to the USB serial port.
 *
 * Requirements:
 * ** A ClearPath motor must be connected to Connector M-0.
 *
 * Links:
 * ** ClearCore Documentation: https://teknic-inc.github.io/ClearCore-library/
 * ** ClearCore Manual: https://www.teknic.com/files/downloads/clearcore_user_manual.pdf
 *
 * 
 * Copyright (c) 2020 Teknic Inc. This work is free to use, copy and distribute under the terms of
 * the standard MIT permissive software license which can be found at https://opensource.org/licenses/MIT
 */

#include "ClearCore.h"

// Select the baud rate to match the target device.
#define baudRate 9600

// Specify which serial to use: ConnectorUsb, ConnectorCOM0, or ConnectorCOM1.
#define SerialPort ConnectorUsb

// The containers for our motor objects. If only some of the motor connectors
// are being used, remove the unused entries from the following arrays and
// reduce motorConnectorCount.
MotorDriver *motorConnectors[] = {&ConnectorM0, &ConnectorM1,
                                  &ConnectorM2, &ConnectorM3
                                 };
char motorConnectorNames[][4] = { "M-0", "M-1", "M-2", "M-3" };
uint8_t motorConnectorCount = 4;

// Hold a string representation of each motor's ready state.
char *readyStateStr;

// Declare our helper function so that it may be used before it is defined.
char *ReadyStateString(MotorDriver::MotorReadyStates readyState);

int main() {
    // Set up serial communication at a baud rate of 9600 bps then wait up to
    // 5 seconds for a port to open.
    // Serial communication is not required for this example to run, however the
    // example will appear to do nothing without serial output.
    SerialPort.Mode(Connector::USB_CDC);
    SerialPort.Speed(baudRate);
    uint32_t timeout = 5000;
    uint32_t startTime = Milliseconds();
    SerialPort.PortOpen();
    while (!SerialPort && Milliseconds() - startTime < timeout) {
        continue;
    }

    while (true) {
        // Get a copy of the motor status register for each motor connector.
        for (uint8_t i = 0; i < motorConnectorCount; i++) {
            MotorDriver *motor = motorConnectors[i];
            volatile const MotorDriver::StatusRegMotor &statusReg = motor->StatusReg();
			volatile const MotorDriver::AlertRegMotor &alertReg = motor->AlertReg();

            SerialPort.Send("Motor status register for motor M");
			SerialPort.Send(i);
			SerialPort.Send(": ");
			SerialPort.SendLine(statusReg.reg, 2); // prints the status register in binary
	
			SerialPort.Send("AtTargetPosition:	");
			SerialPort.SendLine(statusReg.bit.AtTargetPosition);

			SerialPort.Send("StepsActive:     	");
			SerialPort.SendLine(statusReg.bit.StepsActive);

			SerialPort.Send("AtTargetVelocity:	");
			SerialPort.SendLine(statusReg.bit.AtTargetVelocity);
		
			SerialPort.Send("MoveDirection:   	");
			SerialPort.SendLine(statusReg.bit.MoveDirection);

			SerialPort.Send("MotorInFault:    	");
			SerialPort.SendLine(statusReg.bit.MotorInFault);

			SerialPort.Send("Enabled:         	");
			SerialPort.SendLine(statusReg.bit.Enabled);

			SerialPort.Send("PositionalMove:  	");
			SerialPort.SendLine(statusReg.bit.PositionalMove);

			SerialPort.Send("HLFB State:		");
			switch (statusReg.bit.HlfbState) {
				case 0:
					SerialPort.SendLine("HLFB_DEASSERTED");
					break;
				case 1:
					SerialPort.SendLine("HLFB_ASSERTED");
					break;
				case 2:
					SerialPort.SendLine("HLFB_HAS_MEASUREMENT");
					break;
				case 3:
					SerialPort.SendLine("HLFB_UNKNOWN");
					break;
				default:
					// something has gone wrong if this is printed
					SerialPort.SendLine("???");
			}

			SerialPort.Send("AlertsPresent:   	");
			SerialPort.SendLine(statusReg.bit.AlertsPresent);

			SerialPort.Send("Ready state:		");
			switch (statusReg.bit.ReadyState) {
				case MotorDriver::MOTOR_DISABLED:
					SerialPort.SendLine("Disabled");
					break;
				case MotorDriver::MOTOR_ENABLING:
					SerialPort.SendLine("Enabling");
					break;
				case MotorDriver::MOTOR_FAULTED:
					SerialPort.SendLine("Faulted");
					break;
				case MotorDriver::MOTOR_READY:
					SerialPort.SendLine("Ready");
					break;
				case MotorDriver::MOTOR_MOVING:
					SerialPort.SendLine("Moving");
					break;
				default:
					// something has gone wrong if this is printed
					SerialPort.SendLine("???");
			}
	
			SerialPort.Send("Triggering:      	");
			SerialPort.SendLine(statusReg.bit.Triggering);

			SerialPort.Send("InPositiveLimit: 	");
			SerialPort.SendLine(statusReg.bit.InPositiveLimit);

			SerialPort.Send("InNegativeLimit: 	");
			SerialPort.SendLine(statusReg.bit.InNegativeLimit);

			SerialPort.Send("InEStopSensor:   	");
			SerialPort.SendLine(statusReg.bit.InEStopSensor);	

			SerialPort.SendLine("--------------------------------");
	
		
			if (statusReg.bit.AlertsPresent){
				SerialPort.Send("Alert register:	");
				SerialPort.SendLine(alertReg.reg, 2); // prints the alert register in binary

				SerialPort.Send("MotionCanceledInAlert:         ");
				SerialPort.SendLine(alertReg.bit.MotionCanceledInAlert);

				SerialPort.Send("MotionCanceledPositiveLimit:   ");
				SerialPort.SendLine(alertReg.bit.MotionCanceledPositiveLimit);

				SerialPort.Send("MotionCanceledNegativeLimit:   ");
				SerialPort.SendLine(alertReg.bit.MotionCanceledNegativeLimit);

				SerialPort.Send("MotionCanceledSensorEStop:     ");
				SerialPort.SendLine(alertReg.bit.MotionCanceledSensorEStop);

				SerialPort.Send("MotionCanceledMotorDisabled:   ");
				SerialPort.SendLine(alertReg.bit.MotionCanceledMotorDisabled);

				SerialPort.Send("MotorFaulted:                  ");
				SerialPort.SendLine(alertReg.bit.MotorFaulted);

				SerialPort.SendLine("--------------------------------");
			}
        }

        // Wait a few seconds then repeat...
        Delay_ms(5000);
    }
}

/*------------------------------------------------------------------------------
 * ReadyStateString
 *
 *    Converts the state of a motor status register bit into a user-readable
 *    format so it may be printed to a serial port.
 *
 * Parameters:
 *    MotorReadyStates readyState  - The current state of the ReadyState bit
 *
 * Returns: Text describing the state of the status bit.
 */
char *ReadyStateString(MotorDriver::MotorReadyStates readyState) {
    switch (readyState) {
        case MotorDriver::MOTOR_DISABLED:
            return (char *)"Disabled";
        case MotorDriver::MOTOR_ENABLING:
            return (char *)"Enabling";
        case MotorDriver::MOTOR_FAULTED:
            return (char *)"Faulted";
        case MotorDriver::MOTOR_READY:
            return (char *)"Ready";
        case MotorDriver::MOTOR_MOVING:
            return (char *)"Moving";
        default:
            // Something has gone wrong if this is printed
            return (char *)"???";
    }
}
//------------------------------------------------------------------------------
