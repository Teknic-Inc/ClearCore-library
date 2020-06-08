#include "CppUTest/TestHarness.h"

#include "testHooks.h"

namespace ClearCore {
extern SdCardDriver SdCard;
}

TEST_GROUP(SdCardTest) {
    void setup() {
    }

    void teardown() {
    }
};

TEST(SdCardTest, ValidSettings) {
    //CHECK_FALSE(SdCard.PortMode(SerialBase::USART));
    CHECK_TRUE(SdCard.PortMode(SerialBase::SPI));
    CHECK_TRUE(SdCard.SpiSsMode(SerialBase::CtrlLineModes::LINE_OFF));
    CHECK_TRUE(SdCard.SpiSsMode(SerialBase::CtrlLineModes::LINE_ON));
    CHECK_TRUE(SdCard.Speed(115200));
    CHECK_FALSE(SdCard.Speed(9600));
    CHECK_FALSE(SdCard.Parity(SerialBase::Parities::PARITY_E));
    CHECK_FALSE(SdCard.Parity(SerialBase::Parities::PARITY_O));
    CHECK_FALSE(SdCard.Parity(SerialBase::Parities::PARITY_N));
    CHECK_FALSE(SdCard.CharSize(1));
    CHECK_FALSE(SdCard.CharSize(2));
    CHECK_FALSE(SdCard.CharSize(3));
    CHECK_FALSE(SdCard.CharSize(4));
    CHECK_FALSE(SdCard.CharSize(5));
    CHECK_FALSE(SdCard.CharSize(6));
    CHECK_FALSE(SdCard.CharSize(7));
    CHECK_TRUE(SdCard.CharSize(8));
    CHECK_TRUE(SdCard.CharSize(9));
    CHECK_FALSE(SdCard.CharSize(10));
    CHECK_FALSE(SdCard.CharSize(50));
    CHECK_FALSE(SdCard.CharSize(255));
}

