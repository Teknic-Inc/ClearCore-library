/*
 * testHooks.h
 *
 * Created: 9/13/2019 12:19:50 PM
 *  Author: scott_mayne
 */ 


#ifndef TESTHOOKS_H_
#define TESTHOOKS_H_

#include "CppUTest/UtestMacros.h"
#include "ClearCore.h"

#ifdef __cplusplus
extern "C" {
#endif
    int RunTests();
#ifdef __cplusplus
}
#endif


#define TEST_MODE_CHANGE(theConnector, newMode) \
    CHECK_TRUE(theConnector.Mode(newMode)); \
    LONGS_EQUAL(newMode, theConnector.Mode());

#define TEST_MODE_CHANGE_FAILS(theConnector, newMode) { \
    Connector::ConnectorModes oldMode = theConnector.Mode(); \
    CHECK_FALSE(theConnector.Mode(newMode)); \
    LONGS_EQUAL(oldMode, theConnector.Mode()); }

#define TEST_VAL_REFRESH(expected, actual, numRefreshes) \
    for (uint32_t i = 0; i < numRefreshes; i++) { \
        SysMgr.FastUpdate(); \
        LONGS_EQUAL(expected, actual); \
    }

#ifdef __cplusplus
namespace ClearCore {
    extern ShiftRegister ShiftReg;

class TestIO {
public:
	static void SaveInputsToFakes();
	static void UseFakeInputs(bool newVal = false);
	static void FakeInput(DigitalIn &input, bool newVal);
	static void FakeHlfb(MotorDriver &mtr, bool newVal);
    static void FakeSerialInput(SerialDriver &serial, int16_t inputChar);
	static uint32_t StepGenUpdate(StepGenerator &stepGen);
	static int64_t StepGenPosn(StepGenerator &stepGen);
	static int32_t StepGenVel(StepGenerator &stepGen);
	static int32_t StepGenVelLim(StepGenerator &stepGen);
	static int32_t StepGenAccLim(StepGenerator &stepGen);
	static void InitFakeInput(DigitalIn &input, bool initVal, uint16_t filtLen = 0);
    static bool InputStateRT(DigitalIn &input);
    static volatile const uint16_t& InputFilterTicksLeft(DigitalIn &input);
    static bool ShifterState(ShiftRegister::Masks bitToGet) {
        return ShiftReg.ShifterState(bitToGet);
    }

	static volatile uint32_t m_fakeInputs[CLEARCORE_PORT_MAX];
	static void ManualRefresh(bool isManual);
	static ISerial * OutputPort() {
		return m_outputPort;
	}
	static void OutputPort(ISerial *thePort) {
		m_outputPort = thePort;
	}
private:
	TestIO() {};
	static bool m_usingFakeInputs;
	static ISerial *m_outputPort;
};
} // ClearCore namespace
#endif

#endif /* TESTHOOKS_H_ */