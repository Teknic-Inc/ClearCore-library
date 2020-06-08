#include "CppUTest/TestHarness.h"

#include "testHooks.h"


TEST_GROUP(DITest) {
    void setup() {
        TestIO::ManualRefresh(false);
        ConnectorDI6.Reinitialize();
        ConnectorDI7.Reinitialize();
        ConnectorDI8.Reinitialize();
    }

    void teardown() {
        TestIO::ManualRefresh(false);
        TestIO::UseFakeInputs(false);
        ConnectorDI6.Reinitialize();
        ConnectorDI7.Reinitialize();
        ConnectorDI8.Reinitialize();
    }
};

TEST(DITest, InitialState) {
    LONGS_EQUAL(Connector::ConnectorModes::INPUT_DIGITAL, ConnectorDI6.Mode());
    LONGS_EQUAL(0, ConnectorDI6.State());
    LONGS_EQUAL(Connector::DIGITAL_IN_TYPE, ConnectorDI6.Type());
    CHECK_FALSE(ConnectorDI6.IsWritable());
    CHECK_FALSE(ConnectorDI6.IsInHwFault());
    LONGS_EQUAL(Connector::ConnectorModes::INPUT_DIGITAL, ConnectorDI7.Mode());
    LONGS_EQUAL(0, ConnectorDI7.State());
    LONGS_EQUAL(Connector::DIGITAL_IN_TYPE, ConnectorDI7.Type());
    CHECK_FALSE(ConnectorDI7.IsWritable());
    CHECK_FALSE(ConnectorDI7.IsInHwFault());
    LONGS_EQUAL(Connector::ConnectorModes::INPUT_DIGITAL, ConnectorDI8.Mode());
    LONGS_EQUAL(0, ConnectorDI8.State());
    LONGS_EQUAL(Connector::DIGITAL_IN_TYPE, ConnectorDI8.Type());
    CHECK_FALSE(ConnectorDI8.IsWritable());
    CHECK_FALSE(ConnectorDI8.IsInHwFault());
}

TEST(DITest, ModeCheckWithInvalidModes) {
    // Test that changing to invalid modes leaves the mode in INPUT_DIGITAL
    TEST_MODE_CHANGE_FAILS(ConnectorDI6, Connector::ConnectorModes::OUTPUT_DIGITAL);
    TEST_MODE_CHANGE_FAILS(ConnectorDI6, Connector::ConnectorModes::OUTPUT_ANALOG);
    TEST_MODE_CHANGE_FAILS(ConnectorDI6, Connector::ConnectorModes::OUTPUT_PWM);
    TEST_MODE_CHANGE_FAILS(ConnectorDI6, Connector::ConnectorModes::INVALID_NONE);
    TEST_MODE_CHANGE_FAILS(ConnectorDI6, Connector::ConnectorModes::INPUT_ANALOG);
    TEST_MODE_CHANGE_FAILS(ConnectorDI6, Connector::ConnectorModes::OUTPUT_H_BRIDGE);
    TEST_MODE_CHANGE_FAILS(ConnectorDI6, Connector::ConnectorModes::OUTPUT_TONE);
    TEST_MODE_CHANGE_FAILS(ConnectorDI6, Connector::ConnectorModes::OUTPUT_WAVE);
    TEST_MODE_CHANGE_FAILS(ConnectorDI6, Connector::ConnectorModes::CPM_MODE_A_DIRECT_B_DIRECT);
    TEST_MODE_CHANGE_FAILS(ConnectorDI6, Connector::ConnectorModes::CPM_MODE_STEP_AND_DIR);
    TEST_MODE_CHANGE_FAILS(ConnectorDI6, Connector::ConnectorModes::CPM_MODE_A_DIRECT_B_PWM);
    TEST_MODE_CHANGE_FAILS(ConnectorDI6, Connector::ConnectorModes::CPM_MODE_A_PWM_B_PWM);
    TEST_MODE_CHANGE_FAILS(ConnectorDI6, Connector::ConnectorModes::TTL);
    TEST_MODE_CHANGE_FAILS(ConnectorDI6, Connector::ConnectorModes::RS232);
    TEST_MODE_CHANGE_FAILS(ConnectorDI6, Connector::ConnectorModes::SPI);
    TEST_MODE_CHANGE_FAILS(ConnectorDI6, Connector::ConnectorModes::CCIO);
    TEST_MODE_CHANGE_FAILS(ConnectorDI6, Connector::ConnectorModes::USB_CDC);
    LONGS_EQUAL(Connector::ConnectorModes::INPUT_DIGITAL, ConnectorDI6.Mode());

    TEST_MODE_CHANGE_FAILS(ConnectorDI7, Connector::ConnectorModes::OUTPUT_DIGITAL);
    TEST_MODE_CHANGE_FAILS(ConnectorDI7, Connector::ConnectorModes::OUTPUT_ANALOG);
    TEST_MODE_CHANGE_FAILS(ConnectorDI7, Connector::ConnectorModes::OUTPUT_PWM);
    TEST_MODE_CHANGE_FAILS(ConnectorDI7, Connector::ConnectorModes::INVALID_NONE);
    TEST_MODE_CHANGE_FAILS(ConnectorDI7, Connector::ConnectorModes::INPUT_ANALOG);
    TEST_MODE_CHANGE_FAILS(ConnectorDI7, Connector::ConnectorModes::OUTPUT_H_BRIDGE);
    TEST_MODE_CHANGE_FAILS(ConnectorDI7, Connector::ConnectorModes::OUTPUT_TONE);
    TEST_MODE_CHANGE_FAILS(ConnectorDI7, Connector::ConnectorModes::OUTPUT_WAVE);
    TEST_MODE_CHANGE_FAILS(ConnectorDI7, Connector::ConnectorModes::CPM_MODE_A_DIRECT_B_DIRECT);
    TEST_MODE_CHANGE_FAILS(ConnectorDI7, Connector::ConnectorModes::CPM_MODE_STEP_AND_DIR);
    TEST_MODE_CHANGE_FAILS(ConnectorDI7, Connector::ConnectorModes::CPM_MODE_A_DIRECT_B_PWM);
    TEST_MODE_CHANGE_FAILS(ConnectorDI7, Connector::ConnectorModes::CPM_MODE_A_PWM_B_PWM);
    TEST_MODE_CHANGE_FAILS(ConnectorDI7, Connector::ConnectorModes::TTL);
    TEST_MODE_CHANGE_FAILS(ConnectorDI7, Connector::ConnectorModes::RS232);
    TEST_MODE_CHANGE_FAILS(ConnectorDI7, Connector::ConnectorModes::SPI);
    TEST_MODE_CHANGE_FAILS(ConnectorDI7, Connector::ConnectorModes::CCIO);
    TEST_MODE_CHANGE_FAILS(ConnectorDI7, Connector::ConnectorModes::USB_CDC);
    LONGS_EQUAL(Connector::ConnectorModes::INPUT_DIGITAL, ConnectorDI7.Mode());

    TEST_MODE_CHANGE_FAILS(ConnectorDI8, Connector::ConnectorModes::OUTPUT_DIGITAL);
    TEST_MODE_CHANGE_FAILS(ConnectorDI8, Connector::ConnectorModes::OUTPUT_ANALOG);
    TEST_MODE_CHANGE_FAILS(ConnectorDI8, Connector::ConnectorModes::OUTPUT_PWM);
    TEST_MODE_CHANGE_FAILS(ConnectorDI8, Connector::ConnectorModes::INVALID_NONE);
    TEST_MODE_CHANGE_FAILS(ConnectorDI8, Connector::ConnectorModes::INPUT_ANALOG);
    TEST_MODE_CHANGE_FAILS(ConnectorDI8, Connector::ConnectorModes::OUTPUT_H_BRIDGE);
    TEST_MODE_CHANGE_FAILS(ConnectorDI8, Connector::ConnectorModes::OUTPUT_TONE);
    TEST_MODE_CHANGE_FAILS(ConnectorDI8, Connector::ConnectorModes::OUTPUT_WAVE);
    TEST_MODE_CHANGE_FAILS(ConnectorDI8, Connector::ConnectorModes::CPM_MODE_A_DIRECT_B_DIRECT);
    TEST_MODE_CHANGE_FAILS(ConnectorDI8, Connector::ConnectorModes::CPM_MODE_STEP_AND_DIR);
    TEST_MODE_CHANGE_FAILS(ConnectorDI8, Connector::ConnectorModes::CPM_MODE_A_DIRECT_B_PWM);
    TEST_MODE_CHANGE_FAILS(ConnectorDI8, Connector::ConnectorModes::CPM_MODE_A_PWM_B_PWM);
    TEST_MODE_CHANGE_FAILS(ConnectorDI8, Connector::ConnectorModes::TTL);
    TEST_MODE_CHANGE_FAILS(ConnectorDI8, Connector::ConnectorModes::RS232);
    TEST_MODE_CHANGE_FAILS(ConnectorDI8, Connector::ConnectorModes::SPI);
    TEST_MODE_CHANGE_FAILS(ConnectorDI8, Connector::ConnectorModes::CCIO);
    TEST_MODE_CHANGE_FAILS(ConnectorDI8, Connector::ConnectorModes::USB_CDC);
    LONGS_EQUAL(Connector::ConnectorModes::INPUT_DIGITAL, ConnectorDI8.Mode());
}

TEST(DITest, StateInInputDigital) {
    // Test that trying to set the state fails
    CHECK_FALSE(ConnectorDI6.State(1));
    CHECK_FALSE(ConnectorDI6.State(0));
    CHECK_FALSE(ConnectorDI7.State(1));
    CHECK_FALSE(ConnectorDI7.State(0));
    CHECK_FALSE(ConnectorDI8.State(1));
    CHECK_FALSE(ConnectorDI8.State(0));
}



static void testInputFilter(DigitalIn &input, uint16_t len, bool initState) {
    TestIO::ManualRefresh(true);
    TestIO::InitFakeInput(input, initState, len);
    TEST_VAL_REFRESH(initState, input.State(), 1);
    TestIO::FakeInput(input, !initState);
    TEST_VAL_REFRESH(initState, input.State(), len);
    TEST_VAL_REFRESH(!initState, input.State(), 2);
    TestIO::FakeInput(input, initState);
    TEST_VAL_REFRESH(!initState, input.State(), len);
    TEST_VAL_REFRESH(initState, input.State(), 2);
}

TEST(DITest, FilteringTestDI6L3T) {
    testInputFilter(ConnectorDI6, 3, true);
}

TEST(DITest, FilteringTestDI6L3F) {
    testInputFilter(ConnectorDI6, 3, false);
}

TEST(DITest, FilteringTestDI6L1T) {
    testInputFilter(ConnectorDI6, 1, true);
}

TEST(DITest, FilteringTestDI6L1F) {
    testInputFilter(ConnectorDI6, 1, false);
}

TEST(DITest, FilteringTestDI6L0T) {
    testInputFilter(ConnectorDI6, 0, true);
}

TEST(DITest, FilteringTestDI6L0F) {
    testInputFilter(ConnectorDI6, 0, false);
}