#include "CppUTest/TestHarness.h"
#include "testHooks.h"


namespace ClearCore {
extern volatile uint32_t tickCnt;
}

TEST_GROUP(AdcTest) {
    uint32_t lastTick;
    void setup() {
        ConnectorA9.Reinitialize();
        ConnectorA10.Reinitialize();
        ConnectorA11.Reinitialize();
        ConnectorA12.Reinitialize();
        AdcMgr.Initialize();
        lastTick = tickCnt;
        while (tickCnt - lastTick < 2) {
            continue;
        }
    }

    void teardown() {
        ConnectorA9.Reinitialize();
        ConnectorA10.Reinitialize();
        ConnectorA11.Reinitialize();
        ConnectorA12.Reinitialize();
        AdcMgr.Initialize();
    }
};

TEST(AdcTest, InitialState) {
    LONGS_EQUAL(Connector::ConnectorModes::INPUT_ANALOG, ConnectorA9.Mode());
    LONGS_EQUAL(Connector::ConnectorModes::INPUT_ANALOG, ConnectorA10.Mode());
    LONGS_EQUAL(Connector::ConnectorModes::INPUT_ANALOG, ConnectorA11.Mode());
    LONGS_EQUAL(Connector::ConnectorModes::INPUT_ANALOG, ConnectorA12.Mode());
    LONGS_EQUAL(12, AdcMgr.AdcResolution());
    CHECK_FALSE(AdcMgr.AdcTimeout());

    CHECK_FALSE(ConnectorA9.IsWritable());
    CHECK_FALSE(ConnectorA10.IsWritable());
    CHECK_FALSE(ConnectorA11.IsWritable());
    CHECK_FALSE(ConnectorA12.IsWritable());

    CHECK_FALSE(ConnectorA9.IsInHwFault());
    CHECK_FALSE(ConnectorA10.IsInHwFault());
    CHECK_FALSE(ConnectorA11.IsInHwFault());
    CHECK_FALSE(ConnectorA12.IsInHwFault());

    CHECK_COMPARE(ConnectorA9.State(), >=, 0);
    CHECK_COMPARE(ConnectorA10.State(), >=, 0);
    CHECK_COMPARE(ConnectorA11.State(), >=, 0);
    CHECK_COMPARE(ConnectorA12.State(), >=, 0);

    Delay_ms(100);

    CHECK_COMPARE(ConnectorA9.State(), <=, 200);
    CHECK_COMPARE(ConnectorA10.State(), <=, 200);
    CHECK_COMPARE(ConnectorA11.State(), <=, 200);
    CHECK_COMPARE(ConnectorA12.State(), <=, 200);
}

TEST(AdcTest, InitialReadingDelay) {
    uint32_t timestamps[2] = {0};
    timestamps[0] = tickCnt;
    ConnectorA9.Mode(Connector::ConnectorModes::INPUT_ANALOG);
    timestamps[1] = tickCnt;

    // Check that there was an appropriate delay to switch from
    // INPUT_DIGITAL mode to INPUT_ANALOG mode and get a valid reading
    CHECK(timestamps[1] - timestamps[0] < 2);
}

TEST(AdcTest, ReadingDelayAfterDigital) {
    uint32_t timestamps[2] = {0};
    ConnectorA9.Mode(Connector::ConnectorModes::INPUT_DIGITAL);
    timestamps[0] = tickCnt;
    ConnectorA9.Mode(Connector::ConnectorModes::INPUT_ANALOG);
    timestamps[1] = tickCnt;

    // Check that there was an appropriate delay to switch from
    // INPUT_DIGITAL mode to INPUT_ANALOG mode and get a valid reading
    CHECK(timestamps[1] - timestamps[0] > 3);
    CHECK(timestamps[1] - timestamps[0] < 6);
}


#define ADC_DITHER_READINGS    100
#define ADC_DITHER_MARGIN      0x0090
TEST(AdcTest, AdcDither) {
    uint16_t adcReadings[ADC_DITHER_READINGS] = {0};
    uint8_t iReading = 0;
    uint16_t dither;
    ConnectorA9.Mode(Connector::ConnectorModes::INPUT_ANALOG);
    for (iReading = 0; iReading < ADC_DITHER_READINGS; iReading++) {
        adcReadings[iReading] = AdcMgr.ConvertedResult(AdcManager::ADC_AIN09);
        lastTick = tickCnt;
        // Test that the ADC reading dither is within reason
        dither = labs(adcReadings[iReading] - adcReadings[0]);
        if (dither > ADC_DITHER_MARGIN) {
            // We already failed, print out the values so they can be useful
            LONGS_EQUAL(ADC_DITHER_MARGIN, dither);
            break;
        }
        while (lastTick == tickCnt) {
            continue;
        }
    }
    // Check that we compared all of the readings that we wanted to
    LONGS_EQUAL(ADC_DITHER_READINGS, iReading);
}

TEST(AdcTest, FilterTc100) {
    ConnectorA9.Mode(Connector::ConnectorModes::INPUT_DIGITAL);

    uint16_t maxVal =
        (UINT16_MAX >> 1) & (UINT16_MAX << (15 - AdcMgr.AdcResolution()));
    // The ADC reading of an AnalogInDigitalIn pin in digital mode is maxVal
    // This gives a good stable value to test the filtering time constants
    LONGS_EQUAL(maxVal, AdcMgr.ConvertedResult(AdcManager::ADC_AIN09));
    AdcMgr.FilterTc(AdcManager::ADC_AIN09, 100, AdcManager::FILTER_UNIT_SAMPLES);
    AdcMgr.FilterReset(AdcManager::ADC_AIN09, 0);
    lastTick = tickCnt;
    // The filter time constant is defined by the time to rise to 99%
    // of the final value, so test that the filter is hitting the 99%
    // mark near the filter length
    while (tickCnt - lastTick < 90) {
        continue;
    }
    CHECK(AdcMgr.FilteredResult(AdcManager::ADC_AIN09) < maxVal * .988);
    while (tickCnt - lastTick < 100) {
        continue;
    }
    CHECK(AdcMgr.FilteredResult(AdcManager::ADC_AIN09) > maxVal * .988);
}

TEST(AdcTest, FilterTc10) {
    ConnectorA9.Mode(Connector::ConnectorModes::INPUT_DIGITAL);

    uint16_t maxVal =
        (UINT16_MAX >> 1) & (UINT16_MAX << (15 - AdcMgr.AdcResolution()));

    // The ADC reading of an AnalogInDigitalIn pin in digital mode is 0x7ff8
    // This gives a good stable value to test the filtering time constants
    LONGS_EQUAL(maxVal, AdcMgr.ConvertedResult(AdcManager::ADC_AIN09));
    AdcMgr.FilterTc(AdcManager::ADC_AIN09, 10, AdcManager::FILTER_UNIT_SAMPLES);
    AdcMgr.FilterReset(AdcManager::ADC_AIN09, 0);
    lastTick = tickCnt;
    // The filter time constant is defined by the time to rise to 99%
    // of the final value, so test that the filter is hitting the 99%
    // mark near the filter length
    while (tickCnt - lastTick < 9) {
        LONGS_EQUAL(maxVal, AdcMgr.ConvertedResult(AdcManager::ADC_AIN09));
    }
    CHECK(AdcMgr.FilteredResult(AdcManager::ADC_AIN09) < maxVal * .988);
    while (tickCnt - lastTick < 10) {
        continue;
    }
    CHECK(AdcMgr.FilteredResult(AdcManager::ADC_AIN09) > maxVal * .988);
}

TEST(AdcTest, FilterTcMS) {
    ConnectorA9.Mode(Connector::ConnectorModes::INPUT_DIGITAL);
    uint16_t filterLenSamples;
    uint16_t maxVal =
        (UINT16_MAX >> 1) & (UINT16_MAX << (15 - AdcMgr.AdcResolution()));

    // The ADC reading of an AnalogInDigitalIn pin in digital mode is 0x7ff8
    // This gives a good stable value to test the filtering time constants
    LONGS_EQUAL(maxVal, AdcMgr.ConvertedResult(AdcManager::ADC_AIN09));
    AdcMgr.FilterTc(AdcManager::ADC_AIN09, 10, AdcManager::FILTER_UNIT_MS);
    filterLenSamples = AdcMgr.FilterTc(AdcManager::ADC_AIN09, AdcManager::FILTER_UNIT_SAMPLES);
    AdcMgr.FilterReset(AdcManager::ADC_AIN09, 0);
    lastTick = tickCnt;
    // The filter time constant is defined by the time to rise to 99%
    // of the final value, so test that the filter is hitting the 99%
    // mark near the filter length
    while (tickCnt - lastTick < .9 * filterLenSamples) {
        LONGS_EQUAL(maxVal, AdcMgr.ConvertedResult(AdcManager::ADC_AIN09));
    }
    CHECK(AdcMgr.FilteredResult(AdcManager::ADC_AIN09) < maxVal * .988);
    while (tickCnt - lastTick < filterLenSamples) {
        continue;
    }
    CHECK(AdcMgr.FilteredResult(AdcManager::ADC_AIN09) > maxVal * .988);
}

TEST(AdcTest, FilterTcRaw) {
    ConnectorA9.Mode(Connector::ConnectorModes::INPUT_DIGITAL);

    uint16_t maxVal =
        (UINT16_MAX >> 1) & (UINT16_MAX << (15 - AdcMgr.AdcResolution()));

    // The ADC reading of an AnalogInDigitalIn pin in digital mode is 0x7ff8
    // This gives a good stable value to test the filtering time constants
    LONGS_EQUAL(maxVal, AdcMgr.ConvertedResult(AdcManager::ADC_AIN09));
    AdcMgr.FilterTc(AdcManager::ADC_AIN09, 20675, AdcManager::FILTER_UNIT_RAW);
    AdcMgr.FilterReset(AdcManager::ADC_AIN09, 0);
    lastTick = tickCnt;
    // The filter time constant is defined by the time to rise to 99%
    // of the final value, so test that the filter is hitting the 99%
    // mark near the filter length
    while (tickCnt - lastTick < 9) {
        LONGS_EQUAL(maxVal, AdcMgr.ConvertedResult(AdcManager::ADC_AIN09));
    }
    CHECK(AdcMgr.FilteredResult(AdcManager::ADC_AIN09) < maxVal * .988);
    while (tickCnt - lastTick < 10) {
        continue;
    }
    CHECK(AdcMgr.FilteredResult(AdcManager::ADC_AIN09) > maxVal * .988);
}


TEST(AdcTest, FilteredResultTest) {
    ConnectorA9.Mode(Connector::ConnectorModes::INPUT_DIGITAL);

    uint16_t maxVal =
        (UINT16_MAX >> 1) & (UINT16_MAX << (15 - AdcMgr.AdcResolution()));

    // The ADC reading of an AnalogInDigitalIn pin in digital mode is 0x7ff8
    // This gives a good stable value to test the filtering time constants
    LONGS_EQUAL(maxVal, AdcMgr.ConvertedResult(AdcManager::ADC_AIN09));
    AdcMgr.FilterTc(AdcManager::ADC_AIN09, 10, AdcManager::FILTER_UNIT_SAMPLES);
    AdcMgr.FilterReset(AdcManager::ADC_AIN09, 0);
    lastTick = tickCnt;
    // The filter time constant is defined by the time to rise to 99%
    // of the final value, so test that the filter is hitting the 99%
    // mark near the filter length
    while (tickCnt - lastTick < 9) {
        continue;
    }
    CHECK(AdcMgr.FilteredResult(AdcManager::ADC_AIN09) < maxVal * .988);
    while (tickCnt - lastTick < 10) {
        continue;
    }
    CHECK(AdcMgr.FilteredResult(AdcManager::ADC_AIN09) > maxVal * .988);
}

TEST(AdcTest, ConvertedResultTest) {
    ConnectorA9.Mode(Connector::ConnectorModes::INPUT_DIGITAL);

    uint16_t maxVal =
        (UINT16_MAX >> 1) & (UINT16_MAX << (15 - AdcMgr.AdcResolution()));

    // The ADC reading of an AnalogInDigitalIn pin in digital mode is 0x7ff8
    // This gives a good stable value to test the filtering time constants
    LONGS_EQUAL(maxVal, AdcMgr.ConvertedResult(AdcManager::ADC_AIN09));
    AdcMgr.FilterTc(AdcManager::ADC_AIN09, 10, AdcManager::FILTER_UNIT_SAMPLES);
    AdcMgr.FilterReset(AdcManager::ADC_AIN09, 0);
    lastTick = tickCnt;
    // The filter time constant is defined by the time to rise to 99%
    // of the final value, so test that the filter is hitting the 99%
    // mark near the filter length
    while (tickCnt - lastTick < 9) {
        LONGS_EQUAL(maxVal, AdcMgr.ConvertedResult(AdcManager::ADC_AIN09));
    }
    CHECK(AdcMgr.FilteredResult(AdcManager::ADC_AIN09) < maxVal * .988);
    while (tickCnt - lastTick < 10) {
        continue;
    }
    CHECK(AdcMgr.FilteredResult(AdcManager::ADC_AIN09) > maxVal * .988);
}

TEST(AdcTest, Resolution) {
    ConnectorA9.Mode(Connector::ConnectorModes::INPUT_DIGITAL);
    // Make sure an invalid input is rejected
    bool successFlag;
    successFlag = AdcMgr.AdcResolution(25);
    CHECK(successFlag == false);

    uint16_t maxVal =
        (UINT16_MAX >> 1) & (UINT16_MAX << (15 - AdcMgr.AdcResolution()));

    LONGS_EQUAL(12, AdcMgr.AdcResolution());
    LONGS_EQUAL(maxVal, AdcMgr.ConvertedResult(AdcManager::ADC_AIN09));
    AdcMgr.AdcResolution(8);
    lastTick = tickCnt;
    while (tickCnt - lastTick < 2) {
        continue;
    }
    LONGS_EQUAL(8, AdcMgr.AdcResolution());
    maxVal = (UINT16_MAX >> 1) & (UINT16_MAX << (15 - AdcMgr.AdcResolution()));
    LONGS_EQUAL(maxVal, AdcMgr.ConvertedResult(AdcManager::ADC_AIN09));
    AdcMgr.AdcResolution(10);
    lastTick = tickCnt;
    while (tickCnt - lastTick < 2) {
        continue;
    }
    LONGS_EQUAL(10, AdcMgr.AdcResolution());
    maxVal = (UINT16_MAX >> 1) & (UINT16_MAX << (15 - AdcMgr.AdcResolution()));
    LONGS_EQUAL(maxVal, AdcMgr.ConvertedResult(AdcManager::ADC_AIN09));
}

TEST(AdcTest, FilterTcInvalid) {
    ConnectorA9.Mode(Connector::ConnectorModes::INPUT_DIGITAL);
    bool successFlag;
    uint16_t maxVal =
        (UINT16_MAX >> 1) & (UINT16_MAX << (15 - AdcMgr.AdcResolution()));

    // The ADC reading of an AnalogInDigitalIn pin in digital mode is 0x7ff8
    // This gives a good stable value to test the filtering time constants
    LONGS_EQUAL(maxVal, AdcMgr.ConvertedResult(AdcManager::ADC_AIN09));
    successFlag = AdcMgr.FilterTc(AdcManager::ADC_AIN09, 10, (AdcManager::FilterUnits)25);
    CHECK(successFlag == false);
}

TEST(AdcTest, FilterTcInvalidChannel) {
    CHECK_FALSE(AdcMgr.FilterTc(AdcManager::ADC_CHANNEL_COUNT, 10, AdcManager::FILTER_UNIT_RAW));
    CHECK_FALSE(AdcMgr.FilterTc((AdcManager::AdcChannels) - 1, 10, AdcManager::FILTER_UNIT_RAW));
    CHECK_FALSE(AdcMgr.FilterTc((AdcManager::AdcChannels)(AdcManager::ADC_CHANNEL_COUNT + 1), 10, AdcManager::FILTER_UNIT_RAW));

    CHECK_FALSE(AdcMgr.FilterTc(AdcManager::ADC_CHANNEL_COUNT, AdcManager::FILTER_UNIT_RAW));
    CHECK_FALSE(AdcMgr.FilterTc((AdcManager::AdcChannels) - 1, AdcManager::FILTER_UNIT_RAW));
    CHECK_FALSE(AdcMgr.FilterTc((AdcManager::AdcChannels)(AdcManager::ADC_CHANNEL_COUNT + 1), AdcManager::FILTER_UNIT_RAW));
}

TEST(AdcTest, FilterResetTest) {
    ConnectorA9.Mode(Connector::ConnectorModes::INPUT_DIGITAL);

    uint16_t maxVal =
        (UINT16_MAX >> 1) & (UINT16_MAX << (15 - AdcMgr.AdcResolution()));
    uint16_t resetVal = 0;

    LONGS_EQUAL(maxVal, AdcMgr.ConvertedResult(AdcManager::ADC_AIN09));

    AdcMgr.FilterTc(AdcManager::ADC_AIN09, 10, AdcManager::FILTER_UNIT_SAMPLES);
    AdcMgr.FilterReset(AdcManager::ADC_AIN09, resetVal);
    LONGS_EQUAL(resetVal, AdcMgr.FilteredResult(AdcManager::ADC_AIN09));
}

TEST(AdcTest, DigitalInState) {
    TEST_MODE_CHANGE(ConnectorA9, Connector::ConnectorModes::INPUT_DIGITAL);
    TEST_MODE_CHANGE(ConnectorA10, Connector::ConnectorModes::INPUT_DIGITAL);
    TEST_MODE_CHANGE(ConnectorA11, Connector::ConnectorModes::INPUT_DIGITAL);
    TEST_MODE_CHANGE(ConnectorA12, Connector::ConnectorModes::INPUT_DIGITAL);

    CHECK_FALSE(ConnectorA9.IsWritable());
    CHECK_FALSE(ConnectorA10.IsWritable());
    CHECK_FALSE(ConnectorA11.IsWritable());
    CHECK_FALSE(ConnectorA12.IsWritable());

    CHECK_FALSE(ConnectorA9.IsInHwFault());
    CHECK_FALSE(ConnectorA10.IsInHwFault());
    CHECK_FALSE(ConnectorA11.IsInHwFault());
    CHECK_FALSE(ConnectorA12.IsInHwFault());

    CHECK_FALSE(ConnectorA9.State());
    CHECK_FALSE(ConnectorA10.State());
    CHECK_FALSE(ConnectorA11.State());
    CHECK_FALSE(ConnectorA12.State());
}