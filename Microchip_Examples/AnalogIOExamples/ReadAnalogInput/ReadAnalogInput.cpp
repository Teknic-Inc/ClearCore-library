/*
 * Title: ReadAnalogInput
 *
 * Objective:
 *    This example demonstrates how to read the analog voltage of an analog
 *    input. ClearCore analog inputs are natively compatible with 0-10V signals,
 *    and 0-20mA signals with addition of an external resistor (see ClearCore
 *    manual link below for wiring).
 *
 * Description:
 *    This example sets up pin A-12 as an analog input, queries the value on
 *    that connector every second, and calculates the input voltage. This
 *    calculated voltage is written/displayed to the USB serial port.
 *    Connectors IO-0 through IO-5 will act as a coarse meter of the voltage
 *    read-in by turning on their outputs and LEDs (with more than 0V and less
 *    than 2V, IO-0 will be on as an output. With ~2V and less than 4V, IO-0 and
 *    IO-1 will be on; and so on until, with near 10V read in, IO-0 through IO-5
 *    will be turned on).
 *
 * Requirements:
 * ** An analog input source connected to A-12.
 * ** Optional: LEDs connected to IO-0 through IO-5 to act as a more prominent
 *    voltage meter than the ClearCore's onboard LEDs.
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

// Defines the bit-depth of the ADC readings (8-bit, 10-bit, or 12-bit)
// Supported adcResolution values are 8, 10, and 12
#define adcResolution 12

// Select the baud rate to match the target device.
#define baudRate 9600

// Specify which serial connector to use:
//   ConnectorUsb, ConnectorCOM0, or ConnectorCOM1
#define SerialPort ConnectorUsb

int main() {
    // Initialize the serial port for printing analog voltage readings and wait
    // up to 5 seconds for a port to open. Serial communication is not required
    // for this example to run, however without it only the coarse LED meter
    // can be used to read the analog signal.
    SerialPort.Mode(Connector::USB_CDC);
    SerialPort.Speed(baudRate);
    uint32_t timeout = 5000;
    uint32_t startTime = Milliseconds();
    SerialPort.PortOpen();
    while (!SerialPort && Milliseconds() - startTime < timeout) {
        continue;
    }

    // Make a voltage meter display with the I/O pins.
    ConnectorIO0.Mode(Connector::OUTPUT_DIGITAL);
    ConnectorIO1.Mode(Connector::OUTPUT_DIGITAL);
    ConnectorIO2.Mode(Connector::OUTPUT_DIGITAL);
    ConnectorIO3.Mode(Connector::OUTPUT_DIGITAL);
    ConnectorIO4.Mode(Connector::OUTPUT_DIGITAL);
    ConnectorIO5.Mode(Connector::OUTPUT_DIGITAL);

    // Clear out the state of our voltage meter to start.
    ConnectorIO0.State(false);
    ConnectorIO1.State(false);
    ConnectorIO2.State(false);
    ConnectorIO3.State(false);
    ConnectorIO4.State(false);
    ConnectorIO5.State(false);

    // Since analog inputs default to analog input mode, there's no need to
    // call Mode().

    // Set the resolution of the ADC.
    AdcMgr.AdcResolution(adcResolution);

    while (true) {
        // Read the analog input (A-9 through A-12 may be configured as analog
        // inputs).
        int16_t adcResult = ConnectorA12.State();
        // Convert the reading to a voltage.
        double inputVoltage = 10.0 * adcResult / ((1 << adcResolution) - 1);

        // Display the voltage reading to the serial port.
        SerialPort.Send("A-12 input voltage: ");
        SerialPort.Send(inputVoltage);
        SerialPort.SendLine("V.");

        // Write the voltage reading to the voltage meter display pins
        // (IO-0 through IO-5).
        if (inputVoltage > 0.1) {
            ConnectorIO0.State(true);
        }
        else {
            ConnectorIO0.State(false);
        }
        if (inputVoltage > 2.0) {
            ConnectorIO1.State(true);
        }
        else {
            ConnectorIO1.State(false);
        }
        if (inputVoltage > 4.0) {
            ConnectorIO2.State(true);
        }
        else {
            ConnectorIO2.State(false);
        }
        if (inputVoltage > 6.0) {
            ConnectorIO3.State(true);
        }
        else {
            ConnectorIO3.State(false);
        }
        if (inputVoltage > 8.0) {
            ConnectorIO4.State(true);
        }
        else {
            ConnectorIO4.State(false);
        }
        if (inputVoltage >= 9.9) {
            ConnectorIO5.State(true);
        }
        else {
            ConnectorIO5.State(false);
        }

        // Wait a second before the next reading.
        Delay_ms(1000);
    }
}
