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
#include "atomic_utils.h"
#include <cstring>
#include <sam.h>

namespace ClearCore {

// Wait until the NVM controller is ready to accept a new command.
#define WAIT_READY()                                                           \
    while (NVMCTRL->STATUS.bit.READY == 0) {                                   \
        continue;                                                              \
    }

// Execute a command.
// Setting the ADDR register tells the NVM which hardware address to use.
// The given cmd must be OR'd with NVMCTRL_CTRLB_CMDEX_KEY into the CTRLB
// register for the cmd to be executed.
#define EXEC_CMD(cmd)                                                          \
    WAIT_READY();                                                              \
    NVMCTRL->ADDR.reg = static_cast<uint32_t>(NVMCTRL_USER);                   \
    NVMCTRL->CTRLB.reg = NVMCTRL_CTRLB_CMDEX_KEY | (cmd);                      \
    WAIT_READY();

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

// Makeshift mutex.
// Simply making the cache atomic will not cover the case where
// the NVM is being accessed while a write is occurring and the
// NVM page has been cleared.
bool nvmMutexLocker = false;

void NvmMutexLock() {
    while (!atomic_test_and_set(&nvmMutexLocker)) {
        continue;
    }
}

void NvmMutexUnlock() {
    atomic_clear(&nvmMutexLocker);
}

bool CheckNvmMutexLock() {
    return atomic_load_n_relaxed(&nvmMutexLocker);
}

NvmManager &NvmManager::Instance() {
    static NvmManager *instance = new NvmManager();
    return *instance;
}

NvmManager::NvmManager() {
    WAIT_READY();

    NVMCTRL->CTRLA.bit.CACHEDIS0 = 1;
    NVMCTRL->CTRLA.bit.CACHEDIS1 = 1;

    if (!m_cacheInitialized) {
        PopulateCache();
        // Set a 32-bit pointer to the cache so we don't need to do this
        // every time we have to write it
        m_nvmPageCache32 = reinterpret_cast<int32_t *>(m_nvmPageCache);
    }
}

/**
    Read octet from NVM.
**/
int8_t NvmManager::Byte(NvmLocations nvmLocation) {
    if (!m_cacheInitialized) {
        PopulateCache();
    }

    // Check bounds - upper
    if (nvmLocation >= NvmLocations::NVM_LOC_USER_MAX) {
        return -1;
    }

    int8_t returnValue;
    NvmMutexLock();
    returnValue = m_nvmPageCache[NVM_LOCATION_TO_INDEX(nvmLocation)];
    NvmMutexUnlock();
    return returnValue;
}
/**
    Write octet to NVM.
**/
void NvmManager::Byte(NvmLocations nvmLocation, int8_t newValue) {
    if (!m_cacheInitialized) {
        PopulateCache();
    }

    // Check bounds - upper
    if (nvmLocation >= NvmLocations::NVM_LOC_USER_MAX) {
        return;
    }

    // Check bounds - if trying to write to Teknic reserved space, make
    // sure the unlock code is set first
    if (nvmLocation >= NvmLocations::NVM_LOC_RESERVED_TEKNIC) {
        // If trying to write into the Teknic reserved space, return if the
        // unlock code is not set
        if (NvmMgrUnlock != 0x3fadeb) {
            return;
        }
    }

    // Check if an update is actually needed.
    if (Byte(nvmLocation) == newValue) {
        return;
    }

    // Put the new desired value into the page cache.
    NvmMutexLock();
    m_nvmPageCache[NVM_LOCATION_TO_INDEX(nvmLocation)] = newValue;
    NvmMutexUnlock();

    WriteCacheToNvm();
}

/**
    Read 16-bit integer from NVM.
**/
int16_t NvmManager::Int16(NvmLocations nvmLocation) {
    if (!m_cacheInitialized) {
        PopulateCache();
    }

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
    NvmMutexLock();
    returnValue = address[0];
    NvmMutexUnlock();
    return returnValue;
}
/**
    Write 16-bit integer to NVM.
**/
void NvmManager::Int16(NvmLocations nvmLocation, int16_t newValue) {
    if (!m_cacheInitialized) {
        PopulateCache();
    }

    // Check bounds - upper
    if (nvmLocation >=
            (NvmLocations::NVM_LOC_USER_MAX - sizeof(uint16_t) + 1)) {
        return;
    }

    // Check bounds - if trying to write to Teknic reserved space, make
    // sure the unlock code is set first
    if (nvmLocation >=
            (NvmLocations::NVM_LOC_RESERVED_TEKNIC - sizeof(uint16_t) + 1)) {
        // If trying to write into the Teknic reserved space, return if the
        // unlock code is not set
        if (NvmMgrUnlock != 0x3fadeb) {
            return;
        }
    }

    // Check if an update is actually needed.
    if (Int16(nvmLocation) == newValue) {
        return;
    }

    // Get a pointer to the value to be written
    int8_t *byteAddress =
        &(m_nvmPageCache[NVM_LOCATION_TO_INDEX(nvmLocation)]);
    // Access it as a 16-bit array
    int16_t *address = reinterpret_cast<int16_t *>(byteAddress);

    // Put the new desired value into the page cache.
    NvmMutexLock();
    address[0] = newValue;
    NvmMutexUnlock();

    WriteCacheToNvm();
}

/**
    Read 32-bit integer from NVM.
**/
int32_t NvmManager::Int32(NvmLocations nvmLocation) {
    if (!m_cacheInitialized) {
        PopulateCache();
    }

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
    NvmMutexLock();
    returnValue = address[0];
    NvmMutexUnlock();
    return returnValue;
}

/**
    Write 32-bit integer to NVM.
**/
void NvmManager::Int32(NvmLocations nvmLocation, int32_t newValue) {
    if (!m_cacheInitialized) {
        PopulateCache();
    }

    // Check bounds - upper
    if (nvmLocation >=
            (NvmLocations::NVM_LOC_USER_MAX - sizeof(uint32_t) + 1)) {
        return;
    }

    // Check bounds - if trying to write to Teknic reserved space, make
    // sure the unlock code is set first
    if (nvmLocation >=
            (NvmLocations::NVM_LOC_RESERVED_TEKNIC - sizeof(uint32_t) + 1)) {
        // If trying to write into the Teknic reserved space, return if the
        // unlock code is not set
        if (NvmMgrUnlock != 0x3fadeb) {
            return;
        }
    }

    // Check if an update is actually needed.
    if (Int32(nvmLocation) == newValue) {
        return;
    }

    // Get a pointer to the value to be written
    int8_t *byteAddress =
        &(m_nvmPageCache[NVM_LOCATION_TO_INDEX(nvmLocation)]);
    // Access it as a 32-bit array
    int32_t *address = reinterpret_cast<int32_t *>(byteAddress);

    // Put the new desired value into the page cache.
    NvmMutexLock();
    address[0] = newValue;
    NvmMutexUnlock();

    WriteCacheToNvm();
}

/**
    Read 64-bit integer from NVM.
**/
int64_t NvmManager::Int64(NvmLocations nvmLocationStart) {
    if (!m_cacheInitialized) {
        PopulateCache();
    }

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
void NvmManager::Int64(NvmLocations nvmLocationStart, int64_t newValue) {
    if (!m_cacheInitialized) {
        PopulateCache();
    }

    // Check bounds - upper
    if (nvmLocationStart >=
            (NvmLocations::NVM_LOC_USER_MAX - sizeof(uint64_t) + 1)) {
        return;
    }

    // Check bounds - if trying to write to Teknic reserved space, make
    // sure the unlock code is set first
    if (nvmLocationStart >=
            (NvmLocations::NVM_LOC_RESERVED_TEKNIC - sizeof(uint64_t) + 1)) {
        // If trying to write into the Teknic reserved space, return if the
        // unlock code is not set
        if (NvmMgrUnlock != 0x3fadeb) {
            return;
        }
    }

    // Check if an update is actually needed.
    if (Int64(nvmLocationStart) == newValue) {
        return;
    }

    int32_t upper = newValue >> 32;
    int32_t lower = newValue;

    // Get a pointer to the value to be written
    int8_t *byteAddress =
        &(m_nvmPageCache[NVM_LOCATION_TO_INDEX(nvmLocationStart)]);
    // Access it as a 32-bit array
    int32_t *address = reinterpret_cast<int32_t *>(byteAddress);

    // Put the new desired value into the page cache.
    NvmMutexLock();
    address[0] = upper;
    address[1] = lower;
    NvmMutexUnlock();

    WriteCacheToNvm();
}

/**
    Write the page cache to NVM.
**/
void NvmManager::WriteCacheToNvm() {
    // Reading from the NVM immediately after writing to it sometimes returns
    // outdated data so we use a cache in RAM to store a copy of the data
    // that's currently written into non-volatile memory. To be safe, we only
    // read from the NVM once at start-up to initialize the cache and defer all
    // writes to the NVM until the cache is initialized.

    NvmMutexLock();

    WAIT_READY();
    NVMCTRL->CTRLA.bit.WMODE = NVMCTRL_CTRLA_WMODE_MAN;

    // Calculate the address.
    int32_t *addressInNvmToWrite = reinterpret_cast<int32_t *>(NVMCTRL_USER);

    WAIT_READY();

    // Check if the page buffer is dirty, clean if necessary
    if (NVMCTRL->STATUS.bit.LOAD) {
        EXEC_CMD(NVMCTRL_CTRLB_CMD_PBC);
        WAIT_READY();
    }

    // Erase the user page; note that NVM must be erased prior to writing to
    // it (datasheet pg. 648, "Procedure for Manual Page Writes" section), and
    // the User Page only support Page Erase (not Block Erase)
    // Note: This will kill device critical information if not properly
    // restored (the device-critical information is stored in the first 32
    // bytes of the page cache, so it will be written back with the rest of the
    // cache)
    NVMCTRL->INTFLAG.bit.DONE = 1;
    EXEC_CMD(NVMCTRL_CTRLB_CMD_EP);
    WAIT_READY();
    while (NVMCTRL->INTFLAG.bit.DONE == 0) {
        continue;
    }

    // Copy the contents of the page cache into the page buffer in 128-bit
    // chunks, then write each chunk into NVM
#define CHUNK_SIZE 16 //bytes per chunk (128 bits per chunk)
    uint16_t num32sInPb = NVMCTRL_PAGE_SIZE / sizeof(uint32_t);
    uint8_t num32sIn128 = CHUNK_SIZE / sizeof(uint32_t);
    // Copy 4 32-bit values into the page buffer in each step of this loop
    for (uint8_t i = 0; i < num32sInPb; i += num32sIn128) {
        WAIT_READY();

        // Copy each of 4 the 32-bit values into the page buffer
        for (uint8_t j = 0; j < num32sIn128; j++) {
            addressInNvmToWrite[i + j] = m_nvmPageCache32[i + j];
            WAIT_READY();
        }

        // Tell the NVM controller to write the 128-bit value from the page
        // buffer into NVM
        NVMCTRL->ADDR.reg =
            reinterpret_cast<uint32_t>(&addressInNvmToWrite[i]);
        NVMCTRL->INTFLAG.bit.DONE = 1;
        NVMCTRL->CTRLB.reg = NVMCTRL_CTRLB_CMDEX_KEY | NVMCTRL_CTRLB_CMD_WQW;
        WAIT_READY();

        while (NVMCTRL->STATUS.bit.LOAD) {
            continue;
        }
        while (NVMCTRL->INTFLAG.bit.DONE == 0) {
            continue;
        }
    }

    NvmMutexUnlock();
}

/**
    Populate the page cache from NVM.
**/
void NvmManager::PopulateCache() {
    NvmMutexLock();
    NVMCTRL->CTRLA.bit.CACHEDIS0 = 1;
    NVMCTRL->CTRLA.bit.CACHEDIS1 = 1;
    // Copy the contents of memory into a buffer
    memcpy(m_nvmPageCache, reinterpret_cast<const void *>(NVMCTRL_USER),
           NVMCTRL_PAGE_SIZE);
    m_cacheInitialized = true;
    NvmMutexUnlock();
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


#undef WAIT_READY
#undef EXEC_CMD
#undef NVM_LOCATION_TO_INDEX

} // ClearCore namespace