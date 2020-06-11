/**
    \file StepGenTest.cpp

    Created: 9/17/2019 2:26:10 PM
**/

#include "CppUTest/TestHarness.h"
//#include "CppUTest/PlatformSpecificFunctions_c.H"
#include "testHooks.h"

TEST_GROUP(StepGenTest) {
    void setup() {
        TestIO::ManualRefresh(false);
        MotorMgr.Initialize();
        MotorMgr.MotorModeSet(MotorManager::MOTOR_M0M1, Connector::CPM_MODE_STEP_AND_DIR);
        MotorMgr.MotorModeSet(MotorManager::MOTOR_M2M3, Connector::CPM_MODE_STEP_AND_DIR);
    }

    void teardown() {
        MotorMgr.Initialize();
        TestIO::ManualRefresh(false);
    }
};

TEST(StepGenTest, InitialState) {
    LONGS_EQUAL(Connector::ConnectorModes::CPM_MODE_STEP_AND_DIR, ConnectorM0.Mode());
    LONGS_EQUAL(Connector::ConnectorModes::CPM_MODE_STEP_AND_DIR, ConnectorM1.Mode());
    LONGS_EQUAL(Connector::ConnectorModes::CPM_MODE_STEP_AND_DIR, ConnectorM2.Mode());
    LONGS_EQUAL(Connector::ConnectorModes::CPM_MODE_STEP_AND_DIR, ConnectorM3.Mode());
}

extern "C" int debugPutChar(int c);

void TEST_MOVE(int32_t dist, int32_t accLim, int32_t velLim) {
    TestIO::ManualRefresh(true);
    int32_t totalSteps = 0;
    int32_t lastVel, accel;
    int32_t velMax = 0;
    int32_t accelMax = 0;

    ConnectorM0.VelMax(velLim);
    ConnectorM0.AccelMax(accLim);

    if (dist) {
        CHECK_TRUE(ConnectorM0.Move(dist));
    }
    lastVel = TestIO::StepGenVel(ConnectorM0);
    LONGS_EQUAL(0, TestIO::StepGenPosn(ConnectorM0));
    LONGS_EQUAL(0, lastVel);
    accel = 0;

    while (!ConnectorM0.StepsComplete()) {
        velMax = max(velMax, lastVel);
        if (lastVel) {
            accelMax = max(accelMax, accel);
        }
        else {
            // The tail of the move can have a small truncation due to the
            // quantization of the accel and velocity.
            // The area of the truncated move tail (accel^2/(2*AccLim)) <= VelLim/AccLim
            if (accel > TestIO::StepGenAccLim(ConnectorM0)) {
                int32_t truncation = accel - TestIO::StepGenAccLim(ConnectorM0);
                CHECK_COMPARE(truncation * truncation / 2, <=, TestIO::StepGenVelLim(ConnectorM0));
            }
        }
        totalSteps += TestIO::StepGenUpdate(ConnectorM0);
        accel = labs(lastVel - TestIO::StepGenVel(ConnectorM0));
        lastVel = TestIO::StepGenVel(ConnectorM0);
    }

    CHECK_COMPARE(velMax, <=, TestIO::StepGenVelLim(ConnectorM0));
    CHECK_COMPARE(accelMax, <=, TestIO::StepGenAccLim(ConnectorM0));
    LONGS_EQUAL(labs(dist), totalSteps);
    CHECK_COMPARE(dist < 0, ==, ConnectorM0.MotorInAState());

}

TEST(StepGenTest, Triangle1) {
    TEST_MOVE(10000, 10000, 10000);
}

TEST(StepGenTest, NegTriangle1) {
    TEST_MOVE(-10000, 10000, 10000);
}

TEST(StepGenTest, Triangle2) {
    TEST_MOVE(1500, 10000, 10000);
}

TEST(StepGenTest, Trap1) {
    TEST_MOVE(25000, 10000, 10000);
}

TEST(StepGenTest, Trap2) {
    TEST_MOVE(10000, 1000000, 10000);
}

TEST(StepGenTest, LongNegMove) {
    MotorMgr.MotorInputClocking(MotorManager::CLOCK_RATE_HIGH);
    TEST_MOVE(-INT32_MAX, INT32_MAX, 2000000);
}

TEST(StepGenTest, ZeroMove) {
    TEST_MOVE(0, 1000000, 2000000);
}

TEST(StepGenTest, LowAccel) {
    TEST_MOVE(100, 10, 200);
}

TEST(StepGenTest, HighAccel) {
    TEST_MOVE(100000, INT32_MAX, 100000);
}

TEST(StepGenTest, LongMove) {
    MotorMgr.MotorInputClocking(MotorManager::CLOCK_RATE_HIGH);
    TEST_MOVE(INT32_MAX, INT32_MAX, 2000000);
}