/*
 * Title: UserSeeksHome
 *
 * Objective:
 *    This example demonstrates control of the "User Seeks Home" homing feature
 *    of a ClearPath-SD motor (this feature is also available in ClearPath-MCPV
 *    Pulse Burst Positioning Mode, but this example is only for Step and
 *    Direction mode). "User Seeks Home" should be used when more flexibility is
 *    required during a homing sequence (e.g. to move at multiple velocities,
 *    stopping to perform other tasks, manually exiting a homing sequence, or
 *    using sensor-based homing).
 *
 * Description:
 *    This example enables the motor, moves towards a hardstop at a velocity for
 *    2 seconds, then with a slower velocity until clamping into the hardstop.
 *    An offset position move is then commanded away from the hardstop to
 *    complete the homing sequence. Homing status is printed to the USB serial
 *    port.
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
 * 4. The ClearPath must have homing enabled and configured. To configure, look
 *    under the "Homing" label on the MSP's main window, check the "Enabled"
 *    radial button, then click the "Setup..." button. Set the Homing Style set
 *    to "User seeks home; ClearPath ASG signals when homing is complete" then
 *    hit the OK button.
 * 5. A hardstop for homing must be installed on your mechanics. Set the homing
 *    torque limit accordingly. This torque will be used to move toward and
 *    clamp up against the hardstop.
 *
 * ** IMPORTANT: This examples homes in the Positive (CCW) direction, assuming
 *    the hardstop is on the positive end of travel
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

// Specify which motor to move.
// Options are: ConnectorM0, ConnectorM1, ConnectorM2, or ConnectorM3.
#define motor ConnectorM0

// Selects the baud rate to match the target serial device
#define baudRate 9600

// Specify which serial connector to use: ConnectorUsb, ConnectorCOM0, or
// ConnectorCOM1
#define SerialPort ConnectorUsb

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

    // Sets the maximum velocity in step pulses/sec.
    motor.VelMax(10000);
    // Sets the maximum acceleration in step pulses/sec^2.
    motor.AccelMax(100000);

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

    // Enables the motor
    motor.EnableRequest(true);
    SerialPort.SendLine("Motor Enabled");

    // Check if an alert would prevent motion
    if (motor.StatusReg().bit.AlertsPresent) {
        // In this case, we can't proceed with homing.
        SerialPort.SendLine("Motor status: 'In Alert'. Move Canceled.");
        // The end...
        while (true) {
            continue;
        }
    }

    // Commands a speed of 5000 pulses/sec towards the hardstop for 2 seconds
    SerialPort.SendLine("Moving toward hardstop... Waiting for HLFB");
    motor.MoveVelocity(5000);
    Delay_ms(2000);
    // Then slows down to 1000 pulses/sec until clamping into the hard stop
    motor.MoveVelocity(1000);

    // Delay so HLFB has time to deassert
    Delay_ms(10);
    // Waits for HLFB to assert again, meaning the hardstop has been reached
    while (motor.HlfbState() != MotorDriver::HLFB_ASSERTED) {
        continue;
    }

    // Stop the velocity move now that the hardstop is reached
    motor.MoveStopAbrupt();

    // Move away from the hard stop. Any move away from the hardstop will
    // conclude the homing sequence.
    motor.Move(-1000);

    // Delay so HLFB has time to deassert
    Delay_ms(10);
    // Waits for HLFB to assert, meaning homing is complete
    SerialPort.SendLine("Moving away from hardstop... Waiting for HLFB");
    while (motor.HlfbState() != MotorDriver::HLFB_ASSERTED) {
        continue;
    }
    SerialPort.SendLine("Homing Complete. Motor Ready.");

    // Zero the motor's reference position after homing to allow for accurate
    // absolute position moves
    motor.PositionRefSet(0);
}
