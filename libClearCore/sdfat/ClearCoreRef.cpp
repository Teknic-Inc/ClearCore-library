/*
 * ClearCoreRef.cpp
 *
 * Created: 10/6/2020 3:11:58 PM
 *  Author: allen_wells
 */ 

#include <sam.h>
#include "AdcManager.h"
#include "DigitalInOutAnalogOut.h"
#include "DigitalInAnalogIn.h"
#include "DigitalInOut.h"
#include "DigitalIn.h"
#include "DigitalInOutHBridge.h"
#include "InputManager.h"
#include "LedDriver.h"
#include "MotorDriver.h"
#include "SysConnectors.h"
#include "SysManager.h"
#include "SysUtils.h"
#include "CcioBoardManager.h"
#include "ShiftRegister.h"
#include "MotorManager.h"
#include "ClearCoreRef.h"
#include "ClearCore.h"

using ClearCore::Connector;
using ClearCore::SerialBase;
using ClearCore::SerialDriver;
using ClearCore::ISerial;


namespace ClearCore {
	extern SdCardDriver SdCard;
	extern SerialDriver ConnectorCOM0;
	extern SerialDriver ConnectorCOM1;
}

void digitalWriteClearCore(pin_size_t conNum, PinStatus ulVal) {
	// Get a reference to the appropriate connector
	ClearCore::Connector *connector =
	ClearCore::SysMgr.ConnectorByIndex(
	static_cast<ClearCorePins>(conNum));

	// If connector cannot be written, just return
	if (!connector || !connector->IsWritable()) {
		return;
	}

	connector->Mode(ClearCore::Connector::OUTPUT_DIGITAL);
	if (connector->Mode() == ClearCore::Connector::OUTPUT_DIGITAL) {
		connector->State(ulVal);
	}
}

/**
    This function replaces pinModeAPI in most cases as ClearCore uses
    a connector model in place of the pin model of the traditional Arduino
    implementations.
**/
void pinModeClearCore(pin_size_t pinNumber, uint32_t ulMode) {
	pinNumber = (ClearCorePins)pinNumber;
	ulMode = (PinMode)ulMode;
    // Get a reference to the appropriate connector
    ClearCore::Connector *connector =
        ClearCore::SysMgr.ConnectorByIndex(
            static_cast<ClearCorePins>(pinNumber));

    if (!connector) {
        return;
    }

    switch (ulMode) {
        case OUTPUT:
            connector->Mode(ClearCore::Connector::OUTPUT_DIGITAL);
            break;
        case INPUT:
            connector->Mode(ClearCore::Connector::INPUT_DIGITAL);
            break;
        case INPUT_PULLUP:
            connector->Mode(ClearCore::Connector::INPUT_DIGITAL);
            break;
        default:
            break;
    }
}

CCSPI::CCSPI(SerialBase &thePort, bool isCom)
: m_serial(&thePort),
  m_isCom(isCom){
}

void CCSPI::begin(uint32_t clock) {
	m_clock = clock;
    m_serial->PortMode(SerialBase::PortModes::SPI);
	m_serial->SpiSsMode(SerialBase::CtrlLineModes::LINE_OFF);
	config();
	m_serial->PortOpen();
}

void CCSPI::config() {
	m_serial->Speed(m_clock);
	m_serial->SpiClock(SerialBase::SpiClockPolarities::SCK_LOW,
								SerialBase::SpiClockPhases::LEAD_SAMPLE);
}

void CCSPI::end() {
	m_serial->PortClose();
}

void CCSPI::usingInterrupt(int interruptNumber) {
	(void)interruptNumber;
}

void CCSPI::notUsingInterrupt(int interruptNumber) {
	(void)interruptNumber;
}

void CCSPI::beginTransaction() {
	config();
	m_serial->SpiSsMode(SerialBase::CtrlLineModes::LINE_ON);
}

void CCSPI::endTransaction(void) {
	m_serial->SpiSsMode(SerialBase::CtrlLineModes::LINE_OFF);
}

void CCSPI::setClockDivider(uint8_t div) {
	m_clock = MAX_SPI / div;
	config();
}

void CCSPI::SetClockSpeed(uint32_t clockSpeed){
    m_clock = clockSpeed;
	config();
}

uint8_t CCSPI::transfer(uint8_t data) {
	return m_serial->SpiTransferData(data);
}

uint16_t CCSPI::transfer16(uint16_t data) {
	union {
		uint16_t val;
		struct {
			uint8_t lsb;
			uint8_t msb;
		} bytes;
	} t;

	t.val = data;
    t.bytes.msb = transfer(t.bytes.msb);
	t.bytes.lsb = transfer(t.bytes.lsb);

	return t.val;
}

void CCSPI::transfer(void *buf, size_t count) {
	m_serial->SpiTransferData((uint8_t *)buf, (uint8_t *)buf, count);
}

void CCSPI::transfer(const void *txbuf, void *rxbuf, size_t count,
bool block) {
	if (!block &&
	m_serial->SpiTransferDataAsync((uint8_t *)txbuf, (uint8_t *)rxbuf, count)) {
		return;
	}
	m_serial->SpiTransferData((uint8_t *)txbuf, (uint8_t *)rxbuf, count);
}

// Waits for a prior in-background DMA transfer to complete.
void CCSPI::waitForTransfer(void) {
	m_serial->SpiAsyncWaitComplete();
}

void CCSPI::attachInterrupt() {
	// Should be enableInterrupt()
}

void CCSPI::detachInterrupt() {
	// Should be disableInterrupt()
}


CCSPI SPI(ClearCore::ConnectorCOM0, true);
CCSPI SPI1(ClearCore::ConnectorCOM1, true);
CCSPI SPI2(ClearCore::SdCard, false);

