/**
 * Copyright (c) 2011-2018 Bill Greiman
 * This file is part of the SdFat library for SD memory cards.
 *
 * MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */
#include "SdSpiCard.h"
#include "SdCardDriver.h"

 namespace ClearCore {
     extern SdCardDriver SdCard;
 }
//==============================================================================
// debug trace macro
#define SD_TRACE(m, b)
// #define SD_TRACE(m, b) Serial.print(m);Serial.println(b);
#define SD_CS_DBG(m)
// #define SD_CS_DBG(m) Serial.println(F(m));

#define DBG_PROFILE_STATS 0
#if DBG_PROFILE_STATS

#define DBG_TAG_LIST\
  DBG_TAG(DBG_CMD0_TIME, "CMD0 time")\
  DBG_TAG(DBG_ACMD41_TIME, "ACMD41 time")\
  DBG_TAG(DBG_CMD_BUSY, "cmd busy")\
  DBG_TAG(DBG_ERASE_BUSY, "erase busy")\
  DBG_TAG(DBG_WAIT_READ, "wait read")\
  DBG_TAG(DBG_WRITE_FLASH, "write flash")\
  DBG_TAG(DBG_WRITE_BUSY, "write busy")\
  DBG_TAG(DBG_WRITE_STOP, "write stop")\
  DBG_TAG(DBG_ACMD41_COUNT, "ACMD41 count")\
  DBG_TAG(DBG_CMD0_COUNT, "CMD0 count")

#define DBG_TIME_DIM DBG_ACMD41_COUNT

enum DbgTag {
#define DBG_TAG(tag, str) tag,
    DBG_TAG_LIST
    DBG_COUNT_DIM
#undef DBG_TAG
};

static uint32_t dbgCount[DBG_COUNT_DIM];
static uint32_t dbgBgnTime[DBG_TIME_DIM];
static uint32_t dbgMaxTime[DBG_TIME_DIM];
static uint32_t dbgMinTime[DBG_TIME_DIM];
static uint32_t dbgTotalTime[DBG_TIME_DIM];
//------------------------------------------------------------------------------
static void dbgBeginTime(DbgTag tag) {
    dbgBgnTime[tag] = micros();
}
//------------------------------------------------------------------------------
static void dbgClearStats() {
    for (int i = 0; i < DBG_COUNT_DIM; i++) {
        dbgCount[i] = 0;
        if (i < DBG_TIME_DIM) {
            dbgMaxTime[i] = 0;
            dbgMinTime[i] = 9999999;
            dbgTotalTime[i] = 0;
        }
    }
}
//------------------------------------------------------------------------------
static void dbgEndTime(DbgTag tag) {
    uint32_t m = micros() - dbgBgnTime[tag];
    dbgTotalTime[tag] += m;
    if (m > dbgMaxTime[tag]) {
        dbgMaxTime[tag] = m;
    }
    if (m < dbgMinTime[tag]) {
        dbgMinTime[tag] = m;
    }
    dbgCount[tag]++;
}
//------------------------------------------------------------------------------
static void dbgEventCount(DbgTag tag) {
    dbgCount[tag]++;
}
//------------------------------------------------------------------------------
static void dbgPrintTagText(uint8_t tag) {
#define DBG_TAG(e, m) case e: Serial.print(F(m)); break;
    switch (tag) {
            DBG_TAG_LIST
    }
#undef DBG_TAG
}
//------------------------------------------------------------------------------
static void dbgPrintStats() {
    Serial.println();
    Serial.println(F("======================="));
    Serial.println(F("item,event,min,max,avg"));
    Serial.println(F("tag,count,usec,usec,usec"));
    for (int i = 0; i < DBG_COUNT_DIM; i++) {
        if (dbgCount[i]) {
            dbgPrintTagText(i);
            Serial.print(',');
            Serial.print(dbgCount[i]);
            if (i < DBG_TIME_DIM) {
                Serial.print(',');
                Serial.print(dbgMinTime[i]);
                Serial.print(',');
                Serial.print(dbgMaxTime[i]);
                Serial.print(',');
                Serial.print(dbgTotalTime[i] / dbgCount[i]);
            }
            Serial.println();
        }
    }
    Serial.println(F("======================="));
    Serial.println();
}
#undef DBG_TAG_LIST
#define DBG_BEGIN_TIME(tag) dbgBeginTime(tag)
#define DBG_END_TIME(tag) dbgEndTime(tag)
#define DBG_EVENT_COUNT(tag) dbgEventCount(tag)
#else  // DBG_PROFILE_STATS
#define DBG_BEGIN_TIME(tag)
#define DBG_END_TIME(tag)
#define DBG_EVENT_COUNT(tag)
static void dbgClearStats() {}
static void dbgPrintStats() {}
#endif  // DBG_PROFILE_STATS
//==============================================================================
// SdSpiCard member functions
//------------------------------------------------------------------------------
bool SdSpiCard::begin(SdSpiDriver *spi, uint8_t csPin, uint32_t clockSpeed) {
    m_spiActive = false;
    m_errorCode = SD_CARD_ERROR_NONE;
    m_type = 0;
    m_spiDriver = spi;
    uint16_t t0 = curTimeMS();
    uint32_t arg;
    m_spiDriver->begin(csPin, clockSpeed);
    spiStart();

    // must supply min of 74 clock cycles with CS high.
    spiUnselect();
    for (uint8_t i = 0; i < 10; i++) {
        spiSend(0XFF);
    }
    spiSelect();

    DBG_BEGIN_TIME(DBG_CMD0_TIME);
    // command to go idle in SPI mode
    for (uint8_t i = 1;; i++) {
        DBG_EVENT_COUNT(DBG_CMD0_COUNT);
        if (cardCommand(CMD0, 0) == R1_IDLE_STATE) {
            break;
        }
        if (i == SD_CMD0_RETRY) {
            error(SD_CARD_ERROR_CMD0);
            goto fail;
        }
        // stop multi-block write
        spiSend(STOP_TRAN_TOKEN);
        // finish block transfer
        for (int i = 0; i < 520; i++) {
            SdCard.SpiTransferData(0XFF);
        }
    }
    DBG_END_TIME(DBG_CMD0_TIME);
    // check SD version
    if (cardCommand(CMD8, 0x1AA) == (R1_ILLEGAL_COMMAND | R1_IDLE_STATE)) {
        type(SD_CARD_TYPE_SD1);
    }
    else {
        for (uint8_t i = 0; i < 4; i++) {
            m_status = spiReceive();
        }
        if (m_status == 0XAA) {
            type(SD_CARD_TYPE_SD2);
        }
        else {
            error(SD_CARD_ERROR_CMD8);
            goto fail;
        }
    }
    // initialize card and send host supports SDHC if SD2
    arg = type() == SD_CARD_TYPE_SD2 ? 0X40000000 : 0;
    DBG_BEGIN_TIME(DBG_ACMD41_TIME);
    while (cardAcmd(ACMD41, arg) != R1_READY_STATE) {
        DBG_EVENT_COUNT(DBG_ACMD41_COUNT);
        // check for timeout
        if (isTimedOut(t0, SD_INIT_TIMEOUT)) {
            error(SD_CARD_ERROR_ACMD41);
            goto fail;
        }
    }
    DBG_END_TIME(DBG_ACMD41_TIME);
    // if SD2 read OCR register to check for SDHC card
    if (type() == SD_CARD_TYPE_SD2) {
        if (cardCommand(CMD58, 0)) {
            error(SD_CARD_ERROR_CMD58);
            goto fail;
        }
        if ((spiReceive() & 0XC0) == 0XC0) {
            type(SD_CARD_TYPE_SDHC);
        }
        // Discard rest of ocr - contains allowed voltage range.
        for (uint8_t i = 0; i < 3; i++) {
            spiReceive();
        }
    }
    spiStop();
    //if initialization succeeds, make sure error code is clear
    spiSetSDErrorCode(0);
    return true;

fail:
    spiStop();
    //if initialization fails, send error code 1 to Clear Core
    spiSetSDErrorCode(1);
    return false;
}
//------------------------------------------------------------------------------
// send command and return error code.  Return zero for OK
uint8_t SdSpiCard::cardCommand(uint8_t cmd, uint32_t arg) {
    // select card
    if (!m_spiActive) {
        spiStart();
    }
    // wait if busy unless CMD0
    if (cmd != CMD0) {
        DBG_BEGIN_TIME(DBG_CMD_BUSY);
        waitNotBusy(SD_CMD_TIMEOUT);
        DBG_END_TIME(DBG_CMD_BUSY);
    }
    // send command
    spiSend(cmd | 0x40);

    // send argument
    uint8_t *pa = reinterpret_cast<uint8_t *>(&arg);
    for (int8_t i = 3; i >= 0; i--) {
        spiSend(pa[i]);
    }
    // send CRC - correct for CMD0 with arg zero or CMD8 with arg 0X1AA
    spiSend(cmd == CMD0 ? 0X95 : 0X87);

    // discard first fill byte to avoid MISO pull-up problem.
    spiReceive();

    // there are 1-8 fill bytes before response.  fill bytes should be 0XFF.
    for (uint8_t i = 0; ((m_status = spiReceive()) & 0X80) && i < 10; i++) {
    }
    return m_status;
}
//------------------------------------------------------------------------------
// send command and return zero for OK.
uint8_t SdSpiCard::cardCommandASync(uint8_t cmd, uint32_t arg) {
    // select card
    if (!m_spiActive) {
        spiStart();
    }
    // send command
    spiSend(cmd | 0x40);

    // send argument
    uint8_t *pa = reinterpret_cast<uint8_t *>(&arg);
    for (int8_t i = 3; i >= 0; i--) {
        spiSend(pa[i]);
    }
    // send CRC - correct for CMD0 with arg zero or CMD8 with arg 0X1AA
    spiSend(cmd == CMD0 ? 0X95 : 0X87);

    // discard first fill byte to avoid MISO pull-up problem.
    spiReceive();

    // there are 1-8 fill bytes before response.  fill bytes should be 0XFF.
    for (uint8_t i = 0; i < 10; i++) {
        spiReceive();
    }
    return 0;
}
//------------------------------------------------------------------------------
uint32_t SdSpiCard::cardCapacity() {
    csd_t csd;
    return readCSD(&csd) ? sdCardCapacity(&csd) : 0;
}
//------------------------------------------------------------------------------
void SdSpiCard::dbgClearStats() {
    ::dbgClearStats();
}
//------------------------------------------------------------------------------
void SdSpiCard::dbgPrintStats() {
    ::dbgPrintStats();
}
//------------------------------------------------------------------------------
bool SdSpiCard::erase(uint32_t firstBlock, uint32_t lastBlock) {
    csd_t csd;
    if (!readCSD(&csd)) {
        goto fail;
    }
    // check for single block erase
    if (!csd.v1.erase_blk_en) {
        // erase size mask
        uint8_t m = (csd.v1.sector_size_high << 1) | csd.v1.sector_size_low;
        if ((firstBlock & m) != 0 || ((lastBlock + 1) & m) != 0) {
            // error card can't erase specified area
            error(SD_CARD_ERROR_ERASE_SINGLE_BLOCK);
            goto fail;
        }
    }
    if (m_type != SD_CARD_TYPE_SDHC) {
        firstBlock <<= 9;
        lastBlock <<= 9;
    }
    if (cardCommand(CMD32, firstBlock)
            || cardCommand(CMD33, lastBlock)
            || cardCommand(CMD38, 0)) {
        error(SD_CARD_ERROR_ERASE);
        goto fail;
    }
    DBG_BEGIN_TIME(DBG_ERASE_BUSY);
    if (!waitNotBusy(SD_ERASE_TIMEOUT)) {
        error(SD_CARD_ERROR_ERASE_TIMEOUT);
        goto fail;
    }
    DBG_END_TIME(DBG_ERASE_BUSY);
    spiStop();
    return true;

fail:
    spiStop();
    return false;
}
//------------------------------------------------------------------------------
bool SdSpiCard::eraseSingleBlockEnable() {
    csd_t csd;
    return readCSD(&csd) ? csd.v1.erase_blk_en : false;
}
//------------------------------------------------------------------------------
bool SdSpiCard::isBusy() {
    bool rtn = true;
    bool spiActive = m_spiActive;
    if (!spiActive) {
        spiStart();
    }
    for (uint8_t i = 0; i < 8; i++) {
        if (0XFF == spiReceive()) {
            rtn = false;
            break;
        }
    }
    if (!spiActive) {
        spiStop();
    }
    return rtn;
}
//------------------------------------------------------------------------------
bool SdSpiCard::isTimedOut(uint16_t startMS, uint16_t timeoutMS) {
    return (curTimeMS() - startMS) > timeoutMS;
}
//------------------------------------------------------------------------------
bool SdSpiCard::readBlock(uint32_t blockNumber, uint8_t *dst) {
    SD_TRACE("RB", blockNumber);
    // use address if not SDHC card
    if (type() != SD_CARD_TYPE_SDHC) {
        blockNumber <<= 9;
    }
    if (cardCommand(CMD17, blockNumber)) {
        error(SD_CARD_ERROR_CMD17);
        goto fail;
    }
    if (!readData(dst, 512)) {
        goto fail;
    }
    spiStop();
    return true;

fail:
    spiStop();
    return false;
}
//------------------------------------------------------------------------------
bool SdSpiCard::readBlockASync(uint32_t blockNumber, uint8_t *dst) {
    SD_TRACE("RB", blockNumber);
    // use address if not SDHC card
    if (type() != SD_CARD_TYPE_SDHC) {
        blockNumber <<= 9;
    }
    if (cardCommand(CMD17, blockNumber)) {
        error(SD_CARD_ERROR_CMD17);
        goto fail;
    }
    if (!readData(dst, 512)) {
        goto fail;
    }
    spiStop();
    return true;

    fail:
    spiStop();
    return false;
}
//------------------------------------------------------------------------------
bool SdSpiCard::readBlocks(uint32_t block, uint8_t *dst, size_t count) {
    if (!readStart(block)) {
        return false;
    }
    for (uint16_t b = 0; b < count; b++, dst += 512) {
        if (!readData(dst, 512)) {
            return false;
        }
    }

    return readStop();
}
//------------------------------------------------------------------------------
bool SdSpiCard::readBlocksASync(uint32_t block, uint8_t *dst, size_t count) {
    if (!readStart(block)) {
        return false;
    }
    DBG_BEGIN_TIME(DBG_WAIT_READ);
    // wait for start block token
    uint16_t t0 = curTimeMS();
    while ((m_status = spiReceive()) == 0XFF) {
        if (isTimedOut(t0, SD_READ_TIMEOUT)) {
           error(SD_CARD_ERROR_READ_TIMEOUT);
           goto fail;
        }
    }
    DBG_END_TIME(DBG_WAIT_READ);
    if (m_status != DATA_START_BLOCK) {
        error(SD_CARD_ERROR_READ);
        goto fail;
    }
    // transfer data
    SdCard.sendBlockASync(dst,count);

    while(!SdCard.getSDBlockTransferComplete()){
        Delay_ms(1);
        continue;
    }


    return readStop();

fail:
    spiStop();
    return false;
}
//------------------------------------------------------------------------------
bool SdSpiCard::readData(uint8_t *dst) {
    return readData(dst, 512);
}
//------------------------------------------------------------------------------
bool SdSpiCard::readData(uint8_t *dst, size_t count) {
    DBG_BEGIN_TIME(DBG_WAIT_READ);
    // wait for start block token
    uint16_t t0 = curTimeMS();
    while ((m_status = spiReceive()) == 0XFF) {
        if (isTimedOut(t0, SD_READ_TIMEOUT)) {
            error(SD_CARD_ERROR_READ_TIMEOUT);
            goto fail;
        }
    }
    DBG_END_TIME(DBG_WAIT_READ);
    if (m_status != DATA_START_BLOCK) {
        error(SD_CARD_ERROR_READ);
        goto fail;
    }
    // transfer data
    if ((m_status = spiReceive(dst, count))) {
        error(SD_CARD_ERROR_DMA);
        goto fail;
    }

    // discard crc
    spiReceive();
    spiReceive();
    return true;

fail:
    spiStop();
    return false;
}
//------------------------------------------------------------------------------
bool SdSpiCard::readOCR(uint32_t *ocr) {
    uint8_t *p = reinterpret_cast<uint8_t *>(ocr);
    if (cardCommand(CMD58, 0)) {
        error(SD_CARD_ERROR_CMD58);
        goto fail;
    }
    for (uint8_t i = 0; i < 4; i++) {
        p[3 - i] = spiReceive();
    }

    spiStop();
    return true;

fail:
    spiStop();
    return false;
}
//------------------------------------------------------------------------------
/** read CID or CSR register */
bool SdSpiCard::readRegister(uint8_t cmd, void *buf) {
    uint8_t *dst = reinterpret_cast<uint8_t *>(buf);
    if (cardCommand(cmd, 0)) {
        error(SD_CARD_ERROR_READ_REG);
        goto fail;
    }
    if (!readData(dst, 16)) {
        goto fail;
    }
    spiStop();
    return true;

fail:
    spiStop();
    return false;
}
//------------------------------------------------------------------------------
bool SdSpiCard::readStart(uint32_t blockNumber) {
    SD_TRACE("RS", blockNumber);
    if (type() != SD_CARD_TYPE_SDHC) {
        blockNumber <<= 9;
    }
    if (cardCommand(CMD18, blockNumber)) {
        error(SD_CARD_ERROR_CMD18);
        goto fail;
    }
//  spiStop();
    return true;

fail:
    spiStop();
    return false;
}
//-----------------------------------------------------------------------------
bool SdSpiCard::readStatus(uint8_t *status) {
    // retrun is R2 so read extra status byte.
    if (cardAcmd(ACMD13, 0) || spiReceive()) {
        error(SD_CARD_ERROR_ACMD13);
        goto fail;
    }
    if (!readData(status, 64)) {
        goto fail;
    }
    spiStop();
    return true;

fail:
    spiStop();
    return false;
}
//-----------------------------------------------------------------------------
void SdSpiCard::spiStart() {
    if (!m_spiActive) {
        spiActivate();
        spiSelect();
        m_spiActive = true;
    }
}
//-----------------------------------------------------------------------------
void SdSpiCard::spiStop() {
    if (m_spiActive) {
        spiUnselect();
        spiSend(0XFF);
        spiDeactivate();
        m_spiActive = false;
    }
}
//------------------------------------------------------------------------------
bool SdSpiCard::readStop() {
    if (cardCommand(CMD12, 0)) {
        error(SD_CARD_ERROR_CMD12);
        goto fail;
    }
    spiStop();
    return true;

fail:
    spiStop();
    return false;
}
//------------------------------------------------------------------------------
// wait for card to go not busy
bool SdSpiCard::waitNotBusy(uint16_t timeoutMS) {
    uint16_t t0 = curTimeMS();
    // Check not busy first since yield is not called in isTimedOut.
    while (spiReceive() != 0XFF) {
        if (isTimedOut(t0, timeoutMS)) {
            return false;
        }
    }
    return true;
}
//------------------------------------------------------------------------------
bool SdSpiCard::writeBlock(uint32_t blockNumber, const uint8_t *src) {
    SD_TRACE("WB", blockNumber);
    // use address if not SDHC card
    if (type() != SD_CARD_TYPE_SDHC) {
        blockNumber <<= 9;
    }
    if (cardCommand(CMD24, blockNumber)) {
        error(SD_CARD_ERROR_CMD24);
        goto fail;
    }
    if (!writeData(DATA_START_BLOCK, src)) {
        goto fail;
    }


// #if CHECK_FLASH_PROGRAMMING
//     // wait for flash programming to complete
//     DBG_BEGIN_TIME(DBG_WRITE_FLASH);
//     if (!waitNotBusy(SD_WRITE_TIMEOUT)) {
//         error(SD_CARD_ERROR_FLASH_PROGRAMMING);
//         goto fail;
//     }
//     DBG_END_TIME(DBG_WRITE_FLASH);
//     // response is r2 so get and check two bytes for nonzero
//     if (cardCommand(CMD13, 0) || spiReceive()) {
//         error(SD_CARD_ERROR_CMD13);
//         goto fail;
//     }
// #endif  // CHECK_PROGRAMMING

    spiStop();
    return true;

fail:
    spiStop();
    return false;
}
//------------------------------------------------------------------------------
bool SdSpiCard::writeBlocks(uint32_t block, const uint8_t *src, size_t count) {
    if (!writeStart(block)) {
        goto fail;
    }
    for (size_t b = 0; b < count; b++, src += 512) {
        if (!writeData(src)) {
            goto fail;
        }
    }
    return writeStop();

fail:
    spiStop();
    return false;
}
//------------------------------------------------------------------------------
bool SdSpiCard::writeData(const uint8_t *src) {
    // wait for previous write to finish
    DBG_BEGIN_TIME(DBG_WRITE_BUSY);
    if (!waitNotBusy(SD_WRITE_TIMEOUT)) {
        error(SD_CARD_ERROR_WRITE_TIMEOUT);
        goto fail;
    }
    DBG_END_TIME(DBG_WRITE_BUSY);
    if (!writeData(WRITE_MULTIPLE_TOKEN, src)) {
        goto fail;
    }
    return true;

fail:
    spiStop();
    return false;
}
//------------------------------------------------------------------------------
// send one block of data for write block or write multiple blocks
bool SdSpiCard::writeData(uint8_t token, const uint8_t *src) {
    uint16_t crc = 0XFFFF;
    spiSend(token);
    spiSend(src, 512);
    spiSend(crc >> 8);
    spiSend(crc & 0XFF);

    m_status = spiReceive();
    if ((m_status & DATA_RES_MASK) != DATA_RES_ACCEPTED) {
        error(SD_CARD_ERROR_WRITE);
        goto fail;
    }
    return true;

fail:
    spiStop();
    return false;
}
//------------------------------------------------------------------------------
bool SdSpiCard::writeStart(uint32_t blockNumber) {
    // use address if not SDHC card
    if (type() != SD_CARD_TYPE_SDHC) {
        blockNumber <<= 9;
    }
    if (cardCommand(CMD25, blockNumber)) {
        error(SD_CARD_ERROR_CMD25);
        goto fail;
    }
    return true;

fail:
    spiStop();
    return false;
}
//------------------------------------------------------------------------------
bool SdSpiCard::writeStart(uint32_t blockNumber, uint32_t eraseCount) {
    SD_TRACE("WS", blockNumber);
    // send pre-erase count
    if (cardAcmd(ACMD23, eraseCount)) {
        error(SD_CARD_ERROR_ACMD23);
        goto fail;
    }
    // use address if not SDHC card
    if (type() != SD_CARD_TYPE_SDHC) {
        blockNumber <<= 9;
    }
    if (cardCommand(CMD25, blockNumber)) {
        error(SD_CARD_ERROR_CMD25);
        goto fail;
    }
    return true;

fail:
    spiStop();
    return false;
}
//------------------------------------------------------------------------------
bool SdSpiCard::writeStop() {
    DBG_BEGIN_TIME(DBG_WRITE_STOP);
    if (!waitNotBusy(SD_WRITE_TIMEOUT)) {
        goto fail;
    }
    DBG_END_TIME(DBG_WRITE_STOP);
    spiSend(STOP_TRAN_TOKEN);
    spiStop();
    return true;

fail:
    error(SD_CARD_ERROR_STOP_TRAN);
    spiStop();
    return false;
}
