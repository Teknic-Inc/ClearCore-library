/**
    \file SerialTest.cpp

    A collection of software tests for the ClearCore's SerialDriver class.
**/

#include "CppUTest/TestHarness.h"
#include "testHooks.h"

Connector::ConnectorModes com0Mode;

TEST_GROUP(SerialTest) {
    void setup() {
        com0Mode = ConnectorCOM0.Mode();
        ConnectorCOM0.Flush();
        ConnectorCOM0.FlushInput();
        ConnectorCOM0.Reinitialize();
    }

    void teardown() {
        ConnectorCOM0.Mode(com0Mode);
        ConnectorCOM0.Flush();
        ConnectorCOM0.FlushInput();
    }
};

TEST(SerialTest, InitialState) {
    LONGS_EQUAL(Connector::ConnectorModes::TTL, ConnectorCOM0.Mode());
    LONGS_EQUAL(SerialBase::EOB, ConnectorCOM0.CharGet());
    LONGS_EQUAL(false, ConnectorCOM0.State());
    LONGS_EQUAL(Connector::SERIAL_TYPE, ConnectorCOM0.Type());
    CHECK_FALSE(ConnectorCOM0.IsWritable());
    CHECK_FALSE(ConnectorCOM0.PortIsOpen());
    CHECK_FALSE(ConnectorCOM0.IsInHwFault());
}

TEST(SerialTest, ModeCheckWithValidModes) {
    TEST_MODE_CHANGE(ConnectorCOM0, Connector::ConnectorModes::RS232);
    TEST_MODE_CHANGE(ConnectorCOM0, Connector::ConnectorModes::CCIO);
    TEST_MODE_CHANGE(ConnectorCOM0, Connector::ConnectorModes::SPI);
    TEST_MODE_CHANGE(ConnectorCOM0, Connector::ConnectorModes::TTL);
}

TEST(SerialTest, ModeCheckWithInvalidModes) {
    TEST_MODE_CHANGE_FAILS(ConnectorCOM0, Connector::ConnectorModes::INPUT_ANALOG);
    TEST_MODE_CHANGE_FAILS(ConnectorCOM0, Connector::ConnectorModes::INPUT_DIGITAL);
    TEST_MODE_CHANGE_FAILS(ConnectorCOM0, Connector::ConnectorModes::OUTPUT_ANALOG);
    TEST_MODE_CHANGE_FAILS(ConnectorCOM0, Connector::ConnectorModes::OUTPUT_DIGITAL);
    TEST_MODE_CHANGE_FAILS(ConnectorCOM0, Connector::ConnectorModes::OUTPUT_H_BRIDGE);
    TEST_MODE_CHANGE_FAILS(ConnectorCOM0, Connector::ConnectorModes::OUTPUT_PWM);
    TEST_MODE_CHANGE_FAILS(ConnectorCOM0, Connector::ConnectorModes::OUTPUT_TONE);
    TEST_MODE_CHANGE_FAILS(ConnectorCOM0, Connector::ConnectorModes::OUTPUT_WAVE);
    TEST_MODE_CHANGE_FAILS(ConnectorCOM0, Connector::ConnectorModes::CPM_MODE_A_DIRECT_B_DIRECT);
    TEST_MODE_CHANGE_FAILS(ConnectorCOM0, Connector::ConnectorModes::CPM_MODE_STEP_AND_DIR);
    TEST_MODE_CHANGE_FAILS(ConnectorCOM0, Connector::ConnectorModes::CPM_MODE_A_DIRECT_B_PWM);
    TEST_MODE_CHANGE_FAILS(ConnectorCOM0, Connector::ConnectorModes::CPM_MODE_A_PWM_B_PWM);
    TEST_MODE_CHANGE_FAILS(ConnectorCOM0, Connector::ConnectorModes::USB_CDC);
    LONGS_EQUAL(Connector::ConnectorModes::TTL, ConnectorCOM0.Mode());
}

TEST(SerialTest, ValidSettings) {
    CHECK_TRUE(ConnectorCOM0.PortMode(SerialBase::SPI));
    CHECK_FALSE(ConnectorCOM0.CharSize(5));     // Valid settings are 8,9
    CHECK_FALSE(ConnectorCOM0.CharSize(7));
    CHECK_TRUE(ConnectorCOM0.CharSize(8));
    CHECK_TRUE(ConnectorCOM0.CharSize(9));
    CHECK_FALSE(ConnectorCOM0.CharSize(10));
    CHECK_FALSE(ConnectorCOM0.CharSize(12));
    CHECK_TRUE(SdCard.Speed(115200));
    CHECK_FALSE(SdCard.Speed(9600));
    CHECK_FALSE(ConnectorCOM0.Parity(SerialBase::Parities::PARITY_E));
    CHECK_FALSE(ConnectorCOM0.Parity(SerialBase::Parities::PARITY_O));
    CHECK_FALSE(ConnectorCOM0.Parity(SerialBase::Parities::PARITY_N));

    //CHECK_TRUE(ConnectorCOM0.PortMode(SerialBase::USART));
    CHECK_FALSE(ConnectorCOM0.CharSize(0));     // Valid settings are 5,6,7,8,9
    CHECK_FALSE(ConnectorCOM0.CharSize(1));
    CHECK_FALSE(ConnectorCOM0.CharSize(4));
    CHECK_TRUE(ConnectorCOM0.CharSize(5));
    CHECK_TRUE(ConnectorCOM0.CharSize(6));
    CHECK_TRUE(ConnectorCOM0.CharSize(7));
    CHECK_TRUE(ConnectorCOM0.CharSize(8));
    CHECK_TRUE(ConnectorCOM0.CharSize(9));
    CHECK_FALSE(ConnectorCOM0.CharSize(10));
    CHECK_FALSE(ConnectorCOM0.CharSize(12));
    CHECK_TRUE(SdCard.Speed(115200));
    CHECK_FALSE(SdCard.Speed(9600));
    CHECK_TRUE(ConnectorCOM0.Parity(SerialBase::Parities::PARITY_E));
    CHECK_TRUE(ConnectorCOM0.Parity(SerialBase::Parities::PARITY_O));
    CHECK_TRUE(ConnectorCOM0.Parity(SerialBase::Parities::PARITY_N));
}

TEST(SerialTest, AvailableForReadTest) {
    // AvailableForRead() and State()
    LONGS_EQUAL(SerialBase::EOB, ConnectorCOM0.CharGet());
    LONGS_EQUAL(0, ConnectorCOM0.AvailableForRead());
    int16_t charToRead = rand() % UINT8_MAX;
    uint8_t numberCharToRead = rand() % (SERIAL_BUFFER_SIZE - 1);
    for (uint8_t i = 0; i < numberCharToRead; i++) {
        TestIO::FakeSerialInput(ConnectorCOM0, charToRead);
    }
    LONGS_EQUAL(numberCharToRead, ConnectorCOM0.AvailableForRead());
    for (uint8_t i = 0; i < numberCharToRead - 1; i++) {
        ConnectorCOM0.CharGet();
    }
    LONGS_EQUAL(charToRead, ConnectorCOM0.CharGet());
    LONGS_EQUAL(SerialBase::EOB, ConnectorCOM0.CharGet());

    charToRead = rand() % INT16_MAX;
    TestIO::FakeSerialInput(ConnectorCOM0, charToRead);
    LONGS_EQUAL(1, ConnectorCOM0.AvailableForRead());
    LONGS_EQUAL(charToRead, ConnectorCOM0.CharGet());
    LONGS_EQUAL(0, ConnectorCOM0.AvailableForRead());
    LONGS_EQUAL(SerialBase::EOB, ConnectorCOM0.CharGet());
}

TEST(SerialTest, FlushBuffersTest) {
    // FlushInput()
    ConnectorCOM0.PortOpen();
    LONGS_EQUAL(0, ConnectorCOM0.AvailableForRead());
    int16_t inputChar = rand() % UINT8_MAX;
    int16_t charAmt = (rand() % (SERIAL_BUFFER_SIZE - 1 - 20)) + 20; // non-zero
    for (int16_t i = 0; i < charAmt; i++) {
        TestIO::FakeSerialInput(ConnectorCOM0, inputChar);
    }
    LONGS_EQUAL(charAmt, ConnectorCOM0.AvailableForRead());

    ConnectorCOM0.FlushInput();

    LONGS_EQUAL(0, ConnectorCOM0.AvailableForRead());
    LONGS_EQUAL(SerialBase::EOB, ConnectorCOM0.CharPeek());
    LONGS_EQUAL(SerialBase::EOB, ConnectorCOM0.CharGet());

    // Flush()
    int16_t outputChar = rand() % UINT8_MAX;
    charAmt = (rand() % SERIAL_BUFFER_SIZE) + SERIAL_BUFFER_SIZE; // flood the output buffer
    for (int16_t i = 0; i < charAmt; i++) {
        ConnectorCOM0.SendChar(outputChar);
    }
    CHECK_COMPARE(ConnectorCOM0.AvailableForWrite(), !=, SERIAL_BUFFER_SIZE - 1);

    ConnectorCOM0.Flush();

    LONGS_EQUAL(SERIAL_BUFFER_SIZE - 1, ConnectorCOM0.AvailableForWrite());
}

TEST(SerialTest, CharPeekTest) {
    // CharGet() and CharPeek()
    LONGS_EQUAL(SerialBase::EOB, ConnectorCOM0.CharGet());  // Initial state is empty
    LONGS_EQUAL(SerialBase::EOB, ConnectorCOM0.CharPeek());

    int16_t peekChar = rand() % UINT8_MAX;
    int16_t readChar = rand() % UINT8_MAX;
    TestIO::FakeSerialInput(ConnectorCOM0, peekChar);
    TestIO::FakeSerialInput(ConnectorCOM0, readChar);

    // Consecutive calls to CharPeek() return the same char.
    LONGS_EQUAL(peekChar, ConnectorCOM0.CharPeek());
    LONGS_EQUAL(peekChar, ConnectorCOM0.CharPeek());
    LONGS_EQUAL(peekChar, ConnectorCOM0.CharPeek());

    // CharGet() returns the same value as peekk.
    LONGS_EQUAL(ConnectorCOM0.CharPeek(), ConnectorCOM0.CharGet());
    LONGS_EQUAL(readChar, ConnectorCOM0.CharGet());

}

TEST(SerialTest, AvailableForWriteTest) {
    LONGS_EQUAL(SERIAL_BUFFER_SIZE - 1, ConnectorCOM0.AvailableForWrite());
    int16_t charToSend = rand() % UINT8_MAX;
    uint8_t numberCharToSend = rand() % SERIAL_BUFFER_SIZE - 1;
    CHECK_FALSE(ConnectorCOM0.SendChar(charToSend));
    ConnectorCOM0.PortOpen();
    LONGS_EQUAL(true, ConnectorCOM0.State());
    CHECK_TRUE(ConnectorCOM0.SendChar(charToSend));
    for (uint8_t i = 0; i < numberCharToSend - 1; i++) {
        ConnectorCOM0.SendChar(charToSend);
    }
    // Unless we disable interrupts, we lose a few char that are sent out before we read the number.
    CHECK_TRUE(ConnectorCOM0.AvailableForWrite() >= (SERIAL_BUFFER_SIZE - 1 - numberCharToSend));
}