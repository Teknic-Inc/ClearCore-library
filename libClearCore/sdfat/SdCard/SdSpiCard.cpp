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
//==============================================================================
// SdSpiCard member functions
//------------------------------------------------------------------------------
bool SdSpiCard::begin(uint32_t clockSpeed) {
    m_spiActive = false;
    m_errorCode = SD_CARD_ERROR_NONE;
    m_type = 0;
    uint16_t t0 = curTimeMS();
    uint32_t arg;
    //Set settings SDCard Serial Port
    SdCard.PortMode(SerialBase::PortModes::SPI);
    SdCard.SpiSsMode(SerialBase::CtrlLineModes::LINE_OFF);
    SdCard.Speed(clockSpeed);
    SdCard.SpiClock(SerialBase::SpiClockPolarities::SCK_LOW,
    SerialBase::SpiClockPhases::LEAD_SAMPLE);
    SdCard.PortOpen();
    spiStart();

    // must supply min of 74 clock cycles with CS high.
    for (uint8_t i = 0; i < 10; i++) {
        SdCard.SpiTransferData(0XFF);
    }

    // command to go idle in SPI mode
    for (uint8_t i = 1;; i++) {
        if (cardCommand(CMD0, 0) == R1_IDLE_STATE) {
            break;
        }
        if (i == SD_CMD0_RETRY) {
            error(SD_CARD_ERROR_CMD0);
            goto fail;
        }
        // stop multi-block write
        SdCard.SpiTransferData(STOP_TRAN_TOKEN);
        // finish block transfer
        for (int i = 0; i < 520; i++) {
            SdCard.SpiTransferData(0XFF);
        }
    }
    // check SD version
    if (cardCommand(CMD8, 0x1AA) == (R1_ILLEGAL_COMMAND | R1_IDLE_STATE)) {
        type(SD_CARD_TYPE_SD1);
    }
    else {
        for (uint8_t i = 0; i < 4; i++) {
            m_status = SdCard.SpiTransferData(0XFF);
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
    while (cardAcmd(ACMD41, arg) != R1_READY_STATE) {
        // check for timeout
        if (isTimedOut(t0, SD_INIT_TIMEOUT)) {
            error(SD_CARD_ERROR_ACMD41);
            goto fail;
        }
    }
    // if SD2 read OCR register to check for SDHC card
    if (type() == SD_CARD_TYPE_SD2) {
        if (cardCommand(CMD58, 0)) {
            error(SD_CARD_ERROR_CMD58);
            goto fail;
        }
        if ((SdCard.SpiTransferData(0XFF) & 0XC0) == 0XC0) {
            type(SD_CARD_TYPE_SDHC);
        }
        // Discard rest of ocr - contains allowed voltage range.
        for (uint8_t i = 0; i < 3; i++) {
            SdCard.SpiTransferData(0XFF);
        }
    }
    spiStop();
    return true;

fail:
    spiStop();
    //if initialization fails, send error code 1 to Clear Core
    SdCard.SetErrorCode(1);
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
        waitNotBusy(SD_CMD_TIMEOUT);
    }
    // send command
    SdCard.SpiTransferData(cmd | 0x40);

    // send argument
    uint8_t *pa = reinterpret_cast<uint8_t *>(&arg);
    for (int8_t i = 3; i >= 0; i--) {
        SdCard.SpiTransferData(pa[i]);

    }
    // send CRC - correct for CMD0 with arg zero or CMD8 with arg 0X1AA
    SdCard.SpiTransferData(cmd == CMD0 ? 0X95 : 0X87);

    // discard first fill byte to avoid MISO pull-up problem.
    SdCard.SpiTransferData(0XFF);

    // there are 1-8 fill bytes before response.  fill bytes should be 0XFF.
    for (uint8_t i = 0; ((m_status = SdCard.SpiTransferData(0XFF)) & 0X80) && i < 10; i++) {
    }
    return m_status;
}
//------------------------------------------------------------------------------
uint32_t SdSpiCard::cardCapacity() {
    csd_t csd;
    return readCSD(&csd) ? sdCardCapacity(&csd) : 0;
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
    if (!waitNotBusy(SD_ERASE_TIMEOUT)) {
        error(SD_CARD_ERROR_ERASE_TIMEOUT);
        goto fail;
    }
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
        if (0XFF == SdCard.SpiTransferData(0XFF)) {
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
void SdSpiCard::readBlocksASync(uint32_t block, uint8_t *dst, size_t count, uint16_t offset) {
    spiStart();
    if (type() != SD_CARD_TYPE_SDHC) {
        block <<= 9;
    }
    // transfer data
    SdCard.receiveBlocksASync(block, dst, count, offset);
    return;
}
//------------------------------------------------------------------------------
bool SdSpiCard::aSyncDataCheck() {
    if(SdCard.getSDBlockTransferComplete()){
        spiStop();
        return true;
    }
    return false;
}
//------------------------------------------------------------------------------
bool SdSpiCard::readData(uint8_t *dst) {
    return readData(dst, 512);
}
//------------------------------------------------------------------------------
bool SdSpiCard::readData(uint8_t *dst, size_t count) {
    // wait for start block token
    uint16_t t0 = curTimeMS();
    while ((m_status = SdCard.SpiTransferData(0XFF)) == 0XFF) {
        if (isTimedOut(t0, SD_READ_TIMEOUT)) {
            error(SD_CARD_ERROR_READ_TIMEOUT);
            goto fail;
        }
    }
    if (m_status != DATA_START_BLOCK) {
        error(SD_CARD_ERROR_READ);
        goto fail;
    }
    // transfer data
    for (size_t i = 0; i < count; i++) {
        dst[i] = 0xFF;
    }
    //blocking transfer is used by default
    if(!SdCard.SpiTransferData(dst, dst, count)){
        error(SD_CARD_ERROR_DMA);
        goto fail;
    }
    //set status to 0 for success
    m_status = 0;

    // discard crc
    SdCard.SpiTransferData(0XFF);
    SdCard.SpiTransferData(0XFF);
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
        p[3 - i] = SdCard.SpiTransferData(0XFF);
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
    return true;

fail:
    spiStop();
    return false;
}
//-----------------------------------------------------------------------------
bool SdSpiCard::readStatus(uint8_t *status) {
    // retrun is R2 so read extra status byte.
    if (cardAcmd(ACMD13, 0) || SdCard.SpiTransferData(0XFF)) {
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
        activate();
        m_spiActive = true;
    }
}
//-----------------------------------------------------------------------------
void SdSpiCard::spiStop() {
    if (m_spiActive) {
        deactivate();
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
    while (SdCard.SpiTransferData(0XFF) != 0XFF) {
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
    if (!waitNotBusy(SD_WRITE_TIMEOUT)) {
        error(SD_CARD_ERROR_WRITE_TIMEOUT);
        goto fail;
    }
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
    SdCard.SpiTransferData(token);
    SdCard.SpiTransferData((uint8_t *)src, (uint8_t *)NULL, 512);
    SdCard.SpiTransferData(crc >> 8);
    SdCard.SpiTransferData(crc & 0XFF);

    m_status = SdCard.SpiTransferData(0XFF);
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
    if (!waitNotBusy(SD_WRITE_TIMEOUT)) {
        goto fail;
    }
    SdCard.SpiTransferData(STOP_TRAN_TOKEN);
    spiStop();
    return true;

fail:
    error(SD_CARD_ERROR_STOP_TRAN);
    spiStop();
    return false;
}
//------------------------------------------------------------------------------
    void SdSpiCard::activate() {
        SdCard.SpiClock(SerialBase::SpiClockPolarities::SCK_LOW,
        SerialBase::SpiClockPhases::LEAD_SAMPLE);
        SdCard.SpiSsMode(SerialBase::CtrlLineModes::LINE_ON);
    }
//------------------------------------------------------------------------------
    void SdSpiCard::deactivate() {
        SdCard.SpiSsMode(SerialBase::CtrlLineModes::LINE_OFF);
    }
//------------------------------------------------------------------------------
    void SdSpiCard::SetSDErrorCode(uint16_t errorCode){
        SdCard.SetErrorCode(errorCode);
    }
//------------------------------------------------------------------------------
