/*
 * Title: MovementTest
 *
 * Objective:
 *    Provide a test platform for various types of movement. Asserts that 
 *    movements are performed as intended.
 *
 * Description:
 *    Sends random inputs to various Move commands. Performs a mix of absolute,
 *    relative and immediate position moves and velocity moves. Calls them with
 *    some predetermined and some random inputs. Asserts position and 
 *    velocity against reported values. NOTE: This is self testing and movement
 *    should be verified by a 3rd party. NOTE: This does not check continuity 
 *    of velocity nor acceleration limits.
 *
 * Requirements:
 * 1. A motor capable of step and direction must be connected to Connector M-0.
 * 2. The motor may optionally be connected to the MotorDriver's HLFB line if
 *    the motor has a "servo on" type feature.
 *
 * Last Modified: 1/21/2020
 * Copyright (c) 2020 Teknic Inc. This work is free to use, copy and distribute under the terms of
 * the standard MIT permissive software license which can be found at https://opensource.org/licenses/MIT
 */

#include "Arduino.h"
#include "ClearCore.h"

// Specifies which motor to move.
// Options are: ConnectorM0, ConnectorM1, ConnectorM2, or ConnectorM3.
#define motor ConnectorM1

// Select the baud rate to match the target serial device
#define baudRate 115200

// Define the velocity and acceleration limits to be used for each move
int velocityLimit = 10000; // pulses per sec
int accelerationLimit = 100000; // pulses per sec^2

// Declares our user-defined helper function, which is used to command moves to
// the motor. The definition/implementation of this function is at the  bottom
// of the example
void MoveDistanceImmediate(int distance);
void MoveDistance(int distance);

void WaitForMotorStop();
void WaitForMotorCruise();
void AssertPosition(int32_t targetPos);
void AssertMoveStatus(bool expected, bool actual);
void AssertVelocity(int32_t targetVel);

void setup() {
    // Put your setup code here, it will only run once:

    // Sets the input clocking rate. This normal rate is ideal for ClearPath
    // step and direction applications.
    MotorMgr.MotorInputClocking(MotorManager::CLOCK_RATE_NORMAL);

    // Sets all motor connectors into step and direction mode.
    MotorMgr.MotorModeSet(MotorManager::MOTOR_ALL,
    Connector::CPM_MODE_STEP_AND_DIR);

    // Sets the maximum velocity for each move
    motor.VelMax(velocityLimit);

    // Set the maximum acceleration for each move
    motor.AccelMax(accelerationLimit);

    // Sets up serial communication and waits up to 5 seconds for a port to open.
    //   Serial communication is not required for this example to run.
      Serial.ttl(false);
      Serial.begin(baudRate);
    uint32_t timeout = 5000;
    uint32_t startTime = millis();
    while (!  Serial && millis() - startTime < timeout) {
        continue;
    }

    // Enables the motor; homing will begin automatically if enabled
    motor.EnableRequest(true);

    bool en = motor.EnableRequest();
    
    if (en) {
          Serial.println("Motor Enabled");
        } else {
          Serial.println("Motor Not Enabled");
    }
    
    if (motor.IsInHwFault()) {
        // Motor is in a fault state
          Serial.println("Motor is in a fault state, test failure");
        // test failed, block
        while(true) { continue; }
    }

      Serial.print("Motor Status Reg: ");
      Serial.println( motor.StatusReg().reg);

    // Waits for HLFB to assert (waits for homing to complete if applicable)
      Serial.println("Waiting for HLFB...");
    startTime = millis();
    timeout = 3600;
    while (motor.HlfbState() != MotorDriver::HLFB_ASSERTED) {
        if ( millis() - startTime > timeout ){
              Serial.printtln("Timeout waiting for HLFB, Test Setup Failure");
            // test failed, block
            while(true) { continue; }
        }
        continue;
    }
      Serial.println("Motor Ready");
}

int testNum = 0;
void loop() {
    // Put your main code here, it will run repeatedly:
    
    int32_t targetPos = 0;
    int32_t oldTargetPos;
    bool expectedMoveStatus = true;
    bool actualMoveStatus;

    bool absolute;
    bool immediate;

    // Which tests and how many to run
    bool testAbsolute = true;
    int  absoluteTestsNum = 10;
    bool testRelative = true;
    int  relRandTestsNum = 100;
    bool testVelocity = true;
    int  velocitySimpleTestsNum = 10;
    int  velocityInterruptTestsNum = 50;
    bool testAbThenRel = true;
    int  absoluteThenRelTestsNum = 10;
    bool testRelThenAb = true;
    int  relativeThenAbTestsNum = 10;
    bool testRelThenVel = true;
    int  relThenVelTestsNum = 10;
    bool testVelThenRel = true;
    int  velocityThenRelTestsNum = 25;
    bool testVelThenAb = true;
    int  velocityThenAbTestsNum = 10;

    bool pauseAfterTests = false;


    //int32_t velocityMove = 25695;//random(-50000,50000);
    //int32_t relativeMove = 3800;//4049;//random(-50000,50000);
    //int32_t delayBetweenMoves = 327;//random(5000);
//
    //motor.MoveVelocity(velocityMove);
    //delay(delayBetweenMoves);
//
    //int32_t startPos = motor.PositionRefCommanded();
    //int32_t targetPos = startPos + relativeMove;
    //motor.Move(relativeMove, false, true);
//
//
    //while (true) {
        //delay(100);
    //}

    //motor.MoveVelocity(50000);
    //delay(2000);
    //motor.MoveStopDecel(1000000);
    //while (true) {
        //delay(100);
    //}

    ////////////////////// Absolute Moves ///////////////////////////////////
    if (testAbsolute){
          Serial.println("Testing Absolute Position Moves...");

          Serial.println("  Non-interrupted Moves");

        absolute = true;
        immediate = true;

        targetPos = 5000;
        actualMoveStatus = motor.Move(targetPos, absolute, immediate);
        WaitForMotorStop();
        AssertMoveStatus(actualMoveStatus,expectedMoveStatus);
        AssertPosition(targetPos);
        delay(500);

        targetPos = 0;
        actualMoveStatus = motor.Move(targetPos, absolute, immediate);
        WaitForMotorStop();
        AssertMoveStatus(actualMoveStatus,expectedMoveStatus);
        AssertPosition(targetPos);
        delay(500);

        targetPos = -10000;
        actualMoveStatus = motor.Move(targetPos, absolute, immediate);
        WaitForMotorStop();
        AssertMoveStatus(actualMoveStatus,expectedMoveStatus);
        AssertPosition(targetPos);
        delay(500);

        targetPos = -2000;
        actualMoveStatus = motor.Move(targetPos, absolute, immediate);
        WaitForMotorStop();
        AssertMoveStatus(actualMoveStatus,expectedMoveStatus);
        AssertPosition(targetPos);
        delay(500);

        targetPos = -2100;
        actualMoveStatus = motor.Move(targetPos, absolute, immediate);
        WaitForMotorStop();
        AssertMoveStatus(actualMoveStatus,expectedMoveStatus);
        AssertPosition(targetPos);
        delay(500);

        targetPos = -2000;
        actualMoveStatus = motor.Move(targetPos, absolute, immediate);
        WaitForMotorStop();
        AssertMoveStatus(actualMoveStatus,expectedMoveStatus);
        AssertPosition(targetPos);
        delay(500);

          Serial.println("  ... Interrupted Moves");

        targetPos = 0;
        actualMoveStatus = motor.Move(targetPos, absolute, immediate);
        WaitForMotorStop();

        int32_t absoluteMove1;
        int32_t absoluteMove2;
        int delayBetweenMoves;
        absolute = true;
        immediate = true;
        for (int i = 0; i < absoluteTestsNum; i++){
            absoluteMove1 = random(-50000,50000);
            absoluteMove2 = random(-50000,50000);
            delayBetweenMoves = random(1000);

            oldTargetPos = targetPos;
            targetPos = absoluteMove2;

              Serial.print("  Random Relative Position Move Test ");
              Serial.println(testNum);
              Serial.print("    Starting Position: ");
              Serial.println(oldTargetPos);
              Serial.print("    Absolute Move 1: ");
              Serial.println(absoluteMove1);
              Serial.print("    Absolute Move 2: ");
              Serial.println(absoluteMove2);
              Serial.print("    Time Between: ");
              Serial.println(delayBetweenMoves);
              Serial.print("    Final Position: ");
              Serial.println(targetPos);
            
            actualMoveStatus = motor.Move(absoluteMove1, absolute, immediate);
            AssertMoveStatus(actualMoveStatus,expectedMoveStatus);
            delay(delayBetweenMoves);
            actualMoveStatus = motor.Move(absoluteMove2, absolute, immediate);
            AssertMoveStatus(actualMoveStatus,expectedMoveStatus);
            WaitForMotorStop();
            AssertPosition(targetPos);
            delay(100);
        }

          Serial.println("  ... Testing Rejection of moves");
        immediate = false;
        // Recenter
        targetPos = 0;
        actualMoveStatus = motor.Move(targetPos, absolute, immediate);
        WaitForMotorStop();
        delay(500);

        targetPos = 30000;
        actualMoveStatus = motor.Move(targetPos, absolute, immediate);
        AssertMoveStatus(actualMoveStatus,expectedMoveStatus);
        delay(500);
        expectedMoveStatus = false;
        actualMoveStatus = motor.Move(0, absolute, immediate);
        WaitForMotorStop();
        AssertMoveStatus(actualMoveStatus,expectedMoveStatus);
        AssertPosition(targetPos);
        delay(500);

          Serial.println("Finished Testing Absolute Position Moves.");
        } else {
          Serial.println("Skipping Absolute Position Moves.");
    }

    ////////////////////// Relative Moves ///////////////////////////////////
    if (testRelative){
          Serial.println("Testing Relative Position Moves...");

        int32_t relativeMove = 1000;
        absolute = true;
        immediate = true;

        expectedMoveStatus = true;
        // Zero
        targetPos = 0;
        actualMoveStatus = motor.Move(targetPos, absolute, immediate);
        WaitForMotorStop();
        delay(500);

        absolute = false;
        immediate = true;

        targetPos += relativeMove;
        actualMoveStatus = motor.Move(relativeMove, absolute, immediate);
        WaitForMotorStop();
        AssertMoveStatus(actualMoveStatus,expectedMoveStatus);
        AssertPosition(targetPos);
        delay(500);

        relativeMove = -2000;
        targetPos += relativeMove;
        actualMoveStatus = motor.Move(relativeMove, absolute, immediate);
        WaitForMotorStop();
        AssertMoveStatus(actualMoveStatus,expectedMoveStatus);
        AssertPosition(targetPos);
        delay(500);

        relativeMove = -10000;
        targetPos += relativeMove;
        actualMoveStatus = motor.Move(relativeMove, absolute, immediate);
        WaitForMotorStop();
        AssertMoveStatus(actualMoveStatus,expectedMoveStatus);
        AssertPosition(targetPos);
        delay(500);


        relativeMove = 7000;
        targetPos += relativeMove;
        actualMoveStatus = motor.Move(relativeMove, absolute, immediate);
        WaitForMotorStop();
        AssertMoveStatus(actualMoveStatus,expectedMoveStatus);
        AssertPosition(targetPos);
        delay(500);

        relativeMove = 38000;
        targetPos += relativeMove;
        actualMoveStatus = motor.Move(relativeMove, absolute, immediate);
        WaitForMotorStop();
        AssertMoveStatus(actualMoveStatus,expectedMoveStatus);
        AssertPosition(targetPos);
        delay(500);

          Serial.println("  ... Interrupted Moves");
        int32_t relativeMove2;
        int delayBetweenMoves;

        immediate = true;
        absolute = false;

        relativeMove2 = 4000;
        relativeMove = -7000;
        delayBetweenMoves = 500;
        targetPos += relativeMove + relativeMove2;
        actualMoveStatus = motor.Move(relativeMove, absolute, immediate);
        AssertMoveStatus(actualMoveStatus,expectedMoveStatus);
        delay(delayBetweenMoves);
        actualMoveStatus = motor.Move(relativeMove2, absolute, immediate);
        AssertMoveStatus(actualMoveStatus,expectedMoveStatus);
        WaitForMotorStop();
        AssertPosition(targetPos);
        delay(500);

        // Test an overshoot
        relativeMove = -1800;//-1817;//-30817;
        relativeMove2 = 1000;//1058;//30058;
        delayBetweenMoves = 110;

        oldTargetPos = targetPos;
        targetPos += relativeMove + relativeMove2;

          Serial.print("  Random Relative Position Move Test ");
          Serial.println(testNum);
          Serial.print("    Starting Position: ");
          Serial.println(oldTargetPos);
          Serial.print("    Rel Move 1: ");
          Serial.println(relativeMove);
          Serial.print("    Rel Move 2: ");
          Serial.println(relativeMove2);
          Serial.print("    Time Between: ");
          Serial.println(delayBetweenMoves);
          Serial.print("    Final Position: ");
          Serial.println(targetPos);
                
        actualMoveStatus = motor.Move(relativeMove, absolute, immediate);
        AssertMoveStatus(actualMoveStatus,expectedMoveStatus);
        delay(delayBetweenMoves);
        actualMoveStatus = motor.Move(relativeMove2, absolute, immediate);
        AssertMoveStatus(actualMoveStatus,expectedMoveStatus);
        WaitForMotorStop();
        AssertPosition(targetPos);
        delay(100);

        // Too many cases to test manually, test with loop and random numbers
        for (int i = 0; i < relRandTestsNum; i++){
            relativeMove  = random(-50000,50000);
            relativeMove2 = random(-50000,50000);
            delayBetweenMoves = random(1000);

            oldTargetPos = targetPos;
            targetPos += relativeMove + relativeMove2;

              Serial.print("  Random Relative Position Move Test ");
              Serial.println(testNum);
              Serial.print("    Starting Position: ");
              Serial.println(oldTargetPos);
              Serial.print("    Rel Move 1: ");
              Serial.println(relativeMove);
              Serial.print("    Rel Move 2: ");
              Serial.println(relativeMove2);
              Serial.print("    Time Between: ");
              Serial.println(delayBetweenMoves);
              Serial.print("    Final Position: ");
              Serial.println(targetPos);
            
            actualMoveStatus = motor.Move(relativeMove, absolute, immediate);
            AssertMoveStatus(actualMoveStatus,expectedMoveStatus);
            delay(delayBetweenMoves);
            actualMoveStatus = motor.Move(relativeMove2, absolute, immediate);
            AssertMoveStatus(actualMoveStatus,expectedMoveStatus);
            WaitForMotorStop();
            AssertPosition(targetPos);
            delay(100);
        }

        
          Serial.println("Finished Testing Relative Position Moves.");
        } else {
          Serial.println("Skipping Absolute Relative Moves.");
    }


    ////////////////////// Velocity Moves ///////////////////////////////////
    if (testVelocity){
          Serial.println("Testing Velocity Moves...");

        int32_t vel1;
        int32_t vel2;
        int delayBetweenMoves;

        int32_t oldVelocity = motor.VelocityRefCommanded();
        
        for (int i = 0; i < velocitySimpleTestsNum; i++){
            vel1 = random(-50000,50000);
            //vel2 = random(-50000,50000);
            //delayBetweenMoves = random(1000);

              Serial.print("  Random Simple Velocity Move Test ");
              Serial.println(testNum);
              Serial.print("    Current Velocity: ");
              Serial.println(oldVelocity);
              Serial.print("    Target Velocity 1:  ");
              Serial.println(vel1);
            
            motor.MoveVelocity(vel1);
            WaitForMotorCruise();
            delay(10);
            AssertVelocity(vel1);
            delay(100);
        }

        for (int i = 0; i < velocityInterruptTestsNum; i++){
            vel1 = random(-50000,50000);
            vel2 = random(-50000,50000);
            delayBetweenMoves = random(1000);

              Serial.print("  Random Interrupt Velocity Move Test ");
              Serial.println(testNum);
              Serial.print("    Current Velocity: ");
              Serial.println(oldVelocity);
              Serial.print("    Target Velocity 1:  ");
              Serial.println(vel1);
              Serial.print("    Target Velocity 2:  ");
              Serial.println(vel2);
              Serial.print("    Delay Between Moves:  ");
              Serial.println(delayBetweenMoves);
            
            motor.MoveVelocity(vel1);
            delay(delayBetweenMoves);
            motor.MoveVelocity(vel2);
            WaitForMotorCruise();
            delay(10);
            AssertVelocity(vel2);
            delay(100);
        }

          Serial.println("Finished Testing Velocity Moves.");
    } else {
          Serial.println("Skipping Velocity Moves.");
    }

    
    //////////////////// Absolute Then Relative ////////////////////////////////
    if (testAbThenRel){
        // Re-zero
        absolute = true;
        immediate = true;
        motor.Move(0, absolute, immediate);
        WaitForMotorStop();
        delay(500);

        int32_t absoluteMove;
        int32_t relativeMove;
        int delayBetweenMoves;

        for (int i = 0; i < absoluteThenRelTestsNum; i++){
            absoluteMove = random(-50000,50000);
            relativeMove = random(-50000,50000);
            delayBetweenMoves = random(1000);

            oldTargetPos = targetPos;
            targetPos = absoluteMove + relativeMove;

              Serial.print("  Random Absolute then Relative Position Move Test ");
              Serial.println(testNum);
              Serial.print("    Starting Position: ");
              Serial.println(oldTargetPos);
              Serial.print("    Absolute Move: ");
              Serial.println(absoluteMove);
              Serial.print("    Relative Move: ");
              Serial.println(relativeMove);
              Serial.print("    Time Between: ");
              Serial.println(delayBetweenMoves);
              Serial.print("    Final Position: ");
              Serial.println(targetPos);
            
            absolute = true;
            actualMoveStatus = motor.Move(absoluteMove, absolute, immediate);
            AssertMoveStatus(actualMoveStatus,expectedMoveStatus);
            delay(delayBetweenMoves);
            absolute = false;
            actualMoveStatus = motor.Move(relativeMove, absolute, immediate);
            AssertMoveStatus(actualMoveStatus,expectedMoveStatus);
            WaitForMotorStop();
            AssertPosition(targetPos);
            delay(100);
        }


          Serial.println("Finished Testing Absolute Into Relative Moves.");
    } else {
          Serial.println("Skipping Absolute Into Relative Moves.");
    }

    //////////////////// Relative then Absolute ////////////////////////////////
    if (testRelThenAb){
          Serial.println("Testing Relative then Absolute Moves...");
        // Re-zero
        absolute = true;
        immediate = true;
        motor.Move(0, absolute, immediate);
        WaitForMotorStop();
        delay(500);

        int32_t absoluteMove;
        int32_t relativeMove;
        int delayBetweenMoves;

        for (int i = 0; i < relativeThenAbTestsNum; i++){
            absoluteMove = random(-50000,50000);
            relativeMove = random(-50000,50000);
            delayBetweenMoves = random(1000);

            oldTargetPos = targetPos;
            targetPos = absoluteMove;

              Serial.print("  Random Absolute then Relative Position Move Test ");
              Serial.println(testNum);
              Serial.print("    Starting Position: ");
              Serial.println(oldTargetPos);
              Serial.print("    Absolute Move: ");
              Serial.println(absoluteMove);
              Serial.print("    Relative Move: ");
              Serial.println(relativeMove);
              Serial.print("    Time Between: ");
              Serial.println(delayBetweenMoves);
              Serial.print("    Final Position: ");
              Serial.println(targetPos);
            
            absolute = false;
            actualMoveStatus = motor.Move(relativeMove, absolute, immediate);
            AssertMoveStatus(actualMoveStatus,expectedMoveStatus);
            delay(delayBetweenMoves);
            absolute = true;
            actualMoveStatus = motor.Move(absoluteMove, absolute, immediate);
            AssertMoveStatus(actualMoveStatus,expectedMoveStatus);
            WaitForMotorStop();
            AssertPosition(targetPos);
            delay(100);
        }


          Serial.println("Finished Testing Relative Into Absolute Moves.");
    } else {
          Serial.println("Skipping Relative Into Absolute Moves.");
    }

    //////////////////// Relative then Velocity ////////////////////////////////
    if (testRelThenVel){
          Serial.println("Testing Relative then Velocity Moves...");
        // Re-zero
        absolute = true;
        immediate = true;
        motor.Move(0, absolute, immediate);
        WaitForMotorStop();
        delay(500);

        int32_t velocityMove;
        int32_t relativeMove;
        int delayBetweenMoves;

        for (int i = 0; i < relThenVelTestsNum; i++){
            velocityMove = random(-50000,50000);
            relativeMove = random(-50000,50000);
            delayBetweenMoves = random(1000);


              Serial.print("  Random Relative then Velocity Move Test ");
              Serial.println(testNum);
              Serial.print("    Starting Position: ");
              Serial.println(oldTargetPos);
              Serial.print("    Velocity Move: ");
              Serial.println(velocityMove);
              Serial.print("    Relative Move: ");
              Serial.println(relativeMove);
              Serial.print("    Time Between: ");
              Serial.println(delayBetweenMoves);
                
            absolute = false;
            actualMoveStatus = motor.Move(relativeMove, absolute, immediate);
            AssertMoveStatus(actualMoveStatus,expectedMoveStatus);
            delay(delayBetweenMoves);
            absolute = true;

            motor.MoveVelocity(velocityMove);
            WaitForMotorCruise();
            AssertVelocity(velocityMove);
            delay(100);

            motor.MoveVelocity(0);
            WaitForMotorStop();
            delay(100);
        }


          Serial.println("Finished Testing Relative Into Velocity Moves.");
    } else {
          Serial.println("Skipping Relative Into Velocity Moves.");
    }
    

    //////////////////// Velocity Then Relative ////////////////////////////////
    if (testVelThenRel){
          Serial.println("Testing Velocity Then Relative Moves...");
        
        // Re-zero
        //absolute = true;
        //immediate = true;
        //expectedMoveStatus = true;
        //motor.Move(0, absolute, immediate);
        //WaitForMotorStop();
        delay(500);

        absolute = false;
        immediate = true;

        int32_t velocityMove;
        int32_t relativeMove;
        int32_t startPos;
        int delayBetweenMoves;

        for (int i = 0; i < velocityThenRelTestsNum; i++){
            velocityMove = random(-50000,50000);
            relativeMove = random(-50000,50000);
            delayBetweenMoves = random(5000);


              Serial.print("  Random Velocity then Relative Move Test ");
              Serial.println(testNum);
              Serial.print("    Velocity Move: ");
              Serial.println(velocityMove);
              Serial.print("    Relative Move: ");
              Serial.println(relativeMove);
              Serial.print("    Time Between: ");
              Serial.println(delayBetweenMoves);
            

            motor.MoveVelocity(velocityMove);
            delay(delayBetweenMoves);

            startPos = motor.PositionRefCommanded();
            targetPos = startPos + relativeMove;
            actualMoveStatus = motor.Move(relativeMove, absolute, immediate);
            AssertMoveStatus(actualMoveStatus,expectedMoveStatus);

              Serial.print("    Starting Position: ");
              Serial.println(startPos);
              Serial.print("    Target Position: ");
              Serial.println(targetPos);

            WaitForMotorStop();
            AssertPosition(targetPos);
            delay(100);


            

            motor.MoveVelocity(0);
            WaitForMotorStop();
            delay(100);
        }

          Serial.println("Finished Testing Velocity Then Relative Moves.");
    } else {
          Serial.println("Skipping Velocity Then Relative Moves.");
    }


    
    //////////////////// Velocity Then Absolute ////////////////////////////////
    if (testVelThenAb){
          Serial.println("Testing Velocity Then Absolute Moves...");
        // Re-zero
        absolute = true;
        immediate = true;
        expectedMoveStatus = true;
        motor.Move(0, absolute, immediate);
        WaitForMotorStop();
        delay(500);

        int32_t velocityMove;
        int32_t absoluteMove;
        int delayBetweenMoves;

        for (int i = 0; i < velocityThenAbTestsNum; i++){
            velocityMove = random(-50000,50000);
            absoluteMove = random(-50000,50000);
            delayBetweenMoves = random(5000);


              Serial.print("  Random Velocity then Absolute Move Test ");
              Serial.println(testNum);
              Serial.print("    Velocity Move: ");
              Serial.println(velocityMove);
              Serial.print("    Absolute Move: ");
              Serial.println(absoluteMove);
              Serial.print("    Time Between: ");
              Serial.println(delayBetweenMoves);
            

            motor.MoveVelocity(velocityMove);
            delay(delayBetweenMoves);

            targetPos = absoluteMove;
            actualMoveStatus = motor.Move(absoluteMove, absolute, immediate);
            AssertMoveStatus(actualMoveStatus,expectedMoveStatus);
            WaitForMotorStop();
            AssertPosition(targetPos);
            delay(100);


            

            motor.MoveVelocity(0);
            WaitForMotorStop();
            delay(100);
        }


          Serial.println("Finished Testing Velocity Then Absolute Moves.");
    } else {
          Serial.println("Skipping Velocity Then Absolute Moves.");
    }

    // Tests done, move to zero a wait.
      Serial.println("Motion Tests Finished");
    motor.Move(0, true, true);
    WaitForMotorStop();
    while(pauseAfterTests){
        delay(100);
    }

}




void WaitForMotorStop(){
    while (!motor.StepsComplete()) {
        continue;
    }
}

void WaitForMotorCruise(){
    while (!motor.CruiseVelocityReached() && !motor.StepsComplete()) {
        continue;
    }
}

void AssertPosition(int32_t targetPos){
    int32_t measuredPos = motor.PositionRefCommanded();
    if (measuredPos != targetPos) {

          Serial.print("Motor Position Assert Failed. Test Num ");
          Serial.println(testNum);
          Serial.print("    Desired Position: ");
          Serial.println(targetPos);
          Serial.print("    Actual Position:  ");
          Serial.println(measuredPos);

        while (true){
            delay(100);
        }
    }
      Serial.print("  Position Test Finished:");
      Serial.println(testNum);
    testNum++;
}


void AssertMoveStatus(bool expected, bool actual){
    if (expected != actual) {

          Serial.println("Move Status Assert Failed");
          Serial.print("    Expected Position: ");
          Serial.println(expected);
          Serial.print("    Actual Position:  ");
          Serial.println(actual);

        while (true){
            delay(100);
        }
    }
}

void AssertVelocity(int32_t targetVel){
    int32_t measuredVel = motor.VelocityRefCommanded();
    if (measuredVel != targetVel) {
          Serial.print("Motor Velocity Assert Failed. Test Num ");
          Serial.println(testNum);
          Serial.print("    Desired Velocity: ");
          Serial.println(targetVel);
          Serial.print("    Actual Velocity:  ");
          Serial.println(measuredVel);

        while (true){
            delay(100);
        }
    }
      Serial.print("  Veloctity Test Finished:");
      Serial.println(testNum);
    testNum++;
}





void MoveDistanceImmediate(int distance);
void MoveDistance(int distance);

//void loop() {
    //// Put your main code here, it will run repeatedly:
    //// Move back 12800 counts (negative direction), then wait 2000ms
    //MoveDistanceImmediate(-6400);
    //delay(2000);
    //MoveDistanceImmediate(6400);
    //delay(2000);
    ////MoveDistance(-6400);
    ////delay(2000);
    ////MoveDistance(6400);
    ////delay(2000);
//}
/*------------------------------------------------------------------------------
 * MoveDistanceImmediate
 *
 *    Command "distance" number of step pulses away from the current position
 *    Prints the move status to the USB serial port
 *    Returns when HLFB asserts (indicating the motor has reached the commanded
 *    position)
 *
 * Parameters:
 *    int distance  - The distance, in step pulses, to move
 *
 * Returns: None
 */
void MoveDistanceImmediate(int distance) {
    bool status;
    Serial.print("Moving distance: ");
    Serial.println(distance);
    // Command the move of incremental distance
    status = motor.Move(distance, false, true);
    Serial.print("Moving..");
    Serial.println(status);
    // Uncomment to pause between moves
//    Serial.println("Moving.. Waiting for HLFB");
    //while (!motor.StepsComplete() || motor.HlfbState() != MotorDriver::HLFB_ASSERTED) {
        //continue;
    //}
    delay(250);
    status = motor.Move(distance*2, false, true);
    Serial.print("Move interrupted (exptected = 1): ");
    Serial.println(status);
    // Waits for HLFB to assert (signaling the move has successfully completed)
    Serial.println("Moving.. Waiting for HLFB");
    while (!motor.StepsComplete() || motor.HlfbState() != MotorDriver::HLFB_ASSERTED) {
        continue;
    }
}
void MoveDistance(int distance) {
    Serial.print("Moving distance: ");
    Serial.println(distance);
    // Command the move of incremental distance
    motor.Move(distance);
    // Waits for HLFB to assert (signaling the move has successfully completed)
    Serial.println("Moving.. Waiting for HLFB");
    while (!motor.StepsComplete() || motor.HlfbState() != MotorDriver::HLFB_ASSERTED) {
        continue;
    }
    Serial.println("Move Done");
}
//------------------------------------------------------------------------------