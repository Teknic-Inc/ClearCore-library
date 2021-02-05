/*
 * Title: SpiComPortInitialization
 *
 * Objective:
 *    This example demonstrates how to configure a COM port for use with an
 *    SPI device.
 *
 * Description:
 *    This example will explain the basic configuration settings of an SPI
 *    device then perform a brief transaction with the SPI device connected to
 *    COM-0.
 *
 * Requirements:
 * ** An SPI device connected to COM-0.
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
#define baudRate 80000

// Select the clock polarity to match the target device. The clock polarity
// setting indicates whether the device expects a low signal when idle or a
// high signal when idle. It also indicates whether the leading or trailing
// edge of the clock cycle are rising or falling edges.
// Selecting SCK_LOW indicates that SCK is low when idle and the leading edge
// of the clock cycle is a rising edge while the trailing edge is a falling
// edge.
// Selecting SCK_HIGH indicates that SCK is high when idle and the leading edge
// of the clock cycle is a falling edge while the trailing edge is a rising
// edge.
// The default value for a COM connector's clock polarity is SCK_LOW.
#define clockPolarity SerialDriver::SCK_LOW

// Select the clock phase setting to match the target device. The clock phase
// setting indicates whether data is sampled or changed on the leading or
// trailing edge in the clock cycle.
// Selecting LEAD_SAMPLE indicates that data is sampled on the leading edge and
// changed on the trailing edge.
// Selecting LEAD_CHANGE indicates that data is sampled on the trailing edge
// and changed on the leading edge.
// The default value for a COM connector's clock phase is LEAD_CHANGE.
#define clockPhase SerialDriver::LEAD_CHANGE

// Define which COM serial port connector to use: ConnectorCOM0 or ConnectorCOM1
#define SpiPort ConnectorCOM0

int main() {
    // Configure the COM port for our SPI device then open the port.
    SpiPort.Mode(Connector::SPI);
    SpiPort.Speed(baudRate);
    SpiPort.SpiClock(clockPolarity, clockPhase);
    SpiPort.PortOpen();

    // Open the SPI port on ConnectorCOM0.
    SpiPort.SpiSsMode(SerialDriver::LINE_ON);
    // Output some arbitrary sample data to the SPI device. This data is not
    // required for set up, just to demonstrate the transfer process.
    SpiPort.SpiTransferData('a');
    SpiPort.SpiTransferData(98);
    SpiPort.SpiTransferData(0x63);
    // Close the port.
    SpiPort.SpiSsMode(SerialDriver::LINE_OFF);
}
