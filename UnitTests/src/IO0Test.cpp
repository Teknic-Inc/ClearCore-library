#include "CppUTest/TestHarness.h"

#include "testHooks.h"


TEST_GROUP(IO0Test) {
    void setup() {
        ConnectorIO0.Reinitialize();
    }

    void teardown() {
        TestIO::ManualRefresh(false);
        ConnectorIO0.Reinitialize();
    }
};

TEST(IO0Test, InitialState) {
    LONGS_EQUAL(Connector::ConnectorModes::INPUT_DIGITAL, ConnectorIO0.Mode());
    LONGS_EQUAL(0, ConnectorIO0.State());
    LONGS_EQUAL(Connector::ANALOG_OUT_DIGITAL_IN_OUT_TYPE, ConnectorIO0.Type());
    CHECK_FALSE(ConnectorIO0.IsWritable());
}

TEST(IO0Test, ModeCheckWithValidModes) {
    TEST_MODE_CHANGE(ConnectorIO0, Connector::ConnectorModes::OUTPUT_DIGITAL);
    CHECK_TRUE(ConnectorIO0.IsWritable());
    TEST_MODE_CHANGE(ConnectorIO0, Connector::ConnectorModes::INPUT_DIGITAL);
    CHECK_FALSE(ConnectorIO0.IsWritable());
    TEST_MODE_CHANGE(ConnectorIO0, Connector::ConnectorModes::OUTPUT_ANALOG);
    CHECK_TRUE(ConnectorIO0.IsWritable());
    TEST_MODE_CHANGE(ConnectorIO0, Connector::ConnectorModes::OUTPUT_PWM);
    CHECK_TRUE(ConnectorIO0.IsWritable());
}

TEST(IO0Test, ModeCheckWithInvalidModes) {
    // Test that changing to invalid modes leaves the mode in INPUT_DIGITAL
    TEST_MODE_CHANGE_FAILS(ConnectorIO0, Connector::ConnectorModes::INVALID_NONE);
    TEST_MODE_CHANGE_FAILS(ConnectorIO0, Connector::ConnectorModes::INPUT_ANALOG);
    TEST_MODE_CHANGE_FAILS(ConnectorIO0, Connector::ConnectorModes::OUTPUT_H_BRIDGE);
    TEST_MODE_CHANGE_FAILS(ConnectorIO0, Connector::ConnectorModes::OUTPUT_TONE);
    TEST_MODE_CHANGE_FAILS(ConnectorIO0, Connector::ConnectorModes::OUTPUT_WAVE);
    TEST_MODE_CHANGE_FAILS(ConnectorIO0, Connector::ConnectorModes::CPM_MODE_A_DIRECT_B_DIRECT);
    TEST_MODE_CHANGE_FAILS(ConnectorIO0, Connector::ConnectorModes::CPM_MODE_STEP_AND_DIR);
    TEST_MODE_CHANGE_FAILS(ConnectorIO0, Connector::ConnectorModes::CPM_MODE_A_DIRECT_B_PWM);
    TEST_MODE_CHANGE_FAILS(ConnectorIO0, Connector::ConnectorModes::CPM_MODE_A_PWM_B_PWM);
    TEST_MODE_CHANGE_FAILS(ConnectorIO0, Connector::ConnectorModes::TTL);
    TEST_MODE_CHANGE_FAILS(ConnectorIO0, Connector::ConnectorModes::RS232);
    TEST_MODE_CHANGE_FAILS(ConnectorIO0, Connector::ConnectorModes::SPI);
    TEST_MODE_CHANGE_FAILS(ConnectorIO0, Connector::ConnectorModes::CCIO);
    TEST_MODE_CHANGE_FAILS(ConnectorIO0, Connector::ConnectorModes::USB_CDC);
}

TEST(IO0Test, ModeCheckWithInvalidModesAsOutputMode) {
    // Change the mode to OUTPUT_DIGITAL
    CHECK_TRUE(ConnectorIO0.Mode(Connector::ConnectorModes::OUTPUT_DIGITAL));
    LONGS_EQUAL(Connector::ConnectorModes::OUTPUT_DIGITAL, ConnectorIO0.Mode());
    // Now test that attempting to change to an invalid mode does not take it out of OUTPUT_DIGITAL
    TEST_MODE_CHANGE_FAILS(ConnectorIO0, Connector::ConnectorModes::INVALID_NONE);
    TEST_MODE_CHANGE_FAILS(ConnectorIO0, Connector::ConnectorModes::INPUT_ANALOG);
    TEST_MODE_CHANGE_FAILS(ConnectorIO0, Connector::ConnectorModes::OUTPUT_H_BRIDGE);
    TEST_MODE_CHANGE_FAILS(ConnectorIO0, Connector::ConnectorModes::OUTPUT_TONE);
    TEST_MODE_CHANGE_FAILS(ConnectorIO0, Connector::ConnectorModes::OUTPUT_WAVE);
    TEST_MODE_CHANGE_FAILS(ConnectorIO0, Connector::ConnectorModes::CPM_MODE_A_DIRECT_B_DIRECT);
    TEST_MODE_CHANGE_FAILS(ConnectorIO0, Connector::ConnectorModes::CPM_MODE_STEP_AND_DIR);
    TEST_MODE_CHANGE_FAILS(ConnectorIO0, Connector::ConnectorModes::CPM_MODE_A_DIRECT_B_PWM);
    TEST_MODE_CHANGE_FAILS(ConnectorIO0, Connector::ConnectorModes::CPM_MODE_A_PWM_B_PWM);
    TEST_MODE_CHANGE_FAILS(ConnectorIO0, Connector::ConnectorModes::TTL);
    TEST_MODE_CHANGE_FAILS(ConnectorIO0, Connector::ConnectorModes::RS232);
    TEST_MODE_CHANGE_FAILS(ConnectorIO0, Connector::ConnectorModes::SPI);
    TEST_MODE_CHANGE_FAILS(ConnectorIO0, Connector::ConnectorModes::CCIO);
    TEST_MODE_CHANGE_FAILS(ConnectorIO0, Connector::ConnectorModes::USB_CDC);
}

TEST(IO0Test, StateInOutputDigital) {
    TEST_MODE_CHANGE(ConnectorIO0, Connector::ConnectorModes::OUTPUT_DIGITAL);
    CHECK_TRUE(ConnectorIO0.State(1));
    LONGS_EQUAL(1, ConnectorIO0.State());
    CHECK_TRUE(ConnectorIO0.State(0));
    LONGS_EQUAL(0, ConnectorIO0.State());
    CHECK_TRUE(ConnectorIO0.State(99));
    LONGS_EQUAL(1, ConnectorIO0.State());
}

TEST(IO0Test, StateInInputDigital) {
    TEST_MODE_CHANGE(ConnectorIO0, Connector::ConnectorModes::INPUT_DIGITAL);
    CHECK_FALSE(ConnectorIO0.State(1));
    CHECK_FALSE(ConnectorIO0.State(0));
}

TEST(IO0Test, StateInOutputPwm) {
    TEST_MODE_CHANGE(ConnectorIO0, Connector::ConnectorModes::OUTPUT_PWM);
    CHECK_TRUE(ConnectorIO0.State(1));
    LONGS_EQUAL(1, ConnectorIO0.State());
    CHECK_TRUE(ConnectorIO0.State(0));
    LONGS_EQUAL(0, ConnectorIO0.State());
    CHECK_TRUE(ConnectorIO0.State(99));
    LONGS_EQUAL(99, ConnectorIO0.State());
    // PWM values clip at 0xff
    CHECK_TRUE(ConnectorIO0.State(0x8765));
    LONGS_EQUAL(0xff, ConnectorIO0.State());
}


TEST(IO0Test, StateInOutputAnalog) {
    TEST_MODE_CHANGE(ConnectorIO0, Connector::ConnectorModes::OUTPUT_ANALOG);
    CHECK_TRUE(ConnectorIO0.State(2000));
    Delay_ms(200);
    //LONGS_EQUAL(1, ConnectorIO0.State());
    CHECK_TRUE(ConnectorIO0.State(0));
    Delay_ms(200);
    //LONGS_EQUAL(0, ConnectorIO0.State());
    CHECK_TRUE(ConnectorIO0.State(2000));
    Delay_ms(200);
    //LONGS_EQUAL(99, ConnectorIO0.State());
    // Analog values clip at 4095
    CHECK_TRUE(ConnectorIO0.State(4000));
    Delay_ms(200);
    //LONGS_EQUAL(4095, ConnectorIO0.State());
}

TEST(IO0Test, StateOutPulse) {
    TEST_MODE_CHANGE(ConnectorIO0, Connector::ConnectorModes::OUTPUT_DIGITAL);

    uint32_t onTime = 100;
    uint32_t offTime = 200;

    // Check that a pulse transitions at about the right time
    ConnectorIO0.OutputPulsesStart(onTime, offTime, 0);
    CHECK_TRUE(ConnectorIO0.State());
    Delay_ms(onTime + 1);
    CHECK_FALSE(ConnectorIO0.State());
    Delay_ms(offTime + 1);
    CHECK_TRUE(ConnectorIO0.State());
    Delay_ms(100);
    // Ensure that it ends properly
    ConnectorIO0.OutputPulsesStop();
    CHECK_FALSE(ConnectorIO0.State());
}

TEST(IO0Test, StateOutPulseBlockingSingle) {
    TEST_MODE_CHANGE(ConnectorIO0, Connector::ConnectorModes::OUTPUT_DIGITAL);

    uint32_t onTime = 100;
    uint32_t offTime = 200;

    uint32_t startTime, endTime;
    // Check that the blocking version of a single pulse returns at about
    // the right time
    startTime = Milliseconds();
    ConnectorIO0.OutputPulsesStart(onTime, offTime, 1, true);
    endTime = Milliseconds();
    CHECK_COMPARE((endTime - startTime), >=, onTime);
    CHECK_COMPARE((endTime - startTime), <=, onTime + 1);
}

TEST(IO0Test, StateOutPulseBlockingMulti) {
    TEST_MODE_CHANGE(ConnectorIO0, Connector::ConnectorModes::OUTPUT_DIGITAL);

    uint32_t onTime = 100;
    uint32_t offTime = 200;

    uint32_t startTime, endTime;
    // Check that the blocking version of a multi-pulse call returns at about
    // the right time
    uint8_t pulses = 5;
    startTime = Milliseconds();
    ConnectorIO0.OutputPulsesStart(onTime, offTime, pulses, true);
    endTime = Milliseconds();
    CHECK_COMPARE((endTime - startTime), >=, onTime * pulses + offTime * (pulses - 1));
    CHECK_COMPARE((endTime - startTime), <=, onTime * pulses + offTime * (pulses - 1) + 1);
}

TEST(IO0Test, StateOutPulseBlockingInfinite) {
    TEST_MODE_CHANGE(ConnectorIO0, Connector::ConnectorModes::OUTPUT_DIGITAL);

    uint32_t onTime = 100;
    uint32_t offTime = 200;

    uint32_t startTime, endTime;
    // Make sure that a blocking call with infinite pulses doesn't block
    startTime = Milliseconds();
    ConnectorIO0.OutputPulsesStart(onTime, offTime, 0, true);
    endTime = Milliseconds();
    CHECK_COMPARE((endTime - startTime), <=, 1);

    // Ensure that it ends properly
    ConnectorIO0.OutputPulsesStop();
    CHECK_FALSE(ConnectorIO0.State());
}

TEST(IO0Test, VerifyInput) {
    TEST_MODE_CHANGE(ConnectorIO0, Connector::ConnectorModes::OUTPUT_DIGITAL);
    TestIO::ManualRefresh(true);
    ConnectorIO0.State(0);
    while (TestIO::InputStateRT(ConnectorIO0)) {
        continue;
    }
    SysMgr.FastUpdate();
    while (TestIO::InputFilterTicksLeft(ConnectorIO0)) {
        SysMgr.FastUpdate();
    }
    LONGS_EQUAL(0, static_cast<DigitalIn>(ConnectorIO0).State());

    // Test the transition to State 1
    ConnectorIO0.State(1);
    while (!TestIO::InputStateRT(ConnectorIO0)) {
        continue;
    }
    LONGS_EQUAL(0, static_cast<DigitalIn>(ConnectorIO0).State());
    for (uint16_t i = 0; i < ConnectorIO0.FilterLength(); i++) {
        SysMgr.FastUpdate();
        LONGS_EQUAL(0, static_cast<DigitalIn>(ConnectorIO0).State());
    }
    SysMgr.FastUpdate();
    LONGS_EQUAL(1, static_cast<DigitalIn>(ConnectorIO0).State());

    // Test the transition to State 0
    ConnectorIO0.State(0);
    while (TestIO::InputStateRT(ConnectorIO0)) {
        continue;
    }
    LONGS_EQUAL(1, static_cast<DigitalIn>(ConnectorIO0).State());
    for (uint16_t i = 0; i < ConnectorIO0.FilterLength(); i++) {
        SysMgr.FastUpdate();
        LONGS_EQUAL(1, static_cast<DigitalIn>(ConnectorIO0).State());
    }
    SysMgr.FastUpdate();
    LONGS_EQUAL(0, static_cast<DigitalIn>(ConnectorIO0).State());
}