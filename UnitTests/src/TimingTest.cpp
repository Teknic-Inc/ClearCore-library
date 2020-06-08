/*
 * TimingTest.cpp
 *
 * Created: 10/11/2019 4:04:37 PM
 */

#include "CppUTest/TestHarness.h"
#include "testHooks.h"

TEST_GROUP(TimingTest) {
    void setup() {
    }

    void teardown() {
        TestIO::ManualRefresh(false);
    }
};

TEST(TimingTest, HalfSecondDelay) {
    uint32_t startMillis = Milliseconds();
    uint32_t startMicros = Microseconds();
    Delay_ms(500);
    uint32_t endMillis = Milliseconds();
    uint32_t endMicros = Microseconds();

    CHECK_COMPARE(endMillis - startMillis, >=, 500);
    CHECK_COMPARE(endMillis - startMillis, <=, 501);
    CHECK_COMPARE(endMicros - startMicros, >=, 500000);
    CHECK_COMPARE(endMicros - startMicros, <=, 500200);
}

TEST(TimingTest, TwoSecondDelay) {
    uint32_t startMillis = Milliseconds();
    uint32_t startMicros = Microseconds();
    Delay_ms(2000);
    uint32_t endMillis = Milliseconds();
    uint32_t endMicros = Microseconds();

    CHECK_COMPARE(endMillis - startMillis, >=, 2000);
    CHECK_COMPARE(endMillis - startMillis, <=, 2001);
    CHECK_COMPARE(endMicros - startMicros, >=, 2000000);
    CHECK_COMPARE(endMicros - startMicros, <=, 2000200);
}

TEST(TimingTest, ThreeMinuteGlitchCheck) {
    uint32_t startMillis = Milliseconds();
    uint32_t startMicros = Microseconds();
    uint32_t currentMicros;
    uint32_t lastMicros = startMicros;
    uint32_t increment, maxIncrement = 0;

    while (Milliseconds() - startMillis <= 180000) {
        currentMicros = Microseconds();
        increment = currentMicros - lastMicros;
        if (increment > maxIncrement) {
            maxIncrement = increment;
        }
        lastMicros = currentMicros;
    }
    uint32_t endMillis = Milliseconds();
    lastMicros = Microseconds();
    CHECK_COMPARE(maxIncrement, <, 200);
    LONGS_EQUAL(180001, endMillis - startMillis);
    CHECK_COMPARE(lastMicros - startMicros, >=, 180000000);
    CHECK_COMPARE(lastMicros - startMicros, <=, 180001200);
}

TEST(TimingTest, MicroSecondWrap) {
    TimingMgr.ResetMicroseconds();
    uint32_t wrapCnt = 0, loopCnt = 0;
    uint32_t currentMicros;
    uint32_t lastMicros = Microseconds();
    uint32_t increment, maxIncrement = 0, minIncrement = UINT32_MAX;

    while (wrapCnt < 2) {
        // To avoid having the test take over an hour per wrap,
        // modify the CPU cycle counter directly to speed things along
        DWT->CYCCNT += UINT32_MAX / 8 - CYCLES_PER_MICROSECOND * 500;
        // Allow the TimingMgr update to prepare for the upcoming wrap
        Delay_us(500);
        currentMicros = Microseconds();

        if (currentMicros < lastMicros) {
            wrapCnt++;
        }
        increment = currentMicros - lastMicros;
        if (increment > maxIncrement) {
            maxIncrement = increment;
        }
        if (increment < minIncrement) {
            minIncrement = increment;
        }
        lastMicros = currentMicros;
        loopCnt++;
    }

    LONGS_EQUAL(2, wrapCnt);
    // Did we execute the loop the expected number of times?
    LONGS_EQUAL(wrapCnt * 8 * CYCLES_PER_MICROSECOND, loopCnt);
    // Did the values returned by the Microsecond calls increment as expected?
    CHECK_COMPARE(maxIncrement, <, UINT32_MAX / (8 * CYCLES_PER_MICROSECOND) + 1200);
    CHECK_COMPARE(minIncrement, >=, UINT32_MAX / (8 * CYCLES_PER_MICROSECOND));
}

TEST(TimingTest, CycleCounterWrap) {
    TestIO::ManualRefresh(true);
    TimingMgr.ResetMicroseconds();
    SysMgr.FastUpdate();
    // Allow the TimingMgr update to prepare for the upcoming wrap
    DWT->CYCCNT = UINT32_MAX / 2;
    SysMgr.FastUpdate();
    DWT->CYCCNT = UINT32_MAX / 4 * 3;
    SysMgr.FastUpdate();

    // Time to make the cycle counter wrap
    DWT->CYCCNT = UINT32_MAX - (CYCLES_PER_MILLISECOND * 2);
    uint32_t startMicros = Microseconds();
    Delay_ms(4);
    uint32_t endMicros = Microseconds();

    // Check that the cycle counter wrapped, but the microseconds kept incrementing
    CHECK_COMPARE(DWT->CYCCNT, <, CYCLES_PER_MILLISECOND * 3);
    CHECK_COMPARE(endMicros - startMicros, <=, 4200);
    CHECK_COMPARE(endMicros - startMicros, >=, 4000);
    CHECK_COMPARE(endMicros, >, startMicros);
}