/*
 * Title: SpiSerialDisplay
 *
 * Objective:
 *    This example demonstrates how to write data to an SPI device.
 *
 * Description:
 *    This example will set up SPI communications on COM-0 then write various
 *    data to the device.
 *
 * Requirements:
 * ** A NHD-0420D3Z LCD display in SPI mode connected to COM-0
 *    Datasheet: http://www.newhavendisplay.com/specs/NHD-0420D3Z-NSW-BBW-V3.pdf
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

// Data bounds of the device
#define NUM_ROWS 4
#define NUM_COLUMNS 20

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
#define clockPolarity SerialDriver::SCK_HIGH

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

// Sample data to write to the display
const uint8_t line1[21] = "abcdefghijklmnopqrst";
const uint8_t line2[21] = "ABCDEFGHIJKLMNOPQRST";
const uint8_t line3[21] = "01234567890123456789";
const uint8_t line4[21] = "98765432109876543210";

// Declare our helper functions so that they may be used before they are defined
void SetBrightness(uint8_t level);
void SetCursor(uint8_t row, uint8_t column);

int main() {
    // Configure the COM port for our SPI device then open the port.
    SpiPort.Mode(Connector::SPI);
    SpiPort.Speed(baudRate);
    SpiPort.DataOrder(SerialDriver::COM_MSB_FIRST);
    SpiPort.SpiClock(clockPolarity, clockPhase);
    SpiPort.PortOpen();

    // The COM port is now configured and ready to send commands and
    // data to the display.

    // Set the display brightness level.
    // The maximum value for full brightness is 8.
    SetBrightness(4);

    // Set the cursor position to the top-left corner.
    SetCursor(0, 0);

    // Open the SPI port on ConnectorCOM0.
    SpiPort.SpiSsMode(SerialDriver::LINE_ON);

    // Send the lines "out of order" (1, 3, 2, 4) to the display.
    // Without resetting the cursor position for each line, this is the order
    // in which lines must be sent to be displayed correctly.
    SpiPort.SpiTransferData(line1, NULL, 20);
    SpiPort.SpiTransferData(line3, NULL, 20);
    SpiPort.SpiTransferData(line2, NULL, 20);
    SpiPort.SpiTransferData(line4, NULL, 20);

    // Close the port.
    SpiPort.SpiSsMode(SerialDriver::LINE_OFF);
}

/*------------------------------------------------------------------------------
 * SetBrightness
 *
 *    Sends a short SPI transaction to control the brightness of the attached
 *    LCD screen. See the device's datasheet for a full set of commands and
 *    syntax.
 *
 * Parameters:
 *    uint8_t level  - The brightness level to be set
 *
 * Returns: None
 */
void SetBrightness(uint8_t level) {
    SpiPort.SpiSsMode(SerialDriver::LINE_ON);
    SpiPort.SpiTransferData(0xfe);
    SpiPort.SpiTransferData(0x53);
    SpiPort.SpiTransferData(level);
    SpiPort.SpiSsMode(SerialDriver::LINE_OFF);
}
//------------------------------------------------------------------------------

/*------------------------------------------------------------------------------
 * SetCursor
 *
 *    Sends a short SPI transaction to control the position of the device's
 *    internal cursor that controls where characters are printed on the LCD
 *    screen. See the device's datasheet for a full set of commands and syntax.
 *
 * Parameters:
 *    uint8_t row  - The character row to move the cursor to.
 *    uint8_t column  - The character column to move the cursor to.
 *
 * Returns: None
 */
void SetCursor(uint8_t row, uint8_t column) {
    // Bounds-check the passed-in row and column
    if (row >= NUM_ROWS) {
        row = 0;
    }
    if (column >= NUM_COLUMNS) {
        column = 0;
    }

    uint8_t position = row * NUM_COLUMNS + column;
    SpiPort.SpiSsMode(SerialDriver::LINE_ON);
    SpiPort.SpiTransferData(0xfe);
    SpiPort.SpiTransferData(0x45);
    SpiPort.SpiTransferData(position);
    SpiPort.SpiSsMode(SerialDriver::LINE_OFF);
}
//------------------------------------------------------------------------------
