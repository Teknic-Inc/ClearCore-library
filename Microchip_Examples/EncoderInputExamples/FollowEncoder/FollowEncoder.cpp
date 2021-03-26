/*
 * Title: FollowEncoder
 *
 * Objective:
 *    This example demonstrates the ClearCore's Encoder Input module
 *    functionality.
 *
 * Description:
 *    This example takes input signals from an external encoder through the 
 *    CL-ENCRD-DFIN Encoder Adapter Board, and uses the encoder position
 *    or velocity to control a ClearPath-SD servo.
 *
 * Requirements:
 * 1. A ClearPath-SD motor must be connected to Connector M-0.
 * 2. The connected ClearPath motor must be configured through the MSP software
 *    for Step and Direction mode (In MSP select Mode>>Step and Direction).
 * 3. The ClearPath motor must be set to use the HLFB mode "ASG-Position"
 *    through the MSP software (select Advanced>>High Level Feedback [Mode]...
 *    then choose "All Systems Go (ASG) - Position" from the dropdown and hit
 *    the OK button).
 * 4. Set the Input Format in MSP for "Step + Direction".
 * 5. An external encoder much be wired to the CL-ENCRD-DFIN Encoder Adapter Board, 
 *    and the board connected to the ClearCore I/O Header. See the ClearCore User 
 *    Manaual for connector pinouts.
 *
 * ** Reminder: When using the CL-ENCRD-DFIN Encoder Adapter Board, ClearCore 
 *    connectors DI-6, DI-7, and DI-8 are unavailable and should be left Not  
 *    Connected to any external device.
 *
 * ** Note: Homing is optional and not required in this operational mode or in
 *    this example.
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


// Set the operational mode of this example:
// If true, the motor will faithfully follow the encoder's position.
// If false, the motor will faithfully follow the encoder's velocity instead.
bool followPosition = false;

// Define the velocity and acceleration limits to be used for positional moves
int32_t velocityLimit = 100000; // pulses per sec
int32_t accelerationLimit = 1000000; // pulses per sec^2

// Set to true if the sense of encoder direction should be inverted.
bool swapDirection = false;
// Set to true if index detection should occur on the falling edge,
// rather than the rising edge.
bool indexInverted = false;

int main(void) {
    // Set up serial communication and wait up to 5 seconds for a port to open
    uint32_t timeout = 5000;
    uint32_t startTime = Milliseconds();
    ConnectorUsb.PortOpen();
    while (!ConnectorUsb && Milliseconds() - startTime < timeout) {
        continue;
    }

    // Enable the encoder input feature
    EncoderIn.Enable(true);
    // Zero the position to start
    EncoderIn.Position(0);
    // Set the encoder direction
    EncoderIn.SwapDirection(swapDirection);
    // Set the sense of index detection (true = rising edge, false = falling edge)
    EncoderIn.IndexInverted(indexInverted);

    // Motor setup:
    MotorMgr.MotorModeSet(MotorManager::MOTOR_ALL, Connector::CPM_MODE_STEP_AND_DIR);
    ConnectorM0.VelMax(velocityLimit);
    ConnectorM0.AccelMax(accelerationLimit);

    // Enables the motor; homing will begin automatically if enabled
    ConnectorM0.EnableRequest(true);
    ConnectorUsb.SendLine("Motor Enabled");

    // Waits for HLFB to assert (waits for homing to complete if applicable)
    ConnectorUsb.SendLine("Waiting for HLFB...");
    while (ConnectorM0.HlfbState() != MotorDriver::HLFB_ASSERTED) {
        continue;
    }
    ConnectorUsb.SendLine("Motor Ready");

    // Variables to store encoder state
    int32_t position = 0;
    int32_t velocity = 0;
    int32_t indexPosition = 0;
    int32_t lastIndexPosition = 0;
    bool quadratureError = false;

    // Use a timeout to print out encoder information every 500 ms.
    char info[100];
    timeout = 500;
    startTime = Milliseconds();

    while (!quadratureError) {
        position = EncoderIn.Position();
        velocity = EncoderIn.Velocity();
        indexPosition = EncoderIn.IndexPosition();
        quadratureError = EncoderIn.QuadratureError();

        // Print out encoder info at a fixed timeout rate
        if (Milliseconds() - startTime >= timeout) {
            if (followPosition) {
                snprintf(info, 100, "Encoder position: %ld counts", position);
            }
            else {
                snprintf(info, 100, "Encoder velocity: %ld counts/sec", velocity);
            }
            ConnectorUsb.SendLine(info);
            startTime = Milliseconds();
        }

        if (indexPosition != lastIndexPosition) {
            snprintf(info, 100, "Detected index at position: %ld",
            EncoderIn.IndexPosition());
            ConnectorUsb.SendLine(info);
        }

        lastIndexPosition = indexPosition;

        if (followPosition) {
            // Move the motor to the current position read by the encoder
            ConnectorM0.Move(position, MotorDriver::MOVE_TARGET_ABSOLUTE);
        }
        else {
            // Command the motor to follow the encoder's velocity
            ConnectorM0.MoveVelocity(velocity);
        }
    }

    // We detected a quadrature error!
    ConnectorM0.MoveVelocity(0);
    ConnectorM0.EnableRequest(false);
    ConnectorUsb.SendLine("Quadrature error detected. Stopping motion...");
}
