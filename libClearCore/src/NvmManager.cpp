/*
 * Copyright (c) 2020 Teknic, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/**
    NVM Peripheral Manager for the ClearCore Board

    This class manages the the non-volatile EEPROM peripheral on the
    Teknic ClearCore.
**/

#include "NvmManager.h"
#include "AdcManager.h"
#include "StatusManager.h"
#include "atomic_utils.h"
#include <cstring>
#include <sam.h>

#define NVM_BLOCK_WRITE_V (20)
#define UNDER_VOLTAGE_TRIP_CNT ((uint16_t)(NVM_BLOCK_WRITE_V * (1 << 15) / \
    AdcManager::ADC_CHANNEL_MAX_FLOAT[AdcManager::ADC_VSUPPLY_MON]))

namespace ClearCore {


// Execute a command.
// Wait until the NVM controller is ready to accept a new command.
// The given cmd must be OR'd with NVMCTRL_CTRLB_CMDEX_KEY into the CTRLB
// register for the cmd to be executed.
#define EXEC_CMD(cmd)                                                          \
    while (NVMCTRL->STATUS.bit.READY == 0);                                    \
    NVMCTRL->CTRLB.reg = NVMCTRL_CTRLB_CMDEX_KEY | (cmd);

// According to Section 9.4 NVM User Page Mapping in the datasheet, the first
// 32 bytes of the User Page contain calibration data that are automatically
// read at device power-on. Those bytes will get read to and written from the
// page cache, but we won't allow anyone to access them; anytime someone
// accesses the NVM, we'll adjust the given address to account for these
// reserved bytes.
#define NVM_LOCATION_TO_INDEX(loc) ((loc) + 32)
#define DEFAULT_MAC_ADDRESS 0x241510b00000

NvmManager &NvmMgr = NvmManager::Instance();
uint32_t NvmMgrUnlock;

NvmManager &NvmManager::Instance() {
    static NvmManager *instance = new NvmManager();
    return *instance;
}

NvmManager::NvmManager()
    : m_nvmPageCache32(reinterpret_cast<int32_t *>(m_nvmPageCache)),
      m_writeState(IDLE),
      m_quadWordIndex(0),
      m_pageModified(false) {

    PopulateCache();
}

/**
    Read octet from NVM.
**/
int8_t NvmManager::Byte(NvmLocations nvmLocation) {

    // Check bounds - upper
    if (nvmLocation >= NvmLocations::NVM_LOC_USER_MAX) {
        return -1;
    }

    int8_t returnValue = m_nvmPageCache[NVM_LOCATION_TO_INDEX(nvmLocation)];
    return returnValue;
}
/**
    Write octet to NVM.
**/
bool NvmManager::Byte(NvmLocations nvmLocation, int8_t newValue) {

    // Check bounds - upper
    if (nvmLocation >= NvmLocations::NVM_LOC_USER_MAX) {
        return false;
    }

    // Check bounds - if trying to write to Teknic reserved space, make
    // sure the unlock code is set first
    if (nvmLocation >= NvmLocations::NVM_LOC_RESERVED_TEKNIC) {
        // If trying to write into the Teknic reserved space, return if the
        // unlock code is not set
        if (NvmMgrUnlock != 0x3fadeb) {
            return false;
        }
    }

    // Check if an update is actually needed.
    if (Byte(nvmLocation) == newValue) {
        return true;
    }

    // Put the new desired value into the page cache.
    m_nvmPageCache[NVM_LOCATION_TO_INDEX(nvmLocation)] = newValue;

    return WriteCacheToNvm();
}

/**
    Read 16-bit integer from NVM.
**/
int16_t NvmManager::Int16(NvmLocations nvmLocation) {

    // Check bounds - upper
    if (nvmLocation >=
            (NvmLocations::NVM_LOC_USER_MAX - sizeof(uint16_t) + 1)) {
        return -1;
    }

    int16_t returnValue;
    // Get a pointer to the value to be read
    int8_t *byteAddress =
        &(m_nvmPageCache[NVM_LOCATION_TO_INDEX(nvmLocation)]);
    // Access it as a 16-bit array
    int16_t *address = reinterpret_cast<int16_t *>(byteAddress);
    returnValue = address[0];

    return returnValue;
}
/**
    Write 16-bit integer to NVM.
**/
bool NvmManager::Int16(NvmLocations nvmLocation, int16_t newValue) {

    // Check bounds - upper
    if (nvmLocation >=
            (NvmLocations::NVM_LOC_USER_MAX - sizeof(uint16_t) + 1)) {
        return false;
    }

    // Check bounds - if trying to write to Teknic reserved space, make
    // sure the unlock code is set first
    if (nvmLocation >=
            (NvmLocations::NVM_LOC_RESERVED_TEKNIC - sizeof(uint16_t) + 1)) {
        // If trying to write into the Teknic reserved space, return if the
        // unlock code is not set
        if (NvmMgrUnlock != 0x3fadeb) {
            return false;
        }
    }

    // Check if an update is actually needed.
    if (Int16(nvmLocation) == newValue) {
        return true;
    }

    // Get a pointer to the value to be written
    int8_t *byteAddress =
        &(m_nvmPageCache[NVM_LOCATION_TO_INDEX(nvmLocation)]);
    // Access it as a 16-bit array
    int16_t *address = reinterpret_cast<int16_t *>(byteAddress);

    // Put the new desired value into the page cache.
    address[0] = newValue;

    return WriteCacheToNvm();
}

/**
    Read 32-bit integer from NVM.
**/
int32_t NvmManager::Int32(NvmLocations nvmLocation) {

    // Check bounds - upper
    if (nvmLocation >=
            (NvmLocations::NVM_LOC_USER_MAX - sizeof(uint32_t) + 1)) {
        return -1;
    }

    int32_t returnValue;
    // Get a pointer to the value to be read
    int8_t *byteAddress =
        &(m_nvmPageCache[NVM_LOCATION_TO_INDEX(nvmLocation)]);
    // Access it as a 32-bit array
    int32_t *address = reinterpret_cast<int32_t *>(byteAddress);
    returnValue = address[0];

    return returnValue;
}

/**
    Write 32-bit integer to NVM.
**/
bool NvmManager::Int32(NvmLocations nvmLocation, int32_t newValue) {

    // Check bounds - upper
    if (nvmLocation >=
            (NvmLocations::NVM_LOC_USER_MAX - sizeof(uint32_t) + 1)) {
        return false;
    }

    // Check bounds - if trying to write to Teknic reserved space, make
    // sure the unlock code is set first
    if (nvmLocation >=
            (NvmLocations::NVM_LOC_RESERVED_TEKNIC - sizeof(uint32_t) + 1)) {
        // If trying to write into the Teknic reserved space, return if the
        // unlock code is not set
        if (NvmMgrUnlock != 0x3fadeb) {
            return false;
        }
    }

    // Check if an update is actually needed.
    if (Int32(nvmLocation) == newValue) {
        return true;
    }

    // Get a pointer to the value to be written
    int8_t *byteAddress =
        &(m_nvmPageCache[NVM_LOCATION_TO_INDEX(nvmLocation)]);
    // Access it as a 32-bit array
    int32_t *address = reinterpret_cast<int32_t *>(byteAddress);

    // Put the new desired value into the page cache.
    address[0] = newValue;

    return WriteCacheToNvm();
}

/**
    Read 64-bit integer from NVM.
**/
int64_t NvmManager::Int64(NvmLocations nvmLocationStart) {

    // Check bounds - upper
    if (nvmLocationStart >=
            (NvmLocations::NVM_LOC_USER_MAX - sizeof(uint64_t) + 1)) {
        return -1;
    }

    // 64-bit reads don't work if they aren't aligned; read it as 2 32-bit
    // values instead
    uint32_t val1 = Int32(nvmLocationStart);
    uint32_t val2 =
        Int32(static_cast<NvmLocations>(nvmLocationStart + sizeof(int32_t)));
    uint64_t returnValue = val1;
    returnValue = (returnValue << 32) | val2;
    return returnValue;
}

/**
    Write 64-bit integer to NVM.
**/
bool NvmManager::Int64(NvmLocations nvmLocationStart, int64_t newValue) {

    // Check bounds - upper
    if (nvmLocationStart >=
            (NvmLocations::NVM_LOC_USER_MAX - sizeof(uint64_t) + 1)) {
        return false;
    }

    // Check bounds - if trying to write to Teknic reserved space, make
    // sure the unlock code is set first
    if (nvmLocationStart >=
            (NvmLocations::NVM_LOC_RESERVED_TEKNIC - sizeof(uint64_t) + 1)) {
        // If trying to write into the Teknic reserved space, return if the
        // unlock code is not set
        if (NvmMgrUnlock != 0x3fadeb) {
            return false;
        }
    }

    // Check if an update is actually needed.
    if (Int64(nvmLocationStart) == newValue) {
        return true;
    }

    int32_t upper = newValue >> 32;
    int32_t lower = newValue;

    // Get a pointer to the value to be written
    int8_t *byteAddress =
        &(m_nvmPageCache[NVM_LOCATION_TO_INDEX(nvmLocationStart)]);
    // Access it as a 32-bit array
    int32_t *address = reinterpret_cast<int32_t *>(byteAddress);

    // Put the new desired value into the page cache.
    address[0] = upper;
    address[1] = lower;

    return WriteCacheToNvm();
}

/**
    BlockRead from NVM.
**/
void NvmManager::BlockRead(NvmLocations nvmLocationStart, int lengthInBytes, uint8_t * const p_data)
{
    // Check bounds - upper
    if (nvmLocationStart >=
    (NvmLocations::NVM_LOC_USER_MAX - lengthInBytes + 1)) {
        return;
    }

    memcpy(p_data, &(m_nvmPageCache[NVM_LOCATION_TO_INDEX(nvmLocationStart)]), lengthInBytes);
}

/**
    BlockWrite to NVM.
**/
bool NvmManager::BlockWrite(NvmLocations nvmLocationStart, int lengthInBytes, uint8_t const * const p_data)
{
    // Check bounds - upper
    if (nvmLocationStart >=
    (NvmLocations::NVM_LOC_USER_MAX - lengthInBytes + 1)) {
        return false;
    }

    // Check bounds - if trying to write to Teknic reserved space, make
    // sure the unlock code is set first
    if (nvmLocationStart >=
    (NvmLocations::NVM_LOC_RESERVED_TEKNIC - lengthInBytes + 1)) {
        // If trying to write into the Teknic reserved space, return if the
        // unlock code is not set
        if (NvmMgrUnlock != 0x3fadeb) {
            return false;
        }
    }

    if(memcmp(&(m_nvmPageCache[NVM_LOCATION_TO_INDEX(nvmLocationStart)]), p_data, lengthInBytes) == 0)
        return false;

    memcpy(&(m_nvmPageCache[NVM_LOCATION_TO_INDEX(nvmLocationStart)]), p_data, lengthInBytes);

    return WriteCacheToNvm();
}

/**
    Queue the page cache to be written to NVM.
**/
bool NvmManager::WriteCacheToNvm() {
    m_pageModified = true;
    return FinishNvmWrite();
}

/**
    State machine to write the page cache to NVM.
**/
extern uint16_t AdcResultsRaw[AdcManager::ADC_CHANNEL_COUNT];
bool NvmManager::WriteCacheToNvmProc() {
    static int32_t *addressInNvmToWrite = reinterpret_cast<int32_t *>(NVMCTRL_USER);

    // Reading from the NVM immediately after writing to it sometimes returns
    // outdated data so we use a cache in RAM to store a copy of the data
    // that's currently written into non-volatile memory. To be safe, we only
    // read from the NVM once at start-up to initialize the cache and defer all
    // writes to the NVM until the cache is initialized.
    switch (m_writeState) {
    case IDLE:
        if (!m_pageModified) {
            break;
        }
        // The page cache was modified, start the write process
        m_writeState = CLEAR_PAGE_BUFFER;
        // Fall through

    // Check if the page buffer is dirty, clean if necessary
    case CLEAR_PAGE_BUFFER:
        if (NVMCTRL->STATUS.bit.LOAD) {
            // Is the NVM ready for a command
            if (!NVMCTRL->STATUS.bit.READY) {
                break;
            }
            NVMCTRL->CTRLB.reg = NVMCTRL_CTRLB_CMDEX_KEY | NVMCTRL_CTRLB_CMD_PBC;
        }

        // Clear the DONE Flag
        NVMCTRL->INTFLAG.bit.DONE = 1;
        m_writeState = ERASE_PAGE;
        // Fall through

    // Erase the user page; note that NVM must be erased prior to writing to
    // it (datasheet pg. 648, "Procedure for Manual Page Writes" section), and
    // the User Page only support Page Erase (not Block Erase)
    // Note: This will kill device critical information if not properly
    // restored (the device-critical information is stored in the first 32
    // bytes of the page cache, so it will be written back with the rest of the
    // cache)
    case ERASE_PAGE:
        // Is the NVM ready for a command
        if (!NVMCTRL->STATUS.bit.READY) {
            break;
        }

        // Do as much as possible before checking voltage & subsequently erasing 
        // the page
        // Set write mode to manual. It may be faster to use automatic
        NVMCTRL->CTRLA.bit.WMODE = NVMCTRL_CTRLA_WMODE_MAN;
        // Set the addr to the page that we want to address
        NVMCTRL->ADDR.reg = static_cast<uint32_t>(NVMCTRL_USER);
        // Reset our index
        m_quadWordIndex = 0;

        // Check the voltage and if good, erase page
        if (BlockWrite()) {
            m_writeState = IDLE;
            return false;
        }
        // It is now a race against capacitive drain to write as fast as possible
        // in order to not loose data
        NVMCTRL->CTRLB.reg = NVMCTRL_CTRLB_CMDEX_KEY | NVMCTRL_CTRLB_CMD_EP;
        m_writeState = WRITE_DATA;
        // Fall through

    // Copy the contents of the page cache into the page buffer in 128-bit
    // chunks, then write each chunk into NVM
#define CHUNK_SIZE 16 //bytes per chunk (128 bits per chunk)
#define num32sInPb      (NVMCTRL_PAGE_SIZE / sizeof(uint32_t))
#define num32sIn128     (CHUNK_SIZE / sizeof(uint32_t))
    case WRITE_DATA:
        
        // While data cannot be written while the page erase is happening, 
        // the write can be prepared

        // Tell the NVM the location of the 128-bit value to be written
        NVMCTRL->ADDR.reg =
            reinterpret_cast<uint32_t>(&addressInNvmToWrite[m_quadWordIndex]);
        // Copy each of 4 the 32-bit values into the page buffer
        memcpy( &addressInNvmToWrite[m_quadWordIndex], &m_nvmPageCache32[m_quadWordIndex], CHUNK_SIZE);
        m_quadWordIndex+=num32sIn128;

        // The Page Buffer cannot be written while a write command 
        // is executing in the NVM. Wait for the status ready flag.
        if (!NVMCTRL->STATUS.bit.READY) {
            break;
        }

        // Tell the NVM controller to write the 128-bit value
        NVMCTRL->CTRLB.reg = NVMCTRL_CTRLB_CMDEX_KEY | NVMCTRL_CTRLB_CMD_WQW;
        if (m_quadWordIndex >= num32sInPb) {
            m_writeState = IDLE;
            m_pageModified = false;
        }
        break;

    default:
        m_writeState = IDLE;
        break;
    }
    return true;
}

/**
    Populate the page cache from NVM.
**/
void NvmManager::PopulateCache() {
    // BK 9/9/20: Not sure why this was done, but it may slow down performance 
    // if the cache is disabled. 
    //NVMCTRL->CTRLA.bit.CACHEDIS0 = 1;
    //NVMCTRL->CTRLA.bit.CACHEDIS1 = 1;
    // Copy the contents of memory into a buffer
    memcpy(m_nvmPageCache, reinterpret_cast<const void *>(NVMCTRL_USER),
           NVMCTRL_PAGE_SIZE);
}

void NvmManager::MacAddress(uint8_t *macAddress) {
    uint64_t macNvm = Int64(NVM_LOC_MAC_FIRST);
    // If an invalid MAC address is detected, revert to 
    // the default MAC address to be able to come online.
    if (macNvm == UINT64_MAX || (macNvm >> 48)) {
        macNvm = DEFAULT_MAC_ADDRESS;
    }
    for (int8_t shift = 5; shift >= 0; shift--) {
        macAddress[5 - shift] = (macNvm >> shift * 8) & 0xFF;
    }
}

uint32_t NvmManager::SerialNumber() {
    return static_cast<uint32_t>(Int32(NVM_LOC_SERIAL_NUMBER));
}

bool NvmManager::BlockWrite() {
    //return StatusManager::Instance().StatusRT().bit.VSupplyUnderVoltage;
    return AdcManager::Instance().ConvertedResult(AdcManager::ADC_VSUPPLY_MON) 
           < UNDER_VOLTAGE_TRIP_CNT;
}

} // ClearCore namespace
