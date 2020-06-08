#include "CppUTest/TestHarness.h"

#include "ClearCore.h"


/* Test Setup:
    This test assumes you have at least one CCIO module connected to
    COM-1
*/

SerialDriver &ComPort = ConnectorCOM1;
uint8_t pinMax;
ClearCorePins pinMaxIndex;
uint8_t ccioCount;


#define TEST_MODE_CHANGE(theConnector, newMode) \
    CHECK_TRUE(theConnector->Mode(newMode)); \
    LONGS_EQUAL(newMode, theConnector->Mode());

#define TEST_MODE_CHANGE_FAILS(theConnector, newMode) { \
    Connector::ConnectorModes oldMode = theConnector->Mode(); \
    CHECK_FALSE(theConnector->Mode(newMode)); \
    LONGS_EQUAL(oldMode, theConnector->Mode()); }

TEST_GROUP(CcioTest) {
    void setup() {
        // Open the COM port in CCIO mode
        ComPort.Mode(Connector::ConnectorModes::CCIO);
        ComPort.PortOpen();

        ccioCount = CcioMgr.CcioCount();
        pinMax = CLEARCORE_PIN_CCIOA0 + CCIO_PINS_PER_BOARD * ccioCount;
        pinMaxIndex = (ClearCorePins)(pinMax - 1);
    }

    void teardown() {
        ComPort.Reinitialize();
    }
};

TEST(CcioTest, InitialState) {
    CHECK_FALSE(CcioMgr.LinkBroken());
    CHECK_COMPARE(CcioMgr.CcioCount(), >, 0);
    LONGS_EQUAL(Connector::ConnectorModes::INPUT_DIGITAL, CcioMgr.PinByIndex(CLEARCORE_PIN_CCIOA0)->Mode());
    LONGS_EQUAL(0, CcioMgr.PinByIndex(CLEARCORE_PIN_CCIOA0)->State());
    LONGS_EQUAL(Connector::CCIO_DIGITAL_IN_OUT_TYPE, CcioMgr.PinByIndex(CLEARCORE_PIN_CCIOA0)->Type());
    LONGS_EQUAL(Connector::ConnectorModes::INPUT_DIGITAL, CcioMgr.PinByIndex(pinMaxIndex)->Mode());
    LONGS_EQUAL(0, CcioMgr.PinByIndex(pinMaxIndex)->State());
    LONGS_EQUAL(Connector::CCIO_DIGITAL_IN_OUT_TYPE, CcioMgr.PinByIndex(pinMaxIndex)->Type());
}

TEST(CcioTest, ModeCheckWithValidModes) {
    TEST_MODE_CHANGE(CcioMgr.PinByIndex(CLEARCORE_PIN_CCIOA0), Connector::ConnectorModes::OUTPUT_DIGITAL);
    CHECK_TRUE(CcioMgr.PinByIndex(CLEARCORE_PIN_CCIOA0)->IsWritable());
    TEST_MODE_CHANGE(CcioMgr.PinByIndex(CLEARCORE_PIN_CCIOA0), Connector::ConnectorModes::INPUT_DIGITAL);
    CHECK_FALSE(CcioMgr.PinByIndex(CLEARCORE_PIN_CCIOA0)->IsWritable());
}

TEST(CcioTest, ModeCheckWithInvalidModes) {
    TEST_MODE_CHANGE(CcioMgr.PinByIndex(CLEARCORE_PIN_CCIOA0), Connector::ConnectorModes::INPUT_DIGITAL);
    // Test that changing to invalid modes leaves the mode in INPUT_DIGITAL
    TEST_MODE_CHANGE_FAILS(CcioMgr.PinByIndex(CLEARCORE_PIN_CCIOA0), Connector::ConnectorModes::OUTPUT_ANALOG);
    TEST_MODE_CHANGE_FAILS(CcioMgr.PinByIndex(CLEARCORE_PIN_CCIOA0), Connector::ConnectorModes::OUTPUT_PWM);
    TEST_MODE_CHANGE_FAILS(CcioMgr.PinByIndex(CLEARCORE_PIN_CCIOA0), Connector::ConnectorModes::INVALID_NONE);
    TEST_MODE_CHANGE_FAILS(CcioMgr.PinByIndex(CLEARCORE_PIN_CCIOA0), Connector::ConnectorModes::INPUT_ANALOG);
    TEST_MODE_CHANGE_FAILS(CcioMgr.PinByIndex(CLEARCORE_PIN_CCIOA0), Connector::ConnectorModes::OUTPUT_H_BRIDGE);
    TEST_MODE_CHANGE_FAILS(CcioMgr.PinByIndex(CLEARCORE_PIN_CCIOA0), Connector::ConnectorModes::OUTPUT_TONE);
    TEST_MODE_CHANGE_FAILS(CcioMgr.PinByIndex(CLEARCORE_PIN_CCIOA0), Connector::ConnectorModes::OUTPUT_WAVE);
    TEST_MODE_CHANGE_FAILS(CcioMgr.PinByIndex(CLEARCORE_PIN_CCIOA0), Connector::ConnectorModes::CPM_MODE_A_DIRECT_B_DIRECT);
    TEST_MODE_CHANGE_FAILS(CcioMgr.PinByIndex(CLEARCORE_PIN_CCIOA0), Connector::ConnectorModes::CPM_MODE_STEP_AND_DIR);
    TEST_MODE_CHANGE_FAILS(CcioMgr.PinByIndex(CLEARCORE_PIN_CCIOA0), Connector::ConnectorModes::CPM_MODE_A_DIRECT_B_PWM);
    TEST_MODE_CHANGE_FAILS(CcioMgr.PinByIndex(CLEARCORE_PIN_CCIOA0), Connector::ConnectorModes::CPM_MODE_A_PWM_B_PWM);
    TEST_MODE_CHANGE_FAILS(CcioMgr.PinByIndex(CLEARCORE_PIN_CCIOA0), Connector::ConnectorModes::TTL);
    TEST_MODE_CHANGE_FAILS(CcioMgr.PinByIndex(CLEARCORE_PIN_CCIOA0), Connector::ConnectorModes::RS232);
    TEST_MODE_CHANGE_FAILS(CcioMgr.PinByIndex(CLEARCORE_PIN_CCIOA0), Connector::ConnectorModes::SPI);
    TEST_MODE_CHANGE_FAILS(CcioMgr.PinByIndex(CLEARCORE_PIN_CCIOA0), Connector::ConnectorModes::CCIO);
    TEST_MODE_CHANGE_FAILS(CcioMgr.PinByIndex(CLEARCORE_PIN_CCIOA0), Connector::ConnectorModes::USB_CDC);
    LONGS_EQUAL(Connector::ConnectorModes::INPUT_DIGITAL, CcioMgr.PinByIndex(CLEARCORE_PIN_CCIOA0)->Mode());
}

TEST(CcioTest, StateInOutputDigital) {
    TEST_MODE_CHANGE(CcioMgr.PinByIndex(CLEARCORE_PIN_CCIOA0), Connector::ConnectorModes::OUTPUT_DIGITAL);
    CHECK_TRUE(CcioMgr.PinByIndex(CLEARCORE_PIN_CCIOA0)->State(1));
    LONGS_EQUAL(1, CcioMgr.PinByIndex(CLEARCORE_PIN_CCIOA0)->State());
    CHECK_TRUE(CcioMgr.PinByIndex(CLEARCORE_PIN_CCIOA0)->State(0));
    LONGS_EQUAL(0, CcioMgr.PinByIndex(CLEARCORE_PIN_CCIOA0)->State());
}

TEST(CcioTest, StateOutPulse) {
    TEST_MODE_CHANGE(CcioMgr.PinByIndex(CLEARCORE_PIN_CCIOA0), Connector::ConnectorModes::OUTPUT_DIGITAL);

    uint32_t onTime = 100;
    uint32_t offTime = 200;

    // Check that a pulse transitions at about the right time
    CcioMgr.PinByIndex(CLEARCORE_PIN_CCIOA0)->OutputPulsesStart(onTime, offTime, 0);
    CHECK_TRUE(CcioMgr.PinByIndex(CLEARCORE_PIN_CCIOA0)->State());
    Delay_ms(onTime + 1);
    CHECK_FALSE(CcioMgr.PinByIndex(CLEARCORE_PIN_CCIOA0)->State());
    Delay_ms(offTime + 1);
    CHECK_TRUE(CcioMgr.PinByIndex(CLEARCORE_PIN_CCIOA0)->State());
    Delay_ms(100);
    // Ensure that it ends properly
    CcioMgr.PinByIndex(CLEARCORE_PIN_CCIOA0)->OutputPulsesStop();
    CHECK_FALSE(CcioMgr.PinByIndex(CLEARCORE_PIN_CCIOA0)->State());
}

TEST(CcioTest, StateOutPulseBlockingSingle) {
    TEST_MODE_CHANGE(CcioMgr.PinByIndex(CLEARCORE_PIN_CCIOA0), Connector::ConnectorModes::OUTPUT_DIGITAL);

    uint32_t onTime = 100;
    uint32_t offTime = 200;
    uint32_t startTime, endTime;
    // Check that the blocking version of a single pulse returns at about
    // the right time
    startTime = Milliseconds();
    CcioMgr.PinByIndex(CLEARCORE_PIN_CCIOA0)->OutputPulsesStart(onTime, offTime, 1, true);
    endTime = Milliseconds();
    CHECK_COMPARE((endTime - startTime), >=, onTime);
    CHECK_COMPARE((endTime - startTime), <=, onTime + 1);
}

TEST(CcioTest, StateOutPulseBlockingMulti) {
    TEST_MODE_CHANGE(CcioMgr.PinByIndex(CLEARCORE_PIN_CCIOA0), Connector::ConnectorModes::OUTPUT_DIGITAL);

    uint32_t onTime = 100;
    uint32_t offTime = 200;
    uint32_t startTime, endTime;

    // Check that the blocking version of a multi-pulse call returns at about
    // the right time
    uint8_t pulses = 5;
    startTime = Milliseconds();
    CcioMgr.PinByIndex(CLEARCORE_PIN_CCIOA0)->OutputPulsesStart(onTime, offTime, pulses, true);
    endTime = Milliseconds();
    CHECK_COMPARE((endTime - startTime), >=, onTime * pulses + offTime * (pulses - 1));
    CHECK_COMPARE((endTime - startTime), <=, onTime * pulses + offTime * (pulses - 1) + 1);
}

TEST(CcioTest, StateOutPulseBlockingInfinite) {
    TEST_MODE_CHANGE(CcioMgr.PinByIndex(CLEARCORE_PIN_CCIOA0), Connector::ConnectorModes::OUTPUT_DIGITAL);

    uint32_t onTime = 100;
    uint32_t offTime = 200;
    uint32_t startTime, endTime;

    // Make sure that a blocking call with infinite pulses doesn't block
    startTime = Milliseconds();
    CcioMgr.PinByIndex(CLEARCORE_PIN_CCIOA0)->OutputPulsesStart(onTime, offTime, 0, true);
    endTime = Milliseconds();
    CHECK_COMPARE((endTime - startTime), <=, 1);

    // Ensure that it ends properly
    CcioMgr.PinByIndex(CLEARCORE_PIN_CCIOA0)->OutputPulsesStop();
    CHECK_FALSE(CcioMgr.PinByIndex(CLEARCORE_PIN_CCIOA0)->State());
}