/*
 * HBTest.cpp
 *
 * Created: 10/25/2019 11:19:10 AM
 */

#include "CppUTest/TestHarness.h"

#include "testHooks.h"


TEST_GROUP(HBTest) {
    void setup() {
        TestIO::ManualRefresh(false);
        ConnectorIO4.Reinitialize();
        ConnectorIO5.Reinitialize();
    }

    void teardown() {
        ConnectorIO4.Reinitialize();
        ConnectorIO5.Reinitialize();
    }
};

TEST(HBTest, InitialState) {
    LONGS_EQUAL(Connector::ConnectorModes::INPUT_DIGITAL, ConnectorIO4.Mode());
    LONGS_EQUAL(0, ConnectorIO4.State());
    LONGS_EQUAL(Connector::H_BRIDGE_TYPE, ConnectorIO4.Type());
    CHECK_FALSE(ConnectorIO4.IsWritable());
    CHECK_FALSE(ConnectorIO4.IsInHwFault());
    LONGS_EQUAL(Connector::ConnectorModes::INPUT_DIGITAL, ConnectorIO5.Mode());
    LONGS_EQUAL(0, ConnectorIO5.State());
    LONGS_EQUAL(Connector::H_BRIDGE_TYPE, ConnectorIO5.Type());
    CHECK_FALSE(ConnectorIO5.IsWritable());
    CHECK_FALSE(ConnectorIO5.IsInHwFault());
}

TEST(HBTest, ModeCheckWithValidModes) {
    TEST_MODE_CHANGE(ConnectorIO4, Connector::ConnectorModes::OUTPUT_DIGITAL);
    CHECK_TRUE(ConnectorIO4.IsWritable());
    TEST_MODE_CHANGE(ConnectorIO4, Connector::ConnectorModes::INPUT_DIGITAL);
    CHECK_FALSE(ConnectorIO4.IsWritable());
    TEST_MODE_CHANGE(ConnectorIO4, Connector::ConnectorModes::OUTPUT_PWM);
    CHECK_TRUE(ConnectorIO4.IsWritable());
    TEST_MODE_CHANGE(ConnectorIO4, Connector::ConnectorModes::OUTPUT_H_BRIDGE);
    CHECK_TRUE(ConnectorIO4.IsWritable());
    TEST_MODE_CHANGE(ConnectorIO4, Connector::ConnectorModes::OUTPUT_TONE);
    CHECK_TRUE(ConnectorIO4.IsWritable());
    TEST_MODE_CHANGE(ConnectorIO4, Connector::ConnectorModes::OUTPUT_WAVE);
    CHECK_TRUE(ConnectorIO4.IsWritable());

    TEST_MODE_CHANGE(ConnectorIO5, Connector::ConnectorModes::OUTPUT_DIGITAL);
    CHECK_TRUE(ConnectorIO5.IsWritable());
    TEST_MODE_CHANGE(ConnectorIO5, Connector::ConnectorModes::INPUT_DIGITAL);
    CHECK_FALSE(ConnectorIO5.IsWritable());
    TEST_MODE_CHANGE(ConnectorIO5, Connector::ConnectorModes::OUTPUT_PWM);
    CHECK_TRUE(ConnectorIO5.IsWritable());
    TEST_MODE_CHANGE(ConnectorIO5, Connector::ConnectorModes::OUTPUT_H_BRIDGE);
    CHECK_TRUE(ConnectorIO5.IsWritable());
    TEST_MODE_CHANGE(ConnectorIO5, Connector::ConnectorModes::OUTPUT_TONE);
    CHECK_TRUE(ConnectorIO5.IsWritable());
    TEST_MODE_CHANGE(ConnectorIO5, Connector::ConnectorModes::OUTPUT_WAVE);
    CHECK_TRUE(ConnectorIO5.IsWritable());
}

TEST(HBTest, ModeCheckWithInvalidModes) {
    // Test that changing to invalid modes leaves the mode in INPUT_DIGITAL
    TEST_MODE_CHANGE_FAILS(ConnectorIO4, Connector::ConnectorModes::OUTPUT_ANALOG);
    TEST_MODE_CHANGE_FAILS(ConnectorIO4, Connector::ConnectorModes::INVALID_NONE);
    TEST_MODE_CHANGE_FAILS(ConnectorIO4, Connector::ConnectorModes::INPUT_ANALOG);
    TEST_MODE_CHANGE_FAILS(ConnectorIO4, Connector::ConnectorModes::CPM_MODE_A_DIRECT_B_DIRECT);
    TEST_MODE_CHANGE_FAILS(ConnectorIO4, Connector::ConnectorModes::CPM_MODE_STEP_AND_DIR);
    TEST_MODE_CHANGE_FAILS(ConnectorIO4, Connector::ConnectorModes::CPM_MODE_A_DIRECT_B_PWM);
    TEST_MODE_CHANGE_FAILS(ConnectorIO4, Connector::ConnectorModes::CPM_MODE_A_PWM_B_PWM);
    TEST_MODE_CHANGE_FAILS(ConnectorIO4, Connector::ConnectorModes::TTL);
    TEST_MODE_CHANGE_FAILS(ConnectorIO4, Connector::ConnectorModes::RS232);
    TEST_MODE_CHANGE_FAILS(ConnectorIO4, Connector::ConnectorModes::SPI);
    TEST_MODE_CHANGE_FAILS(ConnectorIO4, Connector::ConnectorModes::CCIO);
    TEST_MODE_CHANGE_FAILS(ConnectorIO4, Connector::ConnectorModes::USB_CDC);
    LONGS_EQUAL(Connector::ConnectorModes::INPUT_DIGITAL, ConnectorIO4.Mode());

    TEST_MODE_CHANGE_FAILS(ConnectorIO5, Connector::ConnectorModes::OUTPUT_ANALOG);
    TEST_MODE_CHANGE_FAILS(ConnectorIO5, Connector::ConnectorModes::INVALID_NONE);
    TEST_MODE_CHANGE_FAILS(ConnectorIO5, Connector::ConnectorModes::INPUT_ANALOG);
    TEST_MODE_CHANGE_FAILS(ConnectorIO5, Connector::ConnectorModes::CPM_MODE_A_DIRECT_B_DIRECT);
    TEST_MODE_CHANGE_FAILS(ConnectorIO5, Connector::ConnectorModes::CPM_MODE_STEP_AND_DIR);
    TEST_MODE_CHANGE_FAILS(ConnectorIO5, Connector::ConnectorModes::CPM_MODE_A_DIRECT_B_PWM);
    TEST_MODE_CHANGE_FAILS(ConnectorIO5, Connector::ConnectorModes::CPM_MODE_A_PWM_B_PWM);
    TEST_MODE_CHANGE_FAILS(ConnectorIO5, Connector::ConnectorModes::TTL);
    TEST_MODE_CHANGE_FAILS(ConnectorIO5, Connector::ConnectorModes::RS232);
    TEST_MODE_CHANGE_FAILS(ConnectorIO5, Connector::ConnectorModes::SPI);
    TEST_MODE_CHANGE_FAILS(ConnectorIO5, Connector::ConnectorModes::CCIO);
    TEST_MODE_CHANGE_FAILS(ConnectorIO5, Connector::ConnectorModes::USB_CDC);
    LONGS_EQUAL(Connector::ConnectorModes::INPUT_DIGITAL, ConnectorIO5.Mode());
}

TEST(HBTest, ModeCheckWithInvalidModesAsOutputMode) {
    // Change the mode to OUTPUT_DIGITAL
    CHECK_TRUE(ConnectorIO4.Mode(Connector::ConnectorModes::OUTPUT_DIGITAL));
    LONGS_EQUAL(Connector::ConnectorModes::OUTPUT_DIGITAL, ConnectorIO4.Mode());
    TEST_MODE_CHANGE_FAILS(ConnectorIO4, Connector::ConnectorModes::OUTPUT_ANALOG);
    TEST_MODE_CHANGE_FAILS(ConnectorIO4, Connector::ConnectorModes::INVALID_NONE);
    TEST_MODE_CHANGE_FAILS(ConnectorIO4, Connector::ConnectorModes::INPUT_ANALOG);
    TEST_MODE_CHANGE_FAILS(ConnectorIO4, Connector::ConnectorModes::CPM_MODE_A_DIRECT_B_DIRECT);
    TEST_MODE_CHANGE_FAILS(ConnectorIO4, Connector::ConnectorModes::CPM_MODE_STEP_AND_DIR);
    TEST_MODE_CHANGE_FAILS(ConnectorIO4, Connector::ConnectorModes::CPM_MODE_A_DIRECT_B_PWM);
    TEST_MODE_CHANGE_FAILS(ConnectorIO4, Connector::ConnectorModes::CPM_MODE_A_PWM_B_PWM);
    TEST_MODE_CHANGE_FAILS(ConnectorIO4, Connector::ConnectorModes::TTL);
    TEST_MODE_CHANGE_FAILS(ConnectorIO4, Connector::ConnectorModes::RS232);
    TEST_MODE_CHANGE_FAILS(ConnectorIO4, Connector::ConnectorModes::SPI);
    TEST_MODE_CHANGE_FAILS(ConnectorIO4, Connector::ConnectorModes::CCIO);
    TEST_MODE_CHANGE_FAILS(ConnectorIO4, Connector::ConnectorModes::USB_CDC);
    LONGS_EQUAL(Connector::ConnectorModes::OUTPUT_DIGITAL, ConnectorIO4.Mode());

    CHECK_TRUE(ConnectorIO5.Mode(Connector::ConnectorModes::OUTPUT_DIGITAL));
    LONGS_EQUAL(Connector::ConnectorModes::OUTPUT_DIGITAL, ConnectorIO5.Mode());
    TEST_MODE_CHANGE_FAILS(ConnectorIO5, Connector::ConnectorModes::OUTPUT_ANALOG);
    TEST_MODE_CHANGE_FAILS(ConnectorIO5, Connector::ConnectorModes::INVALID_NONE);
    TEST_MODE_CHANGE_FAILS(ConnectorIO5, Connector::ConnectorModes::INPUT_ANALOG);
    TEST_MODE_CHANGE_FAILS(ConnectorIO5, Connector::ConnectorModes::CPM_MODE_A_DIRECT_B_DIRECT);
    TEST_MODE_CHANGE_FAILS(ConnectorIO5, Connector::ConnectorModes::CPM_MODE_STEP_AND_DIR);
    TEST_MODE_CHANGE_FAILS(ConnectorIO5, Connector::ConnectorModes::CPM_MODE_A_DIRECT_B_PWM);
    TEST_MODE_CHANGE_FAILS(ConnectorIO5, Connector::ConnectorModes::CPM_MODE_A_PWM_B_PWM);
    TEST_MODE_CHANGE_FAILS(ConnectorIO5, Connector::ConnectorModes::TTL);
    TEST_MODE_CHANGE_FAILS(ConnectorIO5, Connector::ConnectorModes::RS232);
    TEST_MODE_CHANGE_FAILS(ConnectorIO5, Connector::ConnectorModes::SPI);
    TEST_MODE_CHANGE_FAILS(ConnectorIO5, Connector::ConnectorModes::CCIO);
    TEST_MODE_CHANGE_FAILS(ConnectorIO5, Connector::ConnectorModes::USB_CDC);
    LONGS_EQUAL(Connector::ConnectorModes::OUTPUT_DIGITAL, ConnectorIO5.Mode());
}

TEST(HBTest, ModeCheckWithInvalidModesAsPwmMode) {
    // Change the mode to OUTPUT_DIGITAL
    CHECK_TRUE(ConnectorIO4.Mode(Connector::ConnectorModes::OUTPUT_PWM));
    LONGS_EQUAL(Connector::ConnectorModes::OUTPUT_PWM, ConnectorIO4.Mode());
    TEST_MODE_CHANGE_FAILS(ConnectorIO4, Connector::ConnectorModes::OUTPUT_ANALOG);
    TEST_MODE_CHANGE_FAILS(ConnectorIO4, Connector::ConnectorModes::INVALID_NONE);
    TEST_MODE_CHANGE_FAILS(ConnectorIO4, Connector::ConnectorModes::INPUT_ANALOG);
    TEST_MODE_CHANGE_FAILS(ConnectorIO4, Connector::ConnectorModes::CPM_MODE_A_DIRECT_B_DIRECT);
    TEST_MODE_CHANGE_FAILS(ConnectorIO4, Connector::ConnectorModes::CPM_MODE_STEP_AND_DIR);
    TEST_MODE_CHANGE_FAILS(ConnectorIO4, Connector::ConnectorModes::CPM_MODE_A_DIRECT_B_PWM);
    TEST_MODE_CHANGE_FAILS(ConnectorIO4, Connector::ConnectorModes::CPM_MODE_A_PWM_B_PWM);
    TEST_MODE_CHANGE_FAILS(ConnectorIO4, Connector::ConnectorModes::TTL);
    TEST_MODE_CHANGE_FAILS(ConnectorIO4, Connector::ConnectorModes::RS232);
    TEST_MODE_CHANGE_FAILS(ConnectorIO4, Connector::ConnectorModes::SPI);
    TEST_MODE_CHANGE_FAILS(ConnectorIO4, Connector::ConnectorModes::CCIO);
    TEST_MODE_CHANGE_FAILS(ConnectorIO4, Connector::ConnectorModes::USB_CDC);
    LONGS_EQUAL(Connector::ConnectorModes::OUTPUT_PWM, ConnectorIO4.Mode());

    CHECK_TRUE(ConnectorIO5.Mode(Connector::ConnectorModes::OUTPUT_PWM));
    LONGS_EQUAL(Connector::ConnectorModes::OUTPUT_PWM, ConnectorIO5.Mode());
    TEST_MODE_CHANGE_FAILS(ConnectorIO5, Connector::ConnectorModes::OUTPUT_ANALOG);
    TEST_MODE_CHANGE_FAILS(ConnectorIO5, Connector::ConnectorModes::INVALID_NONE);
    TEST_MODE_CHANGE_FAILS(ConnectorIO5, Connector::ConnectorModes::INPUT_ANALOG);
    TEST_MODE_CHANGE_FAILS(ConnectorIO5, Connector::ConnectorModes::CPM_MODE_A_DIRECT_B_DIRECT);
    TEST_MODE_CHANGE_FAILS(ConnectorIO5, Connector::ConnectorModes::CPM_MODE_STEP_AND_DIR);
    TEST_MODE_CHANGE_FAILS(ConnectorIO5, Connector::ConnectorModes::CPM_MODE_A_DIRECT_B_PWM);
    TEST_MODE_CHANGE_FAILS(ConnectorIO5, Connector::ConnectorModes::CPM_MODE_A_PWM_B_PWM);
    TEST_MODE_CHANGE_FAILS(ConnectorIO5, Connector::ConnectorModes::TTL);
    TEST_MODE_CHANGE_FAILS(ConnectorIO5, Connector::ConnectorModes::RS232);
    TEST_MODE_CHANGE_FAILS(ConnectorIO5, Connector::ConnectorModes::SPI);
    TEST_MODE_CHANGE_FAILS(ConnectorIO5, Connector::ConnectorModes::CCIO);
    TEST_MODE_CHANGE_FAILS(ConnectorIO5, Connector::ConnectorModes::USB_CDC);
    LONGS_EQUAL(Connector::ConnectorModes::OUTPUT_PWM, ConnectorIO5.Mode());
}

TEST(HBTest, ModeCheckWithInvalidModesAsHbridge) {
    // Change the mode to OUTPUT_DIGITAL
    CHECK_TRUE(ConnectorIO4.Mode(Connector::ConnectorModes::OUTPUT_H_BRIDGE));
    LONGS_EQUAL(Connector::ConnectorModes::OUTPUT_H_BRIDGE, ConnectorIO4.Mode());
    TEST_MODE_CHANGE_FAILS(ConnectorIO4, Connector::ConnectorModes::OUTPUT_ANALOG);
    TEST_MODE_CHANGE_FAILS(ConnectorIO4, Connector::ConnectorModes::INVALID_NONE);
    TEST_MODE_CHANGE_FAILS(ConnectorIO4, Connector::ConnectorModes::INPUT_ANALOG);
    TEST_MODE_CHANGE_FAILS(ConnectorIO4, Connector::ConnectorModes::CPM_MODE_A_DIRECT_B_DIRECT);
    TEST_MODE_CHANGE_FAILS(ConnectorIO4, Connector::ConnectorModes::CPM_MODE_STEP_AND_DIR);
    TEST_MODE_CHANGE_FAILS(ConnectorIO4, Connector::ConnectorModes::CPM_MODE_A_DIRECT_B_PWM);
    TEST_MODE_CHANGE_FAILS(ConnectorIO4, Connector::ConnectorModes::CPM_MODE_A_PWM_B_PWM);
    TEST_MODE_CHANGE_FAILS(ConnectorIO4, Connector::ConnectorModes::TTL);
    TEST_MODE_CHANGE_FAILS(ConnectorIO4, Connector::ConnectorModes::RS232);
    TEST_MODE_CHANGE_FAILS(ConnectorIO4, Connector::ConnectorModes::SPI);
    TEST_MODE_CHANGE_FAILS(ConnectorIO4, Connector::ConnectorModes::CCIO);
    TEST_MODE_CHANGE_FAILS(ConnectorIO4, Connector::ConnectorModes::USB_CDC);
    LONGS_EQUAL(Connector::ConnectorModes::OUTPUT_H_BRIDGE, ConnectorIO4.Mode());

    CHECK_TRUE(ConnectorIO5.Mode(Connector::ConnectorModes::OUTPUT_H_BRIDGE));
    LONGS_EQUAL(Connector::ConnectorModes::OUTPUT_H_BRIDGE, ConnectorIO5.Mode());
    TEST_MODE_CHANGE_FAILS(ConnectorIO5, Connector::ConnectorModes::OUTPUT_ANALOG);
    TEST_MODE_CHANGE_FAILS(ConnectorIO5, Connector::ConnectorModes::INVALID_NONE);
    TEST_MODE_CHANGE_FAILS(ConnectorIO5, Connector::ConnectorModes::INPUT_ANALOG);
    TEST_MODE_CHANGE_FAILS(ConnectorIO5, Connector::ConnectorModes::CPM_MODE_A_DIRECT_B_DIRECT);
    TEST_MODE_CHANGE_FAILS(ConnectorIO5, Connector::ConnectorModes::CPM_MODE_STEP_AND_DIR);
    TEST_MODE_CHANGE_FAILS(ConnectorIO5, Connector::ConnectorModes::CPM_MODE_A_DIRECT_B_PWM);
    TEST_MODE_CHANGE_FAILS(ConnectorIO5, Connector::ConnectorModes::CPM_MODE_A_PWM_B_PWM);
    TEST_MODE_CHANGE_FAILS(ConnectorIO5, Connector::ConnectorModes::TTL);
    TEST_MODE_CHANGE_FAILS(ConnectorIO5, Connector::ConnectorModes::RS232);
    TEST_MODE_CHANGE_FAILS(ConnectorIO5, Connector::ConnectorModes::SPI);
    TEST_MODE_CHANGE_FAILS(ConnectorIO5, Connector::ConnectorModes::CCIO);
    TEST_MODE_CHANGE_FAILS(ConnectorIO5, Connector::ConnectorModes::USB_CDC);
    LONGS_EQUAL(Connector::ConnectorModes::OUTPUT_H_BRIDGE, ConnectorIO5.Mode());
}

TEST(HBTest, ModeCheckWithInvalidModesAsTone) {
    // Change the mode to OUTPUT_DIGITAL
    CHECK_TRUE(ConnectorIO4.Mode(Connector::ConnectorModes::OUTPUT_TONE));
    LONGS_EQUAL(Connector::ConnectorModes::OUTPUT_TONE, ConnectorIO4.Mode());
    TEST_MODE_CHANGE_FAILS(ConnectorIO4, Connector::ConnectorModes::OUTPUT_ANALOG);
    TEST_MODE_CHANGE_FAILS(ConnectorIO4, Connector::ConnectorModes::INVALID_NONE);
    TEST_MODE_CHANGE_FAILS(ConnectorIO4, Connector::ConnectorModes::INPUT_ANALOG);
    TEST_MODE_CHANGE_FAILS(ConnectorIO4, Connector::ConnectorModes::CPM_MODE_A_DIRECT_B_DIRECT);
    TEST_MODE_CHANGE_FAILS(ConnectorIO4, Connector::ConnectorModes::CPM_MODE_STEP_AND_DIR);
    TEST_MODE_CHANGE_FAILS(ConnectorIO4, Connector::ConnectorModes::CPM_MODE_A_DIRECT_B_PWM);
    TEST_MODE_CHANGE_FAILS(ConnectorIO4, Connector::ConnectorModes::CPM_MODE_A_PWM_B_PWM);
    TEST_MODE_CHANGE_FAILS(ConnectorIO4, Connector::ConnectorModes::TTL);
    TEST_MODE_CHANGE_FAILS(ConnectorIO4, Connector::ConnectorModes::RS232);
    TEST_MODE_CHANGE_FAILS(ConnectorIO4, Connector::ConnectorModes::SPI);
    TEST_MODE_CHANGE_FAILS(ConnectorIO4, Connector::ConnectorModes::CCIO);
    TEST_MODE_CHANGE_FAILS(ConnectorIO4, Connector::ConnectorModes::USB_CDC);
    LONGS_EQUAL(Connector::ConnectorModes::OUTPUT_TONE, ConnectorIO4.Mode());

    CHECK_TRUE(ConnectorIO5.Mode(Connector::ConnectorModes::OUTPUT_TONE));
    LONGS_EQUAL(Connector::ConnectorModes::OUTPUT_TONE, ConnectorIO5.Mode());
    TEST_MODE_CHANGE_FAILS(ConnectorIO5, Connector::ConnectorModes::OUTPUT_ANALOG);
    TEST_MODE_CHANGE_FAILS(ConnectorIO5, Connector::ConnectorModes::INVALID_NONE);
    TEST_MODE_CHANGE_FAILS(ConnectorIO5, Connector::ConnectorModes::INPUT_ANALOG);
    TEST_MODE_CHANGE_FAILS(ConnectorIO5, Connector::ConnectorModes::CPM_MODE_A_DIRECT_B_DIRECT);
    TEST_MODE_CHANGE_FAILS(ConnectorIO5, Connector::ConnectorModes::CPM_MODE_STEP_AND_DIR);
    TEST_MODE_CHANGE_FAILS(ConnectorIO5, Connector::ConnectorModes::CPM_MODE_A_DIRECT_B_PWM);
    TEST_MODE_CHANGE_FAILS(ConnectorIO5, Connector::ConnectorModes::CPM_MODE_A_PWM_B_PWM);
    TEST_MODE_CHANGE_FAILS(ConnectorIO5, Connector::ConnectorModes::TTL);
    TEST_MODE_CHANGE_FAILS(ConnectorIO5, Connector::ConnectorModes::RS232);
    TEST_MODE_CHANGE_FAILS(ConnectorIO5, Connector::ConnectorModes::SPI);
    TEST_MODE_CHANGE_FAILS(ConnectorIO5, Connector::ConnectorModes::CCIO);
    TEST_MODE_CHANGE_FAILS(ConnectorIO5, Connector::ConnectorModes::USB_CDC);
    LONGS_EQUAL(Connector::ConnectorModes::OUTPUT_TONE, ConnectorIO5.Mode());
}

TEST(HBTest, ModeCheckWithInvalidModesAsWave) {
    // Change the mode to OUTPUT_DIGITAL
    CHECK_TRUE(ConnectorIO4.Mode(Connector::ConnectorModes::OUTPUT_WAVE));
    LONGS_EQUAL(Connector::ConnectorModes::OUTPUT_WAVE, ConnectorIO4.Mode());
    TEST_MODE_CHANGE_FAILS(ConnectorIO4, Connector::ConnectorModes::OUTPUT_ANALOG);
    TEST_MODE_CHANGE_FAILS(ConnectorIO4, Connector::ConnectorModes::INVALID_NONE);
    TEST_MODE_CHANGE_FAILS(ConnectorIO4, Connector::ConnectorModes::INPUT_ANALOG);
    TEST_MODE_CHANGE_FAILS(ConnectorIO4, Connector::ConnectorModes::CPM_MODE_A_DIRECT_B_DIRECT);
    TEST_MODE_CHANGE_FAILS(ConnectorIO4, Connector::ConnectorModes::CPM_MODE_STEP_AND_DIR);
    TEST_MODE_CHANGE_FAILS(ConnectorIO4, Connector::ConnectorModes::CPM_MODE_A_DIRECT_B_PWM);
    TEST_MODE_CHANGE_FAILS(ConnectorIO4, Connector::ConnectorModes::CPM_MODE_A_PWM_B_PWM);
    TEST_MODE_CHANGE_FAILS(ConnectorIO4, Connector::ConnectorModes::TTL);
    TEST_MODE_CHANGE_FAILS(ConnectorIO4, Connector::ConnectorModes::RS232);
    TEST_MODE_CHANGE_FAILS(ConnectorIO4, Connector::ConnectorModes::SPI);
    TEST_MODE_CHANGE_FAILS(ConnectorIO4, Connector::ConnectorModes::CCIO);
    TEST_MODE_CHANGE_FAILS(ConnectorIO4, Connector::ConnectorModes::USB_CDC);
    LONGS_EQUAL(Connector::ConnectorModes::OUTPUT_WAVE, ConnectorIO4.Mode());

    CHECK_TRUE(ConnectorIO5.Mode(Connector::ConnectorModes::OUTPUT_WAVE));
    LONGS_EQUAL(Connector::ConnectorModes::OUTPUT_WAVE, ConnectorIO5.Mode());
    TEST_MODE_CHANGE_FAILS(ConnectorIO5, Connector::ConnectorModes::OUTPUT_ANALOG);
    TEST_MODE_CHANGE_FAILS(ConnectorIO5, Connector::ConnectorModes::INVALID_NONE);
    TEST_MODE_CHANGE_FAILS(ConnectorIO5, Connector::ConnectorModes::INPUT_ANALOG);
    TEST_MODE_CHANGE_FAILS(ConnectorIO5, Connector::ConnectorModes::CPM_MODE_A_DIRECT_B_DIRECT);
    TEST_MODE_CHANGE_FAILS(ConnectorIO5, Connector::ConnectorModes::CPM_MODE_STEP_AND_DIR);
    TEST_MODE_CHANGE_FAILS(ConnectorIO5, Connector::ConnectorModes::CPM_MODE_A_DIRECT_B_PWM);
    TEST_MODE_CHANGE_FAILS(ConnectorIO5, Connector::ConnectorModes::CPM_MODE_A_PWM_B_PWM);
    TEST_MODE_CHANGE_FAILS(ConnectorIO5, Connector::ConnectorModes::TTL);
    TEST_MODE_CHANGE_FAILS(ConnectorIO5, Connector::ConnectorModes::RS232);
    TEST_MODE_CHANGE_FAILS(ConnectorIO5, Connector::ConnectorModes::SPI);
    TEST_MODE_CHANGE_FAILS(ConnectorIO5, Connector::ConnectorModes::CCIO);
    TEST_MODE_CHANGE_FAILS(ConnectorIO5, Connector::ConnectorModes::USB_CDC);
    LONGS_EQUAL(Connector::ConnectorModes::OUTPUT_WAVE, ConnectorIO5.Mode());
}

TEST(HBTest, StateInInputDigital) {
    // Test that trying to set the state fails
    CHECK_FALSE(ConnectorIO4.State(1));
    CHECK_FALSE(ConnectorIO4.State(0));
    CHECK_FALSE(ConnectorIO5.State(1));
    CHECK_FALSE(ConnectorIO5.State(0));
}

TEST(HBTest, StateInOutputDigital) {
    TEST_MODE_CHANGE(ConnectorIO4, Connector::ConnectorModes::OUTPUT_DIGITAL);
    CHECK_TRUE(ConnectorIO4.State(1));
    LONGS_EQUAL(1, ConnectorIO4.State());
    CHECK_TRUE(ConnectorIO4.State(0));
    LONGS_EQUAL(0, ConnectorIO4.State());
    CHECK_TRUE(ConnectorIO4.State(99));
    LONGS_EQUAL(1, ConnectorIO4.State());

    TEST_MODE_CHANGE(ConnectorIO5, Connector::ConnectorModes::OUTPUT_DIGITAL);
    CHECK_TRUE(ConnectorIO5.State(1));
    LONGS_EQUAL(1, ConnectorIO5.State());
    CHECK_TRUE(ConnectorIO5.State(0));
    LONGS_EQUAL(0, ConnectorIO5.State());
    CHECK_TRUE(ConnectorIO5.State(99));
    LONGS_EQUAL(1, ConnectorIO5.State());
}

TEST(HBTest, StateInOutputPwm) {
    TEST_MODE_CHANGE(ConnectorIO4, Connector::ConnectorModes::OUTPUT_PWM);
    CHECK_TRUE(ConnectorIO4.State(1));
    LONGS_EQUAL(1, ConnectorIO4.State());
    CHECK_TRUE(ConnectorIO4.State(0));
    LONGS_EQUAL(0, ConnectorIO4.State());
    CHECK_TRUE(ConnectorIO4.State(99));
    LONGS_EQUAL(99, ConnectorIO4.State());
    // PWM values clip at 0xff
    CHECK_TRUE(ConnectorIO4.State(0x8765));
    LONGS_EQUAL(0xff, ConnectorIO4.State());

    TEST_MODE_CHANGE(ConnectorIO5, Connector::ConnectorModes::OUTPUT_PWM);
    CHECK_TRUE(ConnectorIO5.State(1));
    LONGS_EQUAL(1, ConnectorIO5.State());
    CHECK_TRUE(ConnectorIO5.State(0));
    LONGS_EQUAL(0, ConnectorIO5.State());
    CHECK_TRUE(ConnectorIO5.State(99));
    LONGS_EQUAL(99, ConnectorIO5.State());
    // PWM values clip at 0xff
    CHECK_TRUE(ConnectorIO5.State(0x8765));
    LONGS_EQUAL(0xff, ConnectorIO5.State());
}

TEST(HBTest, StateOutPulse) {
    uint32_t onTime = 100;
    uint32_t offTime = 200;

    TEST_MODE_CHANGE(ConnectorIO4, Connector::ConnectorModes::OUTPUT_DIGITAL);
    TEST_MODE_CHANGE(ConnectorIO5, Connector::ConnectorModes::OUTPUT_DIGITAL);

    // Check that a pulse transitions at about the right time
    ConnectorIO4.OutputPulsesStart(onTime, offTime, 0);
    CHECK_TRUE(ConnectorIO4.State());
    Delay_ms(onTime + 1);
    CHECK_FALSE(ConnectorIO4.State());
    Delay_ms(offTime + 1);
    CHECK_TRUE(ConnectorIO4.State());
    Delay_ms(100);
    // Ensure that it ends properly
    ConnectorIO4.OutputPulsesStop();
    CHECK_FALSE(ConnectorIO4.State());


    ConnectorIO5.OutputPulsesStart(onTime, offTime, 0);
    CHECK_TRUE(ConnectorIO5.State());
    Delay_ms(onTime + 1);
    CHECK_FALSE(ConnectorIO5.State());
    Delay_ms(offTime + 1);
    CHECK_TRUE(ConnectorIO5.State());
    Delay_ms(100);
    // Ensure that it ends properly
    ConnectorIO5.OutputPulsesStop();
    CHECK_FALSE(ConnectorIO5.State());
}

TEST(HBTest, StateOutPulseBlockingSingle) {
    uint32_t onTime = 100;
    uint32_t offTime = 200;
    uint32_t startTime, endTime;

    TEST_MODE_CHANGE(ConnectorIO4, Connector::ConnectorModes::OUTPUT_DIGITAL);
    TEST_MODE_CHANGE(ConnectorIO5, Connector::ConnectorModes::OUTPUT_DIGITAL);

    // Check that the blocking version of a single pulse returns at about
    // the right time
    startTime = Milliseconds();
    ConnectorIO4.OutputPulsesStart(onTime, offTime, 1, true);
    endTime = Milliseconds();
    CHECK_COMPARE((endTime - startTime), >=, onTime);
    CHECK_COMPARE((endTime - startTime), <=, onTime + 1);

    // Check that the blocking version of a single pulse returns at about
    // the right time
    startTime = Milliseconds();
    ConnectorIO5.OutputPulsesStart(onTime, offTime, 1, true);
    endTime = Milliseconds();
    CHECK_COMPARE((endTime - startTime), >=, onTime);
    CHECK_COMPARE((endTime - startTime), <=, onTime + 1);
}

TEST(HBTest, StateOutPulseBlockingMulti) {
    uint32_t onTime = 100;
    uint32_t offTime = 200;
    uint32_t startTime, endTime;
    uint8_t pulses = 5;

    TEST_MODE_CHANGE(ConnectorIO4, Connector::ConnectorModes::OUTPUT_DIGITAL);
    TEST_MODE_CHANGE(ConnectorIO5, Connector::ConnectorModes::OUTPUT_DIGITAL);

    // Check that the blocking version of a multi-pulse call returns at about
    // the right time
    startTime = Milliseconds();
    ConnectorIO4.OutputPulsesStart(onTime, offTime, pulses, true);
    endTime = Milliseconds();
    CHECK_COMPARE((endTime - startTime), >=, onTime * pulses + offTime * (pulses - 1));
    CHECK_COMPARE((endTime - startTime), <=, onTime * pulses + offTime * (pulses - 1) + 1);

    startTime = Milliseconds();
    ConnectorIO5.OutputPulsesStart(onTime, offTime, pulses, true);
    endTime = Milliseconds();
    CHECK_COMPARE((endTime - startTime), >=, onTime * pulses + offTime * (pulses - 1));
    CHECK_COMPARE((endTime - startTime), <=, onTime * pulses + offTime * (pulses - 1) + 1);
}

TEST(HBTest, StateOutPulseBlockingInfinite) {
    uint32_t onTime = 100;
    uint32_t offTime = 200;
    uint32_t startTime, endTime;

    TEST_MODE_CHANGE(ConnectorIO4, Connector::ConnectorModes::OUTPUT_DIGITAL);
    TEST_MODE_CHANGE(ConnectorIO5, Connector::ConnectorModes::OUTPUT_DIGITAL);

    // Make sure that a blocking call with infinite pulses doesn't block
    startTime = Milliseconds();
    ConnectorIO4.OutputPulsesStart(onTime, offTime, 0, true);
    endTime = Milliseconds();
    CHECK_COMPARE((endTime - startTime), <=, 1);
    // Ensure that it ends properly
    ConnectorIO4.OutputPulsesStop();
    CHECK_FALSE(ConnectorIO4.State());

    startTime = Milliseconds();
    ConnectorIO5.OutputPulsesStart(onTime, offTime, 0, true);
    endTime = Milliseconds();
    CHECK_COMPARE((endTime - startTime), <=, 1);
    // Ensure that it ends properly
    ConnectorIO5.OutputPulsesStop();
    CHECK_FALSE(ConnectorIO5.State());
}

TEST(HBTest, ToneStateTest) {
    uint32_t frequency = 100;
    uint32_t onTime = 100;
    uint32_t offTime = 200;

    TEST_MODE_CHANGE(ConnectorIO4, Connector::ConnectorModes::OUTPUT_TONE);
    TEST_MODE_CHANGE(ConnectorIO5, Connector::ConnectorModes::OUTPUT_TONE);

    LONGS_EQUAL(DigitalInOutHBridge::ToneState::TONE_OFF, ConnectorIO4.ToneActiveState());
    ConnectorIO4.ToneContinuous(frequency);
    LONGS_EQUAL(DigitalInOutHBridge::ToneState::TONE_CONTINUOUS, ConnectorIO4.ToneActiveState());
    ConnectorIO4.ToneStop();
    LONGS_EQUAL(DigitalInOutHBridge::ToneState::TONE_OFF, ConnectorIO4.ToneActiveState());
    ConnectorIO4.TonePeriodic(frequency, onTime, offTime);
    LONGS_EQUAL(DigitalInOutHBridge::ToneState::TONE_PERIODIC_ON, ConnectorIO4.ToneActiveState());
    Delay_ms(onTime + 5);
    LONGS_EQUAL(DigitalInOutHBridge::ToneState::TONE_PERIODIC_OFF, ConnectorIO4.ToneActiveState());
    ConnectorIO4.ToneStop();
    LONGS_EQUAL(DigitalInOutHBridge::ToneState::TONE_OFF, ConnectorIO4.ToneActiveState());
    ConnectorIO4.ToneTimed(frequency, onTime, true);
    LONGS_EQUAL(DigitalInOutHBridge::ToneState::TONE_OFF, ConnectorIO4.ToneActiveState());
    ConnectorIO4.ToneTimed(frequency, onTime);
    LONGS_EQUAL(DigitalInOutHBridge::ToneState::TONE_TIMED, ConnectorIO4.ToneActiveState());
    ConnectorIO4.ToneStop();
    LONGS_EQUAL(DigitalInOutHBridge::ToneState::TONE_OFF, ConnectorIO4.ToneActiveState());

    LONGS_EQUAL(DigitalInOutHBridge::ToneState::TONE_OFF, ConnectorIO5.ToneActiveState());
    ConnectorIO5.ToneContinuous(100);
    LONGS_EQUAL(DigitalInOutHBridge::ToneState::TONE_CONTINUOUS, ConnectorIO5.ToneActiveState());
    ConnectorIO5.ToneStop();
    LONGS_EQUAL(DigitalInOutHBridge::ToneState::TONE_OFF, ConnectorIO5.ToneActiveState());
    ConnectorIO5.TonePeriodic(frequency, onTime, offTime);
    LONGS_EQUAL(DigitalInOutHBridge::ToneState::TONE_PERIODIC_ON, ConnectorIO5.ToneActiveState());
    Delay_ms(onTime + 5);
    LONGS_EQUAL(DigitalInOutHBridge::ToneState::TONE_PERIODIC_OFF, ConnectorIO5.ToneActiveState());
    ConnectorIO5.ToneStop();
    LONGS_EQUAL(DigitalInOutHBridge::ToneState::TONE_OFF, ConnectorIO5.ToneActiveState());
    ConnectorIO5.ToneTimed(frequency, onTime, true);
    LONGS_EQUAL(DigitalInOutHBridge::ToneState::TONE_OFF, ConnectorIO5.ToneActiveState());
    ConnectorIO5.ToneTimed(frequency, onTime);
    LONGS_EQUAL(DigitalInOutHBridge::ToneState::TONE_TIMED, ConnectorIO5.ToneActiveState());
    ConnectorIO5.ToneStop();
    LONGS_EQUAL(DigitalInOutHBridge::ToneState::TONE_OFF, ConnectorIO5.ToneActiveState());
}

TEST(HBTest, ToneTiming) {
    uint32_t frequency = 100;
    uint32_t onTime = 100;
    uint32_t startTime, endTime;

    TEST_MODE_CHANGE(ConnectorIO4, Connector::ConnectorModes::OUTPUT_TONE);
    TEST_MODE_CHANGE(ConnectorIO5, Connector::ConnectorModes::OUTPUT_TONE);

    // Test the timing of blocked and unblocked timed tones
    LONGS_EQUAL(DigitalInOutHBridge::ToneState::TONE_OFF, ConnectorIO4.ToneActiveState());
    startTime = Milliseconds();
    ConnectorIO4.ToneTimed(frequency, onTime, true);
    endTime = Milliseconds();
    CHECK_COMPARE((endTime - startTime), >=, onTime);
    CHECK_COMPARE((endTime - startTime), <=, onTime + 1);
    LONGS_EQUAL(DigitalInOutHBridge::ToneState::TONE_OFF, ConnectorIO4.ToneActiveState());

    startTime = Milliseconds();
    ConnectorIO4.ToneTimed(frequency, onTime);
    LONGS_EQUAL(DigitalInOutHBridge::ToneState::TONE_TIMED, ConnectorIO4.ToneActiveState());
    while (Milliseconds() <= startTime + onTime + 1) {
        // Stay here until the tone should be done playing, then check the state
        continue;
    }
    LONGS_EQUAL(DigitalInOutHBridge::ToneState::TONE_OFF, ConnectorIO4.ToneActiveState());

    // Test the timing of blocked and unblocked timed tones
    LONGS_EQUAL(DigitalInOutHBridge::ToneState::TONE_OFF, ConnectorIO5.ToneActiveState());
    startTime = Milliseconds();
    ConnectorIO5.ToneTimed(frequency, onTime, true);
    endTime = Milliseconds();
    CHECK_COMPARE((endTime - startTime), >=, onTime);
    CHECK_COMPARE((endTime - startTime), <=, onTime + 1);
    LONGS_EQUAL(DigitalInOutHBridge::ToneState::TONE_OFF, ConnectorIO5.ToneActiveState());

    startTime = Milliseconds();
    ConnectorIO5.ToneTimed(frequency, onTime);
    LONGS_EQUAL(DigitalInOutHBridge::ToneState::TONE_TIMED, ConnectorIO5.ToneActiveState());
    while (Milliseconds() <= startTime + onTime + 1) {
        // Stay here until the tone should be done playing, then check the state
        continue;
    }
    LONGS_EQUAL(DigitalInOutHBridge::ToneState::TONE_OFF, ConnectorIO5.ToneActiveState());
}

TEST(HBTest, VerifyInput) {
    // Set our filter lengths and modes
    TEST_MODE_CHANGE(ConnectorIO4, Connector::ConnectorModes::OUTPUT_DIGITAL);
    ConnectorIO4.FilterLength(50);
    LONGS_EQUAL(50, ConnectorIO4.FilterLength());
    TEST_MODE_CHANGE(ConnectorIO5, Connector::ConnectorModes::OUTPUT_DIGITAL);
    ConnectorIO5.FilterLength(50);
    LONGS_EQUAL(50, ConnectorIO5.FilterLength());
    TestIO::ManualRefresh(true);

    // IO4
    ConnectorIO4.State(0);
    while (TestIO::InputStateRT(ConnectorIO4)) {
        continue;
    }
    SysMgr.FastUpdate();
    while (TestIO::InputFilterTicksLeft(ConnectorIO4)) {
        SysMgr.FastUpdate();
    }
    LONGS_EQUAL(0, static_cast<DigitalIn>(ConnectorIO4).State());

    // Test the transition to State 1
    ConnectorIO4.State(1);
    LONGS_EQUAL(0, static_cast<DigitalIn>(ConnectorIO4).State());
    for (uint16_t i = 0; i < ConnectorIO4.FilterLength(); i++) {
        SysMgr.FastUpdate();
        LONGS_EQUAL(0, static_cast<DigitalIn>(ConnectorIO4).State());
    }
    SysMgr.FastUpdate();
    LONGS_EQUAL(1, static_cast<DigitalIn>(ConnectorIO4).State());

    // Test the transition to State 0
    ConnectorIO4.State(0);
    LONGS_EQUAL(1, static_cast<DigitalIn>(ConnectorIO4).State());
    for (uint16_t i = 0; i < ConnectorIO4.FilterLength(); i++) {
        SysMgr.FastUpdate();
        LONGS_EQUAL(1, static_cast<DigitalIn>(ConnectorIO4).State());
    }
    SysMgr.FastUpdate();
    LONGS_EQUAL(0, static_cast<DigitalIn>(ConnectorIO4).State());

    // IO5
    ConnectorIO5.State(0);
    while (TestIO::InputStateRT(ConnectorIO5)) {
        continue;
    }
    SysMgr.FastUpdate();
    while (TestIO::InputFilterTicksLeft(ConnectorIO5)) {
        SysMgr.FastUpdate();
    }
    LONGS_EQUAL(0, static_cast<DigitalIn>(ConnectorIO5).State());

    // Test the transition to State 1
    ConnectorIO5.State(1);
    LONGS_EQUAL(0, static_cast<DigitalIn>(ConnectorIO5).State());
    for (uint16_t i = 0; i < ConnectorIO5.FilterLength(); i++) {
        SysMgr.FastUpdate();
        LONGS_EQUAL(0, static_cast<DigitalIn>(ConnectorIO5).State());
    }
    SysMgr.FastUpdate();
    LONGS_EQUAL(1, static_cast<DigitalIn>(ConnectorIO5).State());

    // Test the transition to State 0
    ConnectorIO5.State(0);
    LONGS_EQUAL(1, static_cast<DigitalIn>(ConnectorIO5).State());
    for (uint16_t i = 0; i < ConnectorIO5.FilterLength(); i++) {
        SysMgr.FastUpdate();
        LONGS_EQUAL(1, static_cast<DigitalIn>(ConnectorIO5).State());
    }
    SysMgr.FastUpdate();
    LONGS_EQUAL(0, static_cast<DigitalIn>(ConnectorIO5).State());
}