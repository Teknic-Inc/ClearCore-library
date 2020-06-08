/*
 * MtrTest.cpp
 *
 * Created: 9/13/2019 2:21:21 PM
 */
#include "CppUTest/TestHarness.h"
#include "testHooks.h"

TEST_GROUP(MtrTest) {
    void setup() {
        TestIO::ManualRefresh(false);
        TestIO::UseFakeInputs(false);
        ConnectorM0.Reinitialize();
        ConnectorM1.Reinitialize();
        ConnectorM2.Reinitialize();
        ConnectorM3.Reinitialize();
        MotorMgr.Initialize();
    }

    void teardown() {
        TestIO::ManualRefresh(false);
        TestIO::UseFakeInputs(false);
        ConnectorM0.Reinitialize();
        ConnectorM1.Reinitialize();
        ConnectorM2.Reinitialize();
        ConnectorM3.Reinitialize();
        MotorMgr.Initialize();
    }
};

TEST(MtrTest, InitialState) {
    LONGS_EQUAL(Connector::ConnectorModes::CPM_MODE_A_DIRECT_B_DIRECT, ConnectorM0.Mode());
    LONGS_EQUAL(Connector::ConnectorModes::CPM_MODE_A_DIRECT_B_DIRECT, ConnectorM1.Mode());
    LONGS_EQUAL(Connector::ConnectorModes::CPM_MODE_A_DIRECT_B_DIRECT, ConnectorM2.Mode());
    LONGS_EQUAL(Connector::ConnectorModes::CPM_MODE_A_DIRECT_B_DIRECT, ConnectorM3.Mode());
    LONGS_EQUAL(Connector::CPM_TYPE, ConnectorM0.Type());
    LONGS_EQUAL(Connector::CPM_TYPE, ConnectorM1.Type());
    LONGS_EQUAL(Connector::CPM_TYPE, ConnectorM2.Type());
    LONGS_EQUAL(Connector::CPM_TYPE, ConnectorM3.Type());
}

TEST(MtrTest, ChangeModes01) {
    MotorMgr.MotorModeSet(MotorManager::MOTOR_M0M1, Connector::CPM_MODE_STEP_AND_DIR);
    LONGS_EQUAL(Connector::ConnectorModes::CPM_MODE_STEP_AND_DIR, ConnectorM0.Mode());
    LONGS_EQUAL(Connector::ConnectorModes::CPM_MODE_STEP_AND_DIR, ConnectorM1.Mode());
    LONGS_EQUAL(Connector::ConnectorModes::CPM_MODE_A_DIRECT_B_DIRECT, ConnectorM2.Mode());
    LONGS_EQUAL(Connector::ConnectorModes::CPM_MODE_A_DIRECT_B_DIRECT, ConnectorM3.Mode());
    MotorMgr.MotorModeSet(MotorManager::MOTOR_M0M1, Connector::CPM_MODE_A_DIRECT_B_PWM);
    LONGS_EQUAL(Connector::ConnectorModes::CPM_MODE_A_DIRECT_B_PWM, ConnectorM0.Mode());
    LONGS_EQUAL(Connector::ConnectorModes::CPM_MODE_A_DIRECT_B_PWM, ConnectorM1.Mode());
    LONGS_EQUAL(Connector::ConnectorModes::CPM_MODE_A_DIRECT_B_DIRECT, ConnectorM2.Mode());
    LONGS_EQUAL(Connector::ConnectorModes::CPM_MODE_A_DIRECT_B_DIRECT, ConnectorM3.Mode());
    MotorMgr.MotorModeSet(MotorManager::MOTOR_M0M1, Connector::CPM_MODE_A_PWM_B_PWM);
    LONGS_EQUAL(Connector::ConnectorModes::CPM_MODE_A_PWM_B_PWM, ConnectorM0.Mode());
    LONGS_EQUAL(Connector::ConnectorModes::CPM_MODE_A_PWM_B_PWM, ConnectorM1.Mode());
    LONGS_EQUAL(Connector::ConnectorModes::CPM_MODE_A_DIRECT_B_DIRECT, ConnectorM2.Mode());
    LONGS_EQUAL(Connector::ConnectorModes::CPM_MODE_A_DIRECT_B_DIRECT, ConnectorM3.Mode());
}

TEST(MtrTest, ChangeModes23) {
    MotorMgr.MotorModeSet(MotorManager::MOTOR_M2M3, Connector::CPM_MODE_STEP_AND_DIR);
    LONGS_EQUAL(Connector::ConnectorModes::CPM_MODE_A_DIRECT_B_DIRECT, ConnectorM0.Mode());
    LONGS_EQUAL(Connector::ConnectorModes::CPM_MODE_A_DIRECT_B_DIRECT, ConnectorM1.Mode());
    LONGS_EQUAL(Connector::ConnectorModes::CPM_MODE_STEP_AND_DIR, ConnectorM2.Mode());
    LONGS_EQUAL(Connector::ConnectorModes::CPM_MODE_STEP_AND_DIR, ConnectorM3.Mode());
    MotorMgr.MotorModeSet(MotorManager::MOTOR_M2M3, Connector::CPM_MODE_A_DIRECT_B_PWM);
    LONGS_EQUAL(Connector::ConnectorModes::CPM_MODE_A_DIRECT_B_DIRECT, ConnectorM0.Mode());
    LONGS_EQUAL(Connector::ConnectorModes::CPM_MODE_A_DIRECT_B_DIRECT, ConnectorM1.Mode());
    LONGS_EQUAL(Connector::ConnectorModes::CPM_MODE_A_DIRECT_B_PWM, ConnectorM2.Mode());
    LONGS_EQUAL(Connector::ConnectorModes::CPM_MODE_A_DIRECT_B_PWM, ConnectorM3.Mode());
    MotorMgr.MotorModeSet(MotorManager::MOTOR_M2M3, Connector::CPM_MODE_A_PWM_B_PWM);
    LONGS_EQUAL(Connector::ConnectorModes::CPM_MODE_A_DIRECT_B_DIRECT, ConnectorM0.Mode());
    LONGS_EQUAL(Connector::ConnectorModes::CPM_MODE_A_DIRECT_B_DIRECT, ConnectorM1.Mode());
    LONGS_EQUAL(Connector::ConnectorModes::CPM_MODE_A_PWM_B_PWM, ConnectorM2.Mode());
    LONGS_EQUAL(Connector::ConnectorModes::CPM_MODE_A_PWM_B_PWM, ConnectorM3.Mode());
}

TEST(MtrTest, ChangeModesInvalid) {
    MotorMgr.MotorModeSet(MotorManager::MOTOR_M2M3, Connector::CPM_MODE_STEP_AND_DIR);
    LONGS_EQUAL(Connector::ConnectorModes::CPM_MODE_A_DIRECT_B_DIRECT, ConnectorM0.Mode());
    LONGS_EQUAL(Connector::ConnectorModes::CPM_MODE_A_DIRECT_B_DIRECT, ConnectorM1.Mode());
    LONGS_EQUAL(Connector::ConnectorModes::CPM_MODE_STEP_AND_DIR, ConnectorM2.Mode());
    LONGS_EQUAL(Connector::ConnectorModes::CPM_MODE_STEP_AND_DIR, ConnectorM3.Mode());
    MotorMgr.MotorModeSet(MotorManager::MOTOR_M2M3, Connector::INPUT_DIGITAL);  // Invalid
    LONGS_EQUAL(Connector::ConnectorModes::CPM_MODE_A_DIRECT_B_DIRECT, ConnectorM0.Mode());
    LONGS_EQUAL(Connector::ConnectorModes::CPM_MODE_A_DIRECT_B_DIRECT, ConnectorM1.Mode());
    LONGS_EQUAL(Connector::ConnectorModes::CPM_MODE_STEP_AND_DIR, ConnectorM2.Mode());
    LONGS_EQUAL(Connector::ConnectorModes::CPM_MODE_STEP_AND_DIR, ConnectorM3.Mode());
}

static void testHlfbFilter(MotorDriver &mtr, uint16_t len) {
    TestIO::ManualRefresh(true);
    TestIO::InitFakeInput(mtr, true, len);
    TEST_VAL_REFRESH(MotorDriver::HlfbStates::HLFB_ASSERTED, mtr.HlfbState(), 2);
    TestIO::FakeHlfb(mtr, false);
    TEST_VAL_REFRESH(MotorDriver::HlfbStates::HLFB_ASSERTED, mtr.HlfbState(), len);
    TEST_VAL_REFRESH(MotorDriver::HlfbStates::HLFB_DEASSERTED, mtr.HlfbState(), 2);
    TestIO::FakeHlfb(mtr, true);
    TEST_VAL_REFRESH(MotorDriver::HlfbStates::HLFB_DEASSERTED, mtr.HlfbState(), len);
    TEST_VAL_REFRESH(MotorDriver::HlfbStates::HLFB_ASSERTED, mtr.HlfbState(), 2);
}

TEST(MtrTest, initFakeHlfbTrue) {
    TestIO::ManualRefresh(true);
    TestIO::InitFakeInput(ConnectorM0, true);
    TestIO::InitFakeInput(ConnectorM1, true);
    TestIO::InitFakeInput(ConnectorM2, true);
    TestIO::InitFakeInput(ConnectorM3, true);
    CHECK_TRUE(ConnectorM0.HlfbState());
    CHECK_TRUE(ConnectorM1.HlfbState());
    CHECK_TRUE(ConnectorM2.HlfbState());
    CHECK_TRUE(ConnectorM3.HlfbState());
}

TEST(MtrTest, initFakeHlfbFalse) {
    TestIO::ManualRefresh(true);
    TestIO::InitFakeInput(ConnectorM0, false);
    TestIO::InitFakeInput(ConnectorM1, false);
    TestIO::InitFakeInput(ConnectorM2, false);
    TestIO::InitFakeInput(ConnectorM3, false);
    CHECK_FALSE(ConnectorM0.HlfbState());
    CHECK_FALSE(ConnectorM1.HlfbState());
    CHECK_FALSE(ConnectorM2.HlfbState());
    CHECK_FALSE(ConnectorM3.HlfbState());
}

TEST(MtrTest, HlfbFiltering3) {
    testHlfbFilter(ConnectorM0, 3);
}

TEST(MtrTest, HlfbFiltering0) {
    testHlfbFilter(ConnectorM0, 0);
}

TEST(MtrTest, HlfbFiltering1) {
    testHlfbFilter(ConnectorM0, 1);
}

TEST(MtrTest, HlfbFilteringMax) {
    testHlfbFilter(ConnectorM0, UINT16_MAX);
}

TEST(MtrTest, EnableTriggerMultiple) {
    ConnectorM0.EnableRequest(true);
    TestIO::ManualRefresh(true);
    CHECK_TRUE(ConnectorM0.EnableRequest());
    CHECK_TRUE(TestIO::ShifterState(ShiftRegister::SR_EN_OUT_0_MASK));
    SysMgr.SysTickUpdate();
    CHECK_TRUE(ConnectorM0.EnableRequest());
    CHECK_TRUE(TestIO::ShifterState(ShiftRegister::SR_EN_OUT_0_MASK));
    uint8_t pulseLen = 5;
    uint8_t numPulses = 3;
    uint32_t startTime = TimingMgr.Milliseconds();
    ConnectorM0.EnableTriggerPulse(numPulses, pulseLen, false);
    for (int32_t i = 0; i < pulseLen * MS_TO_SAMPLES; i++) {
        // The first pulse may be a fraction of a ms shorter
        if (TimingMgr.Milliseconds() - startTime >= pulseLen) {
            CHECK_COMPARE(i, >, (pulseLen - 1) * MS_TO_SAMPLES);
            break;
        }
        CHECK_TRUE(ConnectorM0.EnableRequest());
        CHECK_FALSE(TestIO::ShifterState(ShiftRegister::SR_EN_OUT_0_MASK));
        SysMgr.FastUpdate();
        SysMgr.SysTickUpdate();
    }
    for (uint32_t i = 0; i < pulseLen * MS_TO_SAMPLES; i++) {
        CHECK_TRUE(ConnectorM0.EnableRequest());
        CHECK_TRUE(TestIO::ShifterState(ShiftRegister::SR_EN_OUT_0_MASK));
        SysMgr.FastUpdate();
        SysMgr.SysTickUpdate();
    }
    for (uint8_t j = 1; j < numPulses; j++) {
        for (uint32_t i = 0; i < pulseLen * MS_TO_SAMPLES; i++) {
            CHECK_TRUE(ConnectorM0.EnableRequest());
            CHECK_FALSE(TestIO::ShifterState(ShiftRegister::SR_EN_OUT_0_MASK));
            SysMgr.FastUpdate();
            SysMgr.SysTickUpdate();
        }
        for (uint32_t i = 0; i < pulseLen * MS_TO_SAMPLES; i++) {
            CHECK_TRUE(ConnectorM0.EnableRequest());
            CHECK_TRUE(TestIO::ShifterState(ShiftRegister::SR_EN_OUT_0_MASK));
            SysMgr.FastUpdate();
            SysMgr.SysTickUpdate();
        }
    }
    CHECK_TRUE(ConnectorM0.EnableRequest());
    CHECK_TRUE(TestIO::ShifterState(ShiftRegister::SR_EN_OUT_0_MASK));
}

TEST(MtrTest, EnableTriggerBlocking) {
    uint32_t pulseLen = 1000;
    uint32_t startTime, endTime;
    const uint32_t timeout = 1000;
    bool didTimeout = false;

    if (ConnectorM0.HlfbState() == MotorDriver::HlfbStates::HLFB_ASSERTED) {
        // Disable the motor
        ConnectorM0.EnableRequest(false);
        // Wait for motor to disable
        startTime = Milliseconds();
        while (!ConnectorM0.HlfbHasFallen() && !didTimeout) {
            if (Milliseconds() - startTime > timeout) {
                didTimeout = true;
            }
            continue;
        }
        CHECK_FALSE(didTimeout);
    }

    // When disabled, check that the trigger pulse function returns immediately
    startTime = Milliseconds();
    ConnectorM0.EnableTriggerPulse(1, pulseLen, true);
    endTime = Milliseconds();
    CHECK_FALSE(ConnectorM0.EnableRequest());
    CHECK_COMPARE((endTime - startTime), <=, 1);

    // Now enable the motor and verify that it blocks appropriately
    ConnectorM0.EnableRequest(true);
    // Wait for motor to enable
    startTime = Milliseconds();
    while (!ConnectorM0.EnableRequest() && !didTimeout) {
        if (Milliseconds() - startTime > timeout) {
            didTimeout = true;
        }
        continue;
    }
    CHECK_FALSE(didTimeout);

    startTime = Milliseconds();
    ConnectorM0.EnableTriggerPulse(1, pulseLen, true);
    endTime = Milliseconds();
    CHECK_TRUE(ConnectorM0.EnableRequest());
    CHECK_COMPARE((endTime - startTime), >=, pulseLen * 2);
    CHECK_COMPARE((endTime - startTime), <=, pulseLen * 2 + 1);
}


TEST(MtrTest, EnableTrigger) {
    ConnectorM0.EnableRequest(true);
    TestIO::ManualRefresh(true);
    CHECK_TRUE(ConnectorM0.EnableRequest());
    CHECK_TRUE(TestIO::ShifterState(ShiftRegister::SR_EN_OUT_0_MASK));
    SysMgr.SysTickUpdate();
    CHECK_TRUE(ConnectorM0.EnableRequest());
    CHECK_TRUE(TestIO::ShifterState(ShiftRegister::SR_EN_OUT_0_MASK));
    uint8_t pulseLen = 5;
    uint32_t startTime = TimingMgr.Milliseconds();
    ConnectorM0.EnableTriggerPulse(1, pulseLen, false);
    for (int32_t i = 0; i < pulseLen * MS_TO_SAMPLES; i++) {
        if (TimingMgr.Milliseconds() - startTime >= pulseLen) {
            CHECK_COMPARE(i, >, (pulseLen - 1) * MS_TO_SAMPLES);
            break;
        }
        CHECK_TRUE(ConnectorM0.EnableRequest());
        CHECK_FALSE(TestIO::ShifterState(ShiftRegister::SR_EN_OUT_0_MASK));
        SysMgr.FastUpdate();
        SysMgr.SysTickUpdate();
    }
    CHECK_TRUE(ConnectorM0.EnableRequest());
    CHECK_TRUE(TestIO::ShifterState(ShiftRegister::SR_EN_OUT_0_MASK));
}