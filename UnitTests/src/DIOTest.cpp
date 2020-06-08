/*
 * DIOTest.cpp
 *
 * Created: 10/23/2019 5:46:53 PM
 *  Author: steve_dill
 */

#include "CppUTest/TestHarness.h"

#include "testHooks.h"


TEST_GROUP(DIOTest) {
    void setup() {
        TestIO::ManualRefresh(false);
        ConnectorIO1.Reinitialize();
        ConnectorIO2.Reinitialize();
        ConnectorIO3.Reinitialize();
    }

    void teardown() {
        ConnectorIO1.Reinitialize();
        ConnectorIO2.Reinitialize();
        ConnectorIO3.Reinitialize();
    }
};

TEST(DIOTest, InitialState) {
    LONGS_EQUAL(Connector::ConnectorModes::INPUT_DIGITAL, ConnectorIO1.Mode());
    LONGS_EQUAL(0, ConnectorIO1.State());
    LONGS_EQUAL(Connector::DIGITAL_IN_OUT_TYPE, ConnectorIO1.Type());
    CHECK_FALSE(ConnectorIO1.IsWritable());
    CHECK_FALSE(ConnectorIO1.IsInHwFault());
    LONGS_EQUAL(Connector::ConnectorModes::INPUT_DIGITAL, ConnectorIO2.Mode());
    LONGS_EQUAL(0, ConnectorIO2.State());
    LONGS_EQUAL(Connector::DIGITAL_IN_OUT_TYPE, ConnectorIO2.Type());
    CHECK_FALSE(ConnectorIO2.IsWritable());
    CHECK_FALSE(ConnectorIO2.IsInHwFault());
    LONGS_EQUAL(Connector::ConnectorModes::INPUT_DIGITAL, ConnectorIO3.Mode());
    LONGS_EQUAL(0, ConnectorIO3.State());
    LONGS_EQUAL(Connector::DIGITAL_IN_OUT_TYPE, ConnectorIO3.Type());
    CHECK_FALSE(ConnectorIO3.IsWritable());
    CHECK_FALSE(ConnectorIO3.IsInHwFault());
}

TEST(DIOTest, ModeCheckWithValidModes) {
    TEST_MODE_CHANGE(ConnectorIO1, Connector::ConnectorModes::OUTPUT_DIGITAL);
    CHECK_TRUE(ConnectorIO1.IsWritable());
    TEST_MODE_CHANGE(ConnectorIO1, Connector::ConnectorModes::INPUT_DIGITAL);
    CHECK_FALSE(ConnectorIO1.IsWritable());
    TEST_MODE_CHANGE(ConnectorIO1, Connector::ConnectorModes::OUTPUT_PWM);
    CHECK_TRUE(ConnectorIO1.IsWritable());

    TEST_MODE_CHANGE(ConnectorIO2, Connector::ConnectorModes::OUTPUT_DIGITAL);
    CHECK_TRUE(ConnectorIO2.IsWritable());
    TEST_MODE_CHANGE(ConnectorIO2, Connector::ConnectorModes::INPUT_DIGITAL);
    CHECK_FALSE(ConnectorIO2.IsWritable());
    TEST_MODE_CHANGE(ConnectorIO2, Connector::ConnectorModes::OUTPUT_PWM);
    CHECK_TRUE(ConnectorIO2.IsWritable());

    TEST_MODE_CHANGE(ConnectorIO3, Connector::ConnectorModes::OUTPUT_DIGITAL);
    CHECK_TRUE(ConnectorIO3.IsWritable());
    TEST_MODE_CHANGE(ConnectorIO3, Connector::ConnectorModes::INPUT_DIGITAL);
    CHECK_FALSE(ConnectorIO3.IsWritable());
    TEST_MODE_CHANGE(ConnectorIO3, Connector::ConnectorModes::OUTPUT_PWM);
    CHECK_TRUE(ConnectorIO3.IsWritable());
}

TEST(DIOTest, ModeCheckWithInvalidModes) {
    // Test that changing to invalid modes leaves the mode in INPUT_DIGITAL
    TEST_MODE_CHANGE_FAILS(ConnectorIO1, Connector::ConnectorModes::OUTPUT_ANALOG);
    TEST_MODE_CHANGE_FAILS(ConnectorIO1, Connector::ConnectorModes::INVALID_NONE);
    TEST_MODE_CHANGE_FAILS(ConnectorIO1, Connector::ConnectorModes::INPUT_ANALOG);
    TEST_MODE_CHANGE_FAILS(ConnectorIO1, Connector::ConnectorModes::OUTPUT_H_BRIDGE);
    TEST_MODE_CHANGE_FAILS(ConnectorIO1, Connector::ConnectorModes::OUTPUT_TONE);
    TEST_MODE_CHANGE_FAILS(ConnectorIO1, Connector::ConnectorModes::OUTPUT_WAVE);
    TEST_MODE_CHANGE_FAILS(ConnectorIO1, Connector::ConnectorModes::CPM_MODE_A_DIRECT_B_DIRECT);
    TEST_MODE_CHANGE_FAILS(ConnectorIO1, Connector::ConnectorModes::CPM_MODE_STEP_AND_DIR);
    TEST_MODE_CHANGE_FAILS(ConnectorIO1, Connector::ConnectorModes::CPM_MODE_A_DIRECT_B_PWM);
    TEST_MODE_CHANGE_FAILS(ConnectorIO1, Connector::ConnectorModes::CPM_MODE_A_PWM_B_PWM);
    TEST_MODE_CHANGE_FAILS(ConnectorIO1, Connector::ConnectorModes::TTL);
    TEST_MODE_CHANGE_FAILS(ConnectorIO1, Connector::ConnectorModes::RS232);
    TEST_MODE_CHANGE_FAILS(ConnectorIO1, Connector::ConnectorModes::SPI);
    TEST_MODE_CHANGE_FAILS(ConnectorIO1, Connector::ConnectorModes::CCIO);
    TEST_MODE_CHANGE_FAILS(ConnectorIO1, Connector::ConnectorModes::USB_CDC);
    LONGS_EQUAL(Connector::ConnectorModes::INPUT_DIGITAL, ConnectorIO1.Mode());

    TEST_MODE_CHANGE_FAILS(ConnectorIO2, Connector::ConnectorModes::OUTPUT_ANALOG);
    TEST_MODE_CHANGE_FAILS(ConnectorIO2, Connector::ConnectorModes::INVALID_NONE);
    TEST_MODE_CHANGE_FAILS(ConnectorIO2, Connector::ConnectorModes::INPUT_ANALOG);
    TEST_MODE_CHANGE_FAILS(ConnectorIO2, Connector::ConnectorModes::OUTPUT_H_BRIDGE);
    TEST_MODE_CHANGE_FAILS(ConnectorIO2, Connector::ConnectorModes::OUTPUT_TONE);
    TEST_MODE_CHANGE_FAILS(ConnectorIO2, Connector::ConnectorModes::OUTPUT_WAVE);
    TEST_MODE_CHANGE_FAILS(ConnectorIO2, Connector::ConnectorModes::CPM_MODE_A_DIRECT_B_DIRECT);
    TEST_MODE_CHANGE_FAILS(ConnectorIO2, Connector::ConnectorModes::CPM_MODE_STEP_AND_DIR);
    TEST_MODE_CHANGE_FAILS(ConnectorIO2, Connector::ConnectorModes::CPM_MODE_A_DIRECT_B_PWM);
    TEST_MODE_CHANGE_FAILS(ConnectorIO2, Connector::ConnectorModes::CPM_MODE_A_PWM_B_PWM);
    TEST_MODE_CHANGE_FAILS(ConnectorIO2, Connector::ConnectorModes::TTL);
    TEST_MODE_CHANGE_FAILS(ConnectorIO2, Connector::ConnectorModes::RS232);
    TEST_MODE_CHANGE_FAILS(ConnectorIO2, Connector::ConnectorModes::SPI);
    TEST_MODE_CHANGE_FAILS(ConnectorIO2, Connector::ConnectorModes::CCIO);
    TEST_MODE_CHANGE_FAILS(ConnectorIO2, Connector::ConnectorModes::USB_CDC);
    LONGS_EQUAL(Connector::ConnectorModes::INPUT_DIGITAL, ConnectorIO2.Mode());

    TEST_MODE_CHANGE_FAILS(ConnectorIO3, Connector::ConnectorModes::OUTPUT_ANALOG);
    TEST_MODE_CHANGE_FAILS(ConnectorIO3, Connector::ConnectorModes::INVALID_NONE);
    TEST_MODE_CHANGE_FAILS(ConnectorIO3, Connector::ConnectorModes::INPUT_ANALOG);
    TEST_MODE_CHANGE_FAILS(ConnectorIO3, Connector::ConnectorModes::OUTPUT_H_BRIDGE);
    TEST_MODE_CHANGE_FAILS(ConnectorIO3, Connector::ConnectorModes::OUTPUT_TONE);
    TEST_MODE_CHANGE_FAILS(ConnectorIO3, Connector::ConnectorModes::OUTPUT_WAVE);
    TEST_MODE_CHANGE_FAILS(ConnectorIO3, Connector::ConnectorModes::CPM_MODE_A_DIRECT_B_DIRECT);
    TEST_MODE_CHANGE_FAILS(ConnectorIO3, Connector::ConnectorModes::CPM_MODE_STEP_AND_DIR);
    TEST_MODE_CHANGE_FAILS(ConnectorIO3, Connector::ConnectorModes::CPM_MODE_A_DIRECT_B_PWM);
    TEST_MODE_CHANGE_FAILS(ConnectorIO3, Connector::ConnectorModes::CPM_MODE_A_PWM_B_PWM);
    TEST_MODE_CHANGE_FAILS(ConnectorIO3, Connector::ConnectorModes::TTL);
    TEST_MODE_CHANGE_FAILS(ConnectorIO3, Connector::ConnectorModes::RS232);
    TEST_MODE_CHANGE_FAILS(ConnectorIO3, Connector::ConnectorModes::SPI);
    TEST_MODE_CHANGE_FAILS(ConnectorIO3, Connector::ConnectorModes::CCIO);
    TEST_MODE_CHANGE_FAILS(ConnectorIO3, Connector::ConnectorModes::USB_CDC);
    LONGS_EQUAL(Connector::ConnectorModes::INPUT_DIGITAL, ConnectorIO3.Mode());
}

TEST(DIOTest, ModeCheckWithInvalidModesAsOutputMode) {
    // Change the mode to OUTPUT_DIGITAL
    CHECK_TRUE(ConnectorIO1.Mode(Connector::ConnectorModes::OUTPUT_DIGITAL));
    LONGS_EQUAL(Connector::ConnectorModes::OUTPUT_DIGITAL, ConnectorIO1.Mode());
    TEST_MODE_CHANGE_FAILS(ConnectorIO1, Connector::ConnectorModes::OUTPUT_ANALOG);
    TEST_MODE_CHANGE_FAILS(ConnectorIO1, Connector::ConnectorModes::INVALID_NONE);
    TEST_MODE_CHANGE_FAILS(ConnectorIO1, Connector::ConnectorModes::INPUT_ANALOG);
    TEST_MODE_CHANGE_FAILS(ConnectorIO1, Connector::ConnectorModes::OUTPUT_H_BRIDGE);
    TEST_MODE_CHANGE_FAILS(ConnectorIO1, Connector::ConnectorModes::OUTPUT_TONE);
    TEST_MODE_CHANGE_FAILS(ConnectorIO1, Connector::ConnectorModes::OUTPUT_WAVE);
    TEST_MODE_CHANGE_FAILS(ConnectorIO1, Connector::ConnectorModes::CPM_MODE_A_DIRECT_B_DIRECT);
    TEST_MODE_CHANGE_FAILS(ConnectorIO1, Connector::ConnectorModes::CPM_MODE_STEP_AND_DIR);
    TEST_MODE_CHANGE_FAILS(ConnectorIO1, Connector::ConnectorModes::CPM_MODE_A_DIRECT_B_PWM);
    TEST_MODE_CHANGE_FAILS(ConnectorIO1, Connector::ConnectorModes::CPM_MODE_A_PWM_B_PWM);
    TEST_MODE_CHANGE_FAILS(ConnectorIO1, Connector::ConnectorModes::TTL);
    TEST_MODE_CHANGE_FAILS(ConnectorIO1, Connector::ConnectorModes::RS232);
    TEST_MODE_CHANGE_FAILS(ConnectorIO1, Connector::ConnectorModes::SPI);
    TEST_MODE_CHANGE_FAILS(ConnectorIO1, Connector::ConnectorModes::CCIO);
    TEST_MODE_CHANGE_FAILS(ConnectorIO1, Connector::ConnectorModes::USB_CDC);
    LONGS_EQUAL(Connector::ConnectorModes::OUTPUT_DIGITAL, ConnectorIO1.Mode());

    CHECK_TRUE(ConnectorIO2.Mode(Connector::ConnectorModes::OUTPUT_DIGITAL));
    LONGS_EQUAL(Connector::ConnectorModes::OUTPUT_DIGITAL, ConnectorIO2.Mode());
    TEST_MODE_CHANGE_FAILS(ConnectorIO2, Connector::ConnectorModes::OUTPUT_ANALOG);
    TEST_MODE_CHANGE_FAILS(ConnectorIO2, Connector::ConnectorModes::INVALID_NONE);
    TEST_MODE_CHANGE_FAILS(ConnectorIO2, Connector::ConnectorModes::INPUT_ANALOG);
    TEST_MODE_CHANGE_FAILS(ConnectorIO2, Connector::ConnectorModes::OUTPUT_H_BRIDGE);
    TEST_MODE_CHANGE_FAILS(ConnectorIO2, Connector::ConnectorModes::OUTPUT_TONE);
    TEST_MODE_CHANGE_FAILS(ConnectorIO2, Connector::ConnectorModes::OUTPUT_WAVE);
    TEST_MODE_CHANGE_FAILS(ConnectorIO2, Connector::ConnectorModes::CPM_MODE_A_DIRECT_B_DIRECT);
    TEST_MODE_CHANGE_FAILS(ConnectorIO2, Connector::ConnectorModes::CPM_MODE_STEP_AND_DIR);
    TEST_MODE_CHANGE_FAILS(ConnectorIO2, Connector::ConnectorModes::CPM_MODE_A_DIRECT_B_PWM);
    TEST_MODE_CHANGE_FAILS(ConnectorIO2, Connector::ConnectorModes::CPM_MODE_A_PWM_B_PWM);
    TEST_MODE_CHANGE_FAILS(ConnectorIO2, Connector::ConnectorModes::TTL);
    TEST_MODE_CHANGE_FAILS(ConnectorIO2, Connector::ConnectorModes::RS232);
    TEST_MODE_CHANGE_FAILS(ConnectorIO2, Connector::ConnectorModes::SPI);
    TEST_MODE_CHANGE_FAILS(ConnectorIO2, Connector::ConnectorModes::CCIO);
    TEST_MODE_CHANGE_FAILS(ConnectorIO2, Connector::ConnectorModes::USB_CDC);
    LONGS_EQUAL(Connector::ConnectorModes::OUTPUT_DIGITAL, ConnectorIO2.Mode());

    CHECK_TRUE(ConnectorIO3.Mode(Connector::ConnectorModes::OUTPUT_DIGITAL));
    LONGS_EQUAL(Connector::ConnectorModes::OUTPUT_DIGITAL, ConnectorIO3.Mode());
    TEST_MODE_CHANGE_FAILS(ConnectorIO3, Connector::ConnectorModes::OUTPUT_ANALOG);
    TEST_MODE_CHANGE_FAILS(ConnectorIO3, Connector::ConnectorModes::INVALID_NONE);
    TEST_MODE_CHANGE_FAILS(ConnectorIO3, Connector::ConnectorModes::INPUT_ANALOG);
    TEST_MODE_CHANGE_FAILS(ConnectorIO3, Connector::ConnectorModes::OUTPUT_H_BRIDGE);
    TEST_MODE_CHANGE_FAILS(ConnectorIO3, Connector::ConnectorModes::OUTPUT_TONE);
    TEST_MODE_CHANGE_FAILS(ConnectorIO3, Connector::ConnectorModes::OUTPUT_WAVE);
    TEST_MODE_CHANGE_FAILS(ConnectorIO3, Connector::ConnectorModes::CPM_MODE_A_DIRECT_B_DIRECT);
    TEST_MODE_CHANGE_FAILS(ConnectorIO3, Connector::ConnectorModes::CPM_MODE_STEP_AND_DIR);
    TEST_MODE_CHANGE_FAILS(ConnectorIO3, Connector::ConnectorModes::CPM_MODE_A_DIRECT_B_PWM);
    TEST_MODE_CHANGE_FAILS(ConnectorIO3, Connector::ConnectorModes::CPM_MODE_A_PWM_B_PWM);
    TEST_MODE_CHANGE_FAILS(ConnectorIO3, Connector::ConnectorModes::TTL);
    TEST_MODE_CHANGE_FAILS(ConnectorIO3, Connector::ConnectorModes::RS232);
    TEST_MODE_CHANGE_FAILS(ConnectorIO3, Connector::ConnectorModes::SPI);
    TEST_MODE_CHANGE_FAILS(ConnectorIO3, Connector::ConnectorModes::CCIO);
    TEST_MODE_CHANGE_FAILS(ConnectorIO3, Connector::ConnectorModes::USB_CDC);
    LONGS_EQUAL(Connector::ConnectorModes::OUTPUT_DIGITAL, ConnectorIO3.Mode());
}

TEST(DIOTest, ModeCheckWithInvalidModesAsPwmMode) {
    // Change the mode to OUTPUT_DIGITAL
    CHECK_TRUE(ConnectorIO1.Mode(Connector::ConnectorModes::OUTPUT_PWM));
    LONGS_EQUAL(Connector::ConnectorModes::OUTPUT_PWM, ConnectorIO1.Mode());
    TEST_MODE_CHANGE_FAILS(ConnectorIO1, Connector::ConnectorModes::OUTPUT_ANALOG);
    TEST_MODE_CHANGE_FAILS(ConnectorIO1, Connector::ConnectorModes::INVALID_NONE);
    TEST_MODE_CHANGE_FAILS(ConnectorIO1, Connector::ConnectorModes::INPUT_ANALOG);
    TEST_MODE_CHANGE_FAILS(ConnectorIO1, Connector::ConnectorModes::OUTPUT_H_BRIDGE);
    TEST_MODE_CHANGE_FAILS(ConnectorIO1, Connector::ConnectorModes::OUTPUT_TONE);
    TEST_MODE_CHANGE_FAILS(ConnectorIO1, Connector::ConnectorModes::OUTPUT_WAVE);
    TEST_MODE_CHANGE_FAILS(ConnectorIO1, Connector::ConnectorModes::CPM_MODE_A_DIRECT_B_DIRECT);
    TEST_MODE_CHANGE_FAILS(ConnectorIO1, Connector::ConnectorModes::CPM_MODE_STEP_AND_DIR);
    TEST_MODE_CHANGE_FAILS(ConnectorIO1, Connector::ConnectorModes::CPM_MODE_A_DIRECT_B_PWM);
    TEST_MODE_CHANGE_FAILS(ConnectorIO1, Connector::ConnectorModes::CPM_MODE_A_PWM_B_PWM);
    TEST_MODE_CHANGE_FAILS(ConnectorIO1, Connector::ConnectorModes::TTL);
    TEST_MODE_CHANGE_FAILS(ConnectorIO1, Connector::ConnectorModes::RS232);
    TEST_MODE_CHANGE_FAILS(ConnectorIO1, Connector::ConnectorModes::SPI);
    TEST_MODE_CHANGE_FAILS(ConnectorIO1, Connector::ConnectorModes::CCIO);
    TEST_MODE_CHANGE_FAILS(ConnectorIO1, Connector::ConnectorModes::USB_CDC);
    LONGS_EQUAL(Connector::ConnectorModes::OUTPUT_PWM, ConnectorIO1.Mode());

    CHECK_TRUE(ConnectorIO2.Mode(Connector::ConnectorModes::OUTPUT_PWM));
    LONGS_EQUAL(Connector::ConnectorModes::OUTPUT_PWM, ConnectorIO2.Mode());
    TEST_MODE_CHANGE_FAILS(ConnectorIO2, Connector::ConnectorModes::OUTPUT_ANALOG);
    TEST_MODE_CHANGE_FAILS(ConnectorIO2, Connector::ConnectorModes::INVALID_NONE);
    TEST_MODE_CHANGE_FAILS(ConnectorIO2, Connector::ConnectorModes::INPUT_ANALOG);
    TEST_MODE_CHANGE_FAILS(ConnectorIO2, Connector::ConnectorModes::OUTPUT_H_BRIDGE);
    TEST_MODE_CHANGE_FAILS(ConnectorIO2, Connector::ConnectorModes::OUTPUT_TONE);
    TEST_MODE_CHANGE_FAILS(ConnectorIO2, Connector::ConnectorModes::OUTPUT_WAVE);
    TEST_MODE_CHANGE_FAILS(ConnectorIO2, Connector::ConnectorModes::CPM_MODE_A_DIRECT_B_DIRECT);
    TEST_MODE_CHANGE_FAILS(ConnectorIO2, Connector::ConnectorModes::CPM_MODE_STEP_AND_DIR);
    TEST_MODE_CHANGE_FAILS(ConnectorIO2, Connector::ConnectorModes::CPM_MODE_A_DIRECT_B_PWM);
    TEST_MODE_CHANGE_FAILS(ConnectorIO2, Connector::ConnectorModes::CPM_MODE_A_PWM_B_PWM);
    TEST_MODE_CHANGE_FAILS(ConnectorIO2, Connector::ConnectorModes::TTL);
    TEST_MODE_CHANGE_FAILS(ConnectorIO2, Connector::ConnectorModes::RS232);
    TEST_MODE_CHANGE_FAILS(ConnectorIO2, Connector::ConnectorModes::SPI);
    TEST_MODE_CHANGE_FAILS(ConnectorIO2, Connector::ConnectorModes::CCIO);
    TEST_MODE_CHANGE_FAILS(ConnectorIO2, Connector::ConnectorModes::USB_CDC);
    LONGS_EQUAL(Connector::ConnectorModes::OUTPUT_PWM, ConnectorIO2.Mode());

    CHECK_TRUE(ConnectorIO3.Mode(Connector::ConnectorModes::OUTPUT_PWM));
    LONGS_EQUAL(Connector::ConnectorModes::OUTPUT_PWM, ConnectorIO3.Mode());
    TEST_MODE_CHANGE_FAILS(ConnectorIO3, Connector::ConnectorModes::OUTPUT_ANALOG);
    TEST_MODE_CHANGE_FAILS(ConnectorIO3, Connector::ConnectorModes::INVALID_NONE);
    TEST_MODE_CHANGE_FAILS(ConnectorIO3, Connector::ConnectorModes::INPUT_ANALOG);
    TEST_MODE_CHANGE_FAILS(ConnectorIO3, Connector::ConnectorModes::OUTPUT_H_BRIDGE);
    TEST_MODE_CHANGE_FAILS(ConnectorIO3, Connector::ConnectorModes::OUTPUT_TONE);
    TEST_MODE_CHANGE_FAILS(ConnectorIO3, Connector::ConnectorModes::OUTPUT_WAVE);
    TEST_MODE_CHANGE_FAILS(ConnectorIO3, Connector::ConnectorModes::CPM_MODE_A_DIRECT_B_DIRECT);
    TEST_MODE_CHANGE_FAILS(ConnectorIO3, Connector::ConnectorModes::CPM_MODE_STEP_AND_DIR);
    TEST_MODE_CHANGE_FAILS(ConnectorIO3, Connector::ConnectorModes::CPM_MODE_A_DIRECT_B_PWM);
    TEST_MODE_CHANGE_FAILS(ConnectorIO3, Connector::ConnectorModes::CPM_MODE_A_PWM_B_PWM);
    TEST_MODE_CHANGE_FAILS(ConnectorIO3, Connector::ConnectorModes::TTL);
    TEST_MODE_CHANGE_FAILS(ConnectorIO3, Connector::ConnectorModes::RS232);
    TEST_MODE_CHANGE_FAILS(ConnectorIO3, Connector::ConnectorModes::SPI);
    TEST_MODE_CHANGE_FAILS(ConnectorIO3, Connector::ConnectorModes::CCIO);
    TEST_MODE_CHANGE_FAILS(ConnectorIO3, Connector::ConnectorModes::USB_CDC);
    LONGS_EQUAL(Connector::ConnectorModes::OUTPUT_PWM, ConnectorIO3.Mode());
}

TEST(DIOTest, StateInInputDigital) {
    // Test that trying to set the state fails
    CHECK_FALSE(ConnectorIO1.State(1));
    CHECK_FALSE(ConnectorIO1.State(0));
    CHECK_FALSE(ConnectorIO2.State(1));
    CHECK_FALSE(ConnectorIO2.State(0));
    CHECK_FALSE(ConnectorIO3.State(1));
    CHECK_FALSE(ConnectorIO3.State(0));
}

TEST(DIOTest, StateInOutputDigital) {
    TEST_MODE_CHANGE(ConnectorIO1, Connector::ConnectorModes::OUTPUT_DIGITAL);
    CHECK_TRUE(ConnectorIO1.State(1));
    LONGS_EQUAL(1, ConnectorIO1.State());
    CHECK_TRUE(ConnectorIO1.State(0));
    LONGS_EQUAL(0, ConnectorIO1.State());
    CHECK_TRUE(ConnectorIO1.State(99));
    LONGS_EQUAL(1, ConnectorIO1.State());

    TEST_MODE_CHANGE(ConnectorIO2, Connector::ConnectorModes::OUTPUT_DIGITAL);
    CHECK_TRUE(ConnectorIO2.State(1));
    LONGS_EQUAL(1, ConnectorIO2.State());
    CHECK_TRUE(ConnectorIO2.State(0));
    LONGS_EQUAL(0, ConnectorIO2.State());
    CHECK_TRUE(ConnectorIO2.State(99));
    LONGS_EQUAL(1, ConnectorIO2.State());

    TEST_MODE_CHANGE(ConnectorIO3, Connector::ConnectorModes::OUTPUT_DIGITAL);
    CHECK_TRUE(ConnectorIO3.State(1));
    LONGS_EQUAL(1, ConnectorIO3.State());
    CHECK_TRUE(ConnectorIO3.State(0));
    LONGS_EQUAL(0, ConnectorIO3.State());
    CHECK_TRUE(ConnectorIO3.State(99));
    LONGS_EQUAL(1, ConnectorIO3.State());
}

TEST(DIOTest, StateInOutputPwm) {
    TEST_MODE_CHANGE(ConnectorIO1, Connector::ConnectorModes::OUTPUT_PWM);
    CHECK_TRUE(ConnectorIO1.State(1));
    LONGS_EQUAL(1, ConnectorIO1.State());
    CHECK_TRUE(ConnectorIO1.State(0));
    LONGS_EQUAL(0, ConnectorIO1.State());
    CHECK_TRUE(ConnectorIO1.State(99));
    LONGS_EQUAL(99, ConnectorIO1.State());
    // PWM values clip at 0xff
    CHECK_TRUE(ConnectorIO1.State(0x8765));
    LONGS_EQUAL(0xff, ConnectorIO1.State());

    TEST_MODE_CHANGE(ConnectorIO2, Connector::ConnectorModes::OUTPUT_PWM);
    CHECK_TRUE(ConnectorIO2.State(1));
    LONGS_EQUAL(1, ConnectorIO2.State());
    CHECK_TRUE(ConnectorIO2.State(0));
    LONGS_EQUAL(0, ConnectorIO2.State());
    CHECK_TRUE(ConnectorIO2.State(99));
    LONGS_EQUAL(99, ConnectorIO2.State());
    // PWM values clip at 0xff
    CHECK_TRUE(ConnectorIO2.State(0x8765));
    LONGS_EQUAL(0xff, ConnectorIO2.State());

    TEST_MODE_CHANGE(ConnectorIO3, Connector::ConnectorModes::OUTPUT_PWM);
    CHECK_TRUE(ConnectorIO3.State(1));
    LONGS_EQUAL(1, ConnectorIO3.State());
    CHECK_TRUE(ConnectorIO3.State(0));
    LONGS_EQUAL(0, ConnectorIO3.State());
    CHECK_TRUE(ConnectorIO3.State(99));
    LONGS_EQUAL(99, ConnectorIO3.State());
    // PWM values clip at 0xff
    CHECK_TRUE(ConnectorIO3.State(0x8765));
    LONGS_EQUAL(0xff, ConnectorIO3.State());
}

TEST(DIOTest, StateOutPulse) {
    uint32_t onTime = 100;
    uint32_t offTime = 200;

    TEST_MODE_CHANGE(ConnectorIO1, Connector::ConnectorModes::OUTPUT_DIGITAL);
    TEST_MODE_CHANGE(ConnectorIO2, Connector::ConnectorModes::OUTPUT_DIGITAL);
    TEST_MODE_CHANGE(ConnectorIO3, Connector::ConnectorModes::OUTPUT_DIGITAL);

    // Check that a pulse transitions at about the right time
    ConnectorIO1.OutputPulsesStart(onTime, offTime, 0);
    CHECK_TRUE(ConnectorIO1.State());
    Delay_ms(onTime + 1);
    CHECK_FALSE(ConnectorIO1.State());
    Delay_ms(offTime + 1);
    CHECK_TRUE(ConnectorIO1.State());
    Delay_ms(100);
    // Ensure that it ends properly
    ConnectorIO1.OutputPulsesStop();
    CHECK_FALSE(ConnectorIO1.State());


    ConnectorIO2.OutputPulsesStart(onTime, offTime, 0);
    CHECK_TRUE(ConnectorIO2.State());
    Delay_ms(onTime + 1);
    CHECK_FALSE(ConnectorIO2.State());
    Delay_ms(offTime + 1);
    CHECK_TRUE(ConnectorIO2.State());
    Delay_ms(100);
    // Ensure that it ends properly
    ConnectorIO2.OutputPulsesStop();
    CHECK_FALSE(ConnectorIO2.State());


    ConnectorIO3.OutputPulsesStart(onTime, offTime, 0);
    CHECK_TRUE(ConnectorIO3.State());
    Delay_ms(onTime + 1);
    CHECK_FALSE(ConnectorIO3.State());
    Delay_ms(offTime + 1);
    CHECK_TRUE(ConnectorIO3.State());
    Delay_ms(100);
    // Ensure that it ends properly
    ConnectorIO3.OutputPulsesStop();
    CHECK_FALSE(ConnectorIO3.State());
}

TEST(DIOTest, StateOutPulseBlockingSingle) {
    uint32_t onTime = 100;
    uint32_t offTime = 200;
    uint32_t startTime, endTime;

    TEST_MODE_CHANGE(ConnectorIO1, Connector::ConnectorModes::OUTPUT_DIGITAL);
    TEST_MODE_CHANGE(ConnectorIO2, Connector::ConnectorModes::OUTPUT_DIGITAL);
    TEST_MODE_CHANGE(ConnectorIO3, Connector::ConnectorModes::OUTPUT_DIGITAL);

    // Check that the blocking version of a single pulse returns at about
    // the right time
    startTime = Milliseconds();
    ConnectorIO1.OutputPulsesStart(onTime, offTime, 1, true);
    endTime = Milliseconds();
    CHECK_COMPARE((endTime - startTime), >=, onTime);
    CHECK_COMPARE((endTime - startTime), <=, onTime + 1);

    // Check that the blocking version of a single pulse returns at about
    // the right time
    startTime = Milliseconds();
    ConnectorIO2.OutputPulsesStart(onTime, offTime, 1, true);
    endTime = Milliseconds();
    CHECK_COMPARE((endTime - startTime), >=, onTime);
    CHECK_COMPARE((endTime - startTime), <=, onTime + 1);

    // Check that the blocking version of a single pulse returns at about
    // the right time
    startTime = Milliseconds();
    ConnectorIO3.OutputPulsesStart(onTime, offTime, 1, true);
    endTime = Milliseconds();
    CHECK_COMPARE((endTime - startTime), >=, onTime);
    CHECK_COMPARE((endTime - startTime), <=, onTime + 1);
}

TEST(DIOTest, StateOutPulseBlockingMulti) {
    uint32_t onTime = 100;
    uint32_t offTime = 200;
    uint32_t startTime, endTime;
    uint8_t pulses = 5;

    TEST_MODE_CHANGE(ConnectorIO1, Connector::ConnectorModes::OUTPUT_DIGITAL);
    TEST_MODE_CHANGE(ConnectorIO2, Connector::ConnectorModes::OUTPUT_DIGITAL);
    TEST_MODE_CHANGE(ConnectorIO3, Connector::ConnectorModes::OUTPUT_DIGITAL);

    // Check that the blocking version of a multi-pulse call returns at about
    // the right time
    startTime = Milliseconds();
    ConnectorIO1.OutputPulsesStart(onTime, offTime, pulses, true);
    endTime = Milliseconds();
    CHECK_COMPARE((endTime - startTime), >=, onTime * pulses + offTime * (pulses - 1));
    CHECK_COMPARE((endTime - startTime), <=, onTime * pulses + offTime * (pulses - 1) + 1);

    startTime = Milliseconds();
    ConnectorIO2.OutputPulsesStart(onTime, offTime, pulses, true);
    endTime = Milliseconds();
    CHECK_COMPARE((endTime - startTime), >=, onTime * pulses + offTime * (pulses - 1));
    CHECK_COMPARE((endTime - startTime), <=, onTime * pulses + offTime * (pulses - 1) + 1);

    startTime = Milliseconds();
    ConnectorIO3.OutputPulsesStart(onTime, offTime, pulses, true);
    endTime = Milliseconds();
    CHECK_COMPARE((endTime - startTime), >=, onTime * pulses + offTime * (pulses - 1));
    CHECK_COMPARE((endTime - startTime), <=, onTime * pulses + offTime * (pulses - 1) + 1);
}

TEST(DIOTest, StateOutPulseBlockingInfinite) {
    uint32_t onTime = 100;
    uint32_t offTime = 200;
    uint32_t startTime, endTime;

    TEST_MODE_CHANGE(ConnectorIO1, Connector::ConnectorModes::OUTPUT_DIGITAL);
    TEST_MODE_CHANGE(ConnectorIO2, Connector::ConnectorModes::OUTPUT_DIGITAL);
    TEST_MODE_CHANGE(ConnectorIO3, Connector::ConnectorModes::OUTPUT_DIGITAL);

    // Make sure that a blocking call with infinite pulses doesn't block
    startTime = Milliseconds();
    ConnectorIO1.OutputPulsesStart(onTime, offTime, 0, true);
    endTime = Milliseconds();
    CHECK_COMPARE((endTime - startTime), <=, 1);
    // Ensure that it ends properly
    ConnectorIO1.OutputPulsesStop();
    CHECK_FALSE(ConnectorIO1.State());

    startTime = Milliseconds();
    ConnectorIO2.OutputPulsesStart(onTime, offTime, 0, true);
    endTime = Milliseconds();
    CHECK_COMPARE((endTime - startTime), <=, 1);
    // Ensure that it ends properly
    ConnectorIO2.OutputPulsesStop();
    CHECK_FALSE(ConnectorIO2.State());

    startTime = Milliseconds();
    ConnectorIO3.OutputPulsesStart(onTime, offTime, 0, true);
    endTime = Milliseconds();
    CHECK_COMPARE((endTime - startTime), <=, 1);
    // Ensure that it ends properly
    ConnectorIO3.OutputPulsesStop();
    CHECK_FALSE(ConnectorIO3.State());
}

TEST(DIOTest, VerifyInput) {
    // Set our filter lengths and modes
    TEST_MODE_CHANGE(ConnectorIO1, Connector::ConnectorModes::OUTPUT_DIGITAL);
    ConnectorIO1.FilterLength(50);
    LONGS_EQUAL(50, ConnectorIO1.FilterLength());
    TEST_MODE_CHANGE(ConnectorIO2, Connector::ConnectorModes::OUTPUT_DIGITAL);
    ConnectorIO2.FilterLength(50);
    LONGS_EQUAL(50, ConnectorIO2.FilterLength());
    TEST_MODE_CHANGE(ConnectorIO3, Connector::ConnectorModes::OUTPUT_DIGITAL);
    ConnectorIO3.FilterLength(50);
    LONGS_EQUAL(50, ConnectorIO3.FilterLength());
    TestIO::ManualRefresh(true);

    // IO1
    ConnectorIO1.State(0);
    while (TestIO::InputStateRT(ConnectorIO1)) {
        continue;
    }
    SysMgr.FastUpdate();
    while (TestIO::InputFilterTicksLeft(ConnectorIO1)) {
        SysMgr.FastUpdate();
    }
    LONGS_EQUAL(0, static_cast<DigitalIn>(ConnectorIO1).State());

    // Test the transition to State 1
    ConnectorIO1.State(1);
    while (!TestIO::InputStateRT(ConnectorIO1)) {
        continue;
    }
    LONGS_EQUAL(0, static_cast<DigitalIn>(ConnectorIO1).State());
    for (uint16_t i = 0; i < ConnectorIO1.FilterLength(); i++) {
        SysMgr.FastUpdate();
        LONGS_EQUAL(0, static_cast<DigitalIn>(ConnectorIO1).State());
    }
    SysMgr.FastUpdate();
    LONGS_EQUAL(1, static_cast<DigitalIn>(ConnectorIO1).State());

    // Test the transition to State 0
    ConnectorIO1.State(0);
    while (TestIO::InputStateRT(ConnectorIO1)) {
        continue;
    }
    LONGS_EQUAL(1, static_cast<DigitalIn>(ConnectorIO1).State());
    for (uint16_t i = 0; i < ConnectorIO1.FilterLength(); i++) {
        SysMgr.FastUpdate();
        LONGS_EQUAL(1, static_cast<DigitalIn>(ConnectorIO1).State());
    }
    SysMgr.FastUpdate();
    LONGS_EQUAL(0, static_cast<DigitalIn>(ConnectorIO1).State());

    // IO2
    ConnectorIO2.State(0);
    while (TestIO::InputStateRT(ConnectorIO2)) {
        continue;
    }
    SysMgr.FastUpdate();
    while (TestIO::InputFilterTicksLeft(ConnectorIO2)) {
        SysMgr.FastUpdate();
    }
    LONGS_EQUAL(0, static_cast<DigitalIn>(ConnectorIO2).State());

    // Test the transition to State 1
    ConnectorIO2.State(1);
    while (!TestIO::InputStateRT(ConnectorIO2)) {
        continue;
    }
    LONGS_EQUAL(0, static_cast<DigitalIn>(ConnectorIO2).State());
    for (uint16_t i = 0; i < ConnectorIO2.FilterLength(); i++) {
        SysMgr.FastUpdate();
        LONGS_EQUAL(0, static_cast<DigitalIn>(ConnectorIO2).State());
    }
    SysMgr.FastUpdate();
    LONGS_EQUAL(1, static_cast<DigitalIn>(ConnectorIO2).State());

    // Test the transition to State 0
    ConnectorIO2.State(0);
    while (TestIO::InputStateRT(ConnectorIO2)) {
        continue;
    }
    LONGS_EQUAL(1, static_cast<DigitalIn>(ConnectorIO2).State());
    for (uint16_t i = 0; i < ConnectorIO2.FilterLength(); i++) {
        SysMgr.FastUpdate();
        LONGS_EQUAL(1, static_cast<DigitalIn>(ConnectorIO2).State());
    }
    SysMgr.FastUpdate();
    LONGS_EQUAL(0, static_cast<DigitalIn>(ConnectorIO2).State());

    // IO3
    ConnectorIO3.State(0);
    while (TestIO::InputStateRT(ConnectorIO3)) {
        continue;
    }
    SysMgr.FastUpdate();
    while (TestIO::InputFilterTicksLeft(ConnectorIO3)) {
        SysMgr.FastUpdate();
    }
    LONGS_EQUAL(0, static_cast<DigitalIn>(ConnectorIO3).State());

    // Test the transition to State 1
    ConnectorIO3.State(1);
    while (!TestIO::InputStateRT(ConnectorIO3)) {
        continue;
    }
    LONGS_EQUAL(0, static_cast<DigitalIn>(ConnectorIO3).State());
    for (uint16_t i = 0; i < ConnectorIO3.FilterLength(); i++) {
        SysMgr.FastUpdate();
        LONGS_EQUAL(0, static_cast<DigitalIn>(ConnectorIO3).State());
    }
    SysMgr.FastUpdate();
    LONGS_EQUAL(1, static_cast<DigitalIn>(ConnectorIO3).State());

    // Test the transition to State 0
    ConnectorIO3.State(0);
    while (TestIO::InputStateRT(ConnectorIO3)) {
        continue;
    }
    LONGS_EQUAL(1, static_cast<DigitalIn>(ConnectorIO3).State());
    for (uint16_t i = 0; i < ConnectorIO3.FilterLength(); i++) {
        SysMgr.FastUpdate();
        LONGS_EQUAL(1, static_cast<DigitalIn>(ConnectorIO3).State());
    }
    SysMgr.FastUpdate();
    LONGS_EQUAL(0, static_cast<DigitalIn>(ConnectorIO3).State());
}