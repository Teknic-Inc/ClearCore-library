/*
 * sdRef.h
 *
 * Created: 10/6/2020 1:45:57 PM
 *  Author: allen_wells
 */ 


#ifndef CLEARCOREREF_H_
#define CLEARCOREREF_H_

#include "ClearCore.h"
#include "SerialBase.h"

#define SDCARD_SPI SPI2

#define MAX_SPI 10000000
#define SPI_MIN_CLOCK_DIVIDER 1

typedef uint8_t pin_size_t;


typedef enum {
	LOW = 0,
	HIGH = 1,
	CHANGE = 2,
	FALLING = 3,
	RISING = 4,
} PinStatus;

typedef enum {
	INPUT = 0x0,
	OUTPUT = 0x1,
	INPUT_PULLUP = 0x2,
} PinMode;

void digitalWriteClearCore(pin_size_t conNum, PinStatus ulVal);

// Parallel version of pinMode to setup ClearCore "connectors" using the
// connector index.
void pinModeClearCore(pin_size_t pinNumber, uint32_t ulMode);


class CCSPI {
	public:
	CCSPI(ClearCore::SerialBase &thePort, bool isCom);

	uint8_t transfer(uint8_t data);
	uint16_t transfer16(uint16_t data);
	void transfer(void *buf, size_t count);
	void transfer(const void *txbuf, void *rxbuf, size_t count,
	bool block = true);
	void waitForTransfer(void);

	// Transaction Functions
	void usingInterrupt(int interruptNumber);
	void notUsingInterrupt(int interruptNumber);
	void beginTransaction();
	void endTransaction(void);

	// SPI Configuration methods
	void attachInterrupt();
	void detachInterrupt();

	void begin(uint32_t clock);
	void end();

	void setDataMode(uint8_t uc_mode);
	void setClockDivider(uint8_t uc_div);
	void SetClockSpeed(uint32_t clockSpeed);

	private:
	void config();

	ClearCore::SerialBase *m_serial;
	bool m_isCom;
	uint32_t m_clock;

};

extern CCSPI SPI;
extern CCSPI SPI1;
extern CCSPI SPI2;



#endif /* SDREF_H_ */