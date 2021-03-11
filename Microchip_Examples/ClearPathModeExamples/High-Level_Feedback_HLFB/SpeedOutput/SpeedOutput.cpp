/*
 * Title: SpeedOutput
 *
 * Objective:
 *    This example demonstrates how to configure and read-in the High-Level
 *    Feedback mode "Speed Output" of a ClearPath motor.
 *
 *    This HLFB mode is available in ClearPath-MC series servos, in every
 *    ClearPath-MC operational mode except Ramp Up/Down to Selected Velocity.
 *
 * Description:
 *    This example reads the state of an attached ClearPath motor's HLFB output
 *    when configured for "Speed Output". During operation, the state of HLFB
 *    and calculated measured speed are written to the USB serial port.
 *
 *    This example does not enable the motor or command motion. Use the
 *    "Override Inputs" feature in MSP to command motion and see changes in the
 *    HLFB measurement.
 *
 * Requirements:
 * 1. A ClearPath motor must be connected to Connector M-0.
 * 2.  The connected ClearPath motor must be configured through the MSP software
 *    for an operational mode compatible with Speed Output HLFB mode (see above)
 * 3. The connected ClearPath motor must have its HLFB mode set to "Speed Output"
 *    (select Advanced>>High Level Feedback [Mode]... then choose "Speed Output"
 *    from the dropdown and hit the OK button).
 *    Select a 482 Hz PWM Carrier Frequency in this menu.
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

// Specify which motor to move.
// Options are: ConnectorM0, ConnectorM1, ConnectorM2, or ConnectorM3.
#define motor ConnectorM0

// Select the baud rate to match the target serial device
#define baudRate 9600

// Specify which serial to use: ConnectorUsb, ConnectorCOM0, or ConnectorCOM1.
#define SerialPort ConnectorUsb

int main() {
    // Put the motor connector into the correct HLFB mode to read the Speed
    // Output PWM signal and convert it to percent of Max Speed.
    motor.HlfbMode(MotorDriver::HLFB_MODE_HAS_PWM);
    // Set the HFLB carrier frequency to 482 Hz
    motor.HlfbCarrier(MotorDriver::HLFB_CARRIER_482_HZ);

    // Set up serial communication at a baud rate of baudRate (9600 bps) then
    // wait up to 5 seconds for a port to open.
    SerialPort.Mode(Connector::USB_CDC);
    SerialPort.Speed(baudRate);
    uint32_t timeout = 5000;
    uint32_t startTime = Milliseconds();
    SerialPort.PortOpen();
    while (!SerialPort && Milliseconds() - startTime < timeout) {
        continue;
    }

    while (true) {
        // Use the MSP application to enable and move the motor. The duty cycle of
        // the HFLB output will be refreshed and displayed every 0.5 seconds.

        // Check the state of the HLFB.
        MotorDriver::HlfbStates hlfbState = motor.HlfbState();

        // Print the HLFB state.
        if (hlfbState == MotorDriver::HLFB_HAS_MEASUREMENT) {
            // Get the measured speed as a percent of Max Speed.
            float hlfbPercent = motor.HlfbPercent();

            SerialPort.Send("Speed output: ");

            if (hlfbPercent == MotorDriver::HLFB_DUTY_UNKNOWN) {
                SerialPort.SendLine("UNKNOWN");
            }
            else {
                char hlfbPercentStr[10];
                // Convert the floating point duty cycle into a string representation.
                snprintf(hlfbPercentStr, sizeof(hlfbPercentStr), "%.0f%%", hlfbPercent);
                SerialPort.Send(hlfbPercentStr);
                SerialPort.SendLine(" of maximum speed");
            }
        }
        else if (hlfbState == MotorDriver::HLFB_DEASSERTED) {
            SerialPort.SendLine("Disabled or Shutdown");
        }

        // Wait 0.5 secs before reading the HLFB again.
        Delay_ms(500);
    }
}
