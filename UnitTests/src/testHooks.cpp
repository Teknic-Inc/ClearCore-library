/**
    \file testHooks.cpp

    Created: 9/10/2019 12:19:39 PM
**/

#include "CppUTest/CommandLineTestRunner.h"
#include "testHooks.h"


extern "C" int debugPutChar(int c) {
    return TestIO::OutputPort()->SendChar(c);
}


extern "C" int RunTests() {

    int retVal = 0;
    TestIO::OutputPort()->PortClose();
    TestIO::OutputPort()->Speed(115200);
    TestIO::OutputPort()->PortOpen();
    while (!*TestIO::OutputPort());

    TestIO::OutputPort()->Send("Starting unit tests...\n");

    int ac = 2;
    char args[2][3] = {"xx", "-v"};
    char *av[2] = {args[0], args[1]};
    retVal = CommandLineTestRunner::RunAllTests(ac, (char **)av);

    TestIO::OutputPort()->Send("Unit tests Complete\n");
    return retVal;
}

namespace ClearCore {
volatile uint32_t TestIO::m_fakeInputs[CLEARCORE_PORT_MAX];
bool TestIO::m_usingFakeInputs = false;
ISerial *TestIO::m_outputPort = &ConnectorUsb;

void TestIO::UseFakeInputs(bool newVal /* = false */) {
    if (newVal) {
        if (!m_usingFakeInputs) {
            SysMgr.FastUpdate();
            SaveInputsToFakes();
        }
        InputMgr.SetInputRegisters(&TestIO::m_fakeInputs[0], &TestIO::m_fakeInputs[1], &TestIO::m_fakeInputs[2]);
    }
    else {
        InputMgr.SetInputRegisters(&PORT->Group[0].IN.reg, &PORT->Group[1].IN.reg, &PORT->Group[2].IN.reg);
    }
    m_usingFakeInputs = newVal;
}

void TestIO::SaveInputsToFakes() {
    for (int i = 0; i < CLEARCORE_PORT_MAX; i++) {
        TestIO::m_fakeInputs[i] = PORT->Group[i].IN.reg;
    }
}

void TestIO::FakeInput(DigitalIn &input, bool newVal) {
    if (!newVal) {
        TestIO::m_fakeInputs[input.m_inputPort] |= input.m_inputDataMask;
    }
    else {
        TestIO::m_fakeInputs[input.m_inputPort] &= ~input.m_inputDataMask;
    }
}
void TestIO::FakeHlfb(MotorDriver &mtr, bool newVal) {
    FakeInput(mtr, newVal);
}

void TestIO::FakeSerialInput(SerialDriver &serial, int16_t inputChar) {
    uint32_t nextIndex = serial.NextIndex(serial.m_inTail);
    if (nextIndex != serial.m_inHead) {
        serial.m_bufferIn[serial.m_inTail] = inputChar;
        serial.m_inTail = nextIndex;
    }
}

void TestIO::InitFakeInput(DigitalIn &input, bool initVal, uint16_t filtLen) {
    UseFakeInputs(true);
    SysMgr.FastUpdate();
    input.FilterLength(0);
    FakeInput(input, initVal);
    SysMgr.FastUpdate();
    input.FilterLength(filtLen);
}

bool TestIO::InputStateRT(DigitalIn &input) {
    return !static_cast<bool>(*InputMgr.m_inputPtrs[input.m_inputPort] &
                              input.m_inputDataMask);
}

volatile const uint16_t &TestIO::InputFilterTicksLeft(DigitalIn &input) {
    return input.m_filterTicksLeft;
}

uint32_t TestIO::StepGenUpdate(StepGenerator &stepGen) {
    stepGen.StepsCalculated();
    volatile uint32_t steps = stepGen.StepsPrevious();
    if (steps > 0) {
        return steps;
    }
    else {
        return 0;
    }
}

int64_t TestIO::StepGenPosn(StepGenerator &stepGen) {
    return stepGen.m_posnCurrentQx;
}

int32_t TestIO::StepGenVel(StepGenerator &stepGen) {
    return stepGen.m_velCurrentQx;
}

int32_t TestIO::StepGenVelLim(StepGenerator &stepGen) {
    return stepGen.m_velLimitQx;
}

int32_t TestIO::StepGenAccLim(StepGenerator &stepGen) {
    return stepGen.m_accelLimitQx;
}

void TestIO::ManualRefresh(bool isManual) {

    if (isManual) {
        NVIC_DisableIRQ(TCC0_0_IRQn);
        SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;
    }
    else {
        NVIC_EnableIRQ(TCC0_0_IRQn);
        SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;
    }

}
}