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
    \file NvmManager.h
    Non-Volatile Memory Interface for the ClearCore Board
**/

#ifndef __NVMMANAGER_H__
#define __NVMMANAGER_H__

#include <stdint.h>
#include <sam.h>

#ifndef HIDE_FROM_DOXYGEN
namespace ClearCore {

/**
    \brief ClearCore Board Non-Volatile Memory Interface

    This class manages the non-volatile memory of the Teknic ClearCore board.

    Data is stored in the USER section of memory.
    the User section is protected from Chip Erase, making it ideal for
    persistent info like MAC Address, Serial Config, etc.
    The User section is erasable by page, and writable by quad word.
    We keep a page cached so that we know what to write back once we clear the
    page.
    The writes will write the cache back to memory quad word at a time.

    \note Writing to NVM does not immediately update what will be read from
    that memory, this is why we read from cache.

    \note Access will fail if the UF2 boot loader has not been run
**/
class NvmManager {
public:

    /**
        Define byte-offsets for user-accessible NVM space
    **/
    typedef enum {
        NVM_LOC_USER_START      = 0, // First user-accessible byte after
        // Microchip's reserved 32 bytes (Section
        // 9.4 NVM User Page Mapping in the
        // datasheet)

        NVM_LOC_RESERVED_TEKNIC = 416, // Reserved 64 bytes of data for
        // Teknic use

        NVM_LOC_USER_MAX = NVMCTRL_PAGE_SIZE - 32,   // 480

        NVM_LOC_HW_REVISION = NVM_LOC_USER_MAX - 18,   // 462
        NVM_LOC_SERIAL_NUMBER = NVM_LOC_USER_MAX - 16, // 464

        NVM_LOC_MAC_FIRST  = NVM_LOC_USER_MAX - 12,    // 468
        NVM_LOC_MAC_SECOND = NVM_LOC_USER_MAX - 8,     // 472

        NVM_LOC_DAC_ZERO = NVM_LOC_USER_MAX - 4,       // 476
        NVM_LOC_DAC_SPAN = NVM_LOC_USER_MAX - 2,       // 478

    } NvmLocations;

    /**
        Public accessor for singleton instance
    **/
    static NvmManager &Instance();

    /**
        \brief Read octet from NVM

        \param[in] nvmLocation location to read from
    **/
    int8_t Byte(NvmLocations nvmLocation);
    /**
        \brief Write octet to NVM

        \param[in] nvmLocation location to write to
        \param[in] newValue value to write

        \return True if the write was submitted, false otherwise
    **/
    bool Byte(NvmLocations nvmLocation, int8_t newValue);


    /**
        \brief Read 16-bit integer from NVM

        \param[in] nvmLocation location to read from

        \note nvmLocation is the byte address into NVM
    **/
    int16_t Int16(NvmLocations nvmLocation);
    /**
        \brief Write 16-bit integer to NVM

        \param[in] nvmLocation location to write to
        \param[in] newValue value to write

        \note nvmLocation is the byte address into NVM
        \return True if the write was submitted, false otherwise
    **/
    bool Int16(NvmLocations nvmLocation, int16_t newValue);


    /**
        \brief Read 32-bit integer from NVM

        \param[in] nvmLocation location to read from

        \note nvmLocation is the byte address into NVM
    **/
    int32_t Int32(NvmLocations nvmLocation);
    /**
        \brief Write 32-bit integer to NVM

        \param[in] nvmLocation location to write to
        \param[in] newValue value to write

        \note nvmLocation is the byte address into NVM
        \return True if the write was submitted, false otherwise
    **/
    bool Int32(NvmLocations nvmLocation, int32_t newValue);

    /**
        \brief Read 64-bit integer from NVM

        Reads the location at nvmLocationStart and nvmLocatinStart+1
        and appends to two together

        \param[in] nvmLocationStart location to start read
    **/
    int64_t Int64(NvmLocations nvmLocationStart);
    /**
        \brief Write 64-bit integer to NVM

        \param[in] nvmLocationStart location to start write
        \param[in] newValue value to write

        \note nvmLocation is the byte address into NVM
        \return True if the write was submitted, false otherwise
    **/
    bool Int64(NvmLocations nvmLocationStart, int64_t newValue);

    /**
        \brief Read a block of bytes from NVM

        \param[in] nvmLocationStart location to start read
        \param[in] lengthInBytes number of bytes to read
        \param[in] p_data pointer to store read data
    **/
    void BlockRead(NvmLocations nvmLocationStart, int lengthInBytes, uint8_t * const p_data);

    /**
        \brief Write a block of bytes to NVM

        \param[in] nvmLocationStart location to start write
        \param[in] lengthInBytes number of bytes to write
        \param[in] p_data pointer to store read data

        \return True if the write was submitted, false otherwise
    **/
    bool BlockWrite(NvmLocations nvmLocationStart, int lengthInBytes, uint8_t const * const p_data);


    /**
        \brief Get the MAC address of the ClearCore.

        \param[out] macAddress An unsigned byte pointer to store the MAC address
        of this device.
        \note Make sure to have allocated six bytes to store the full address!
        This function will always write six bytes starting at macAddress[0].
    **/
    void MacAddress(uint8_t *macAddress);

    /**
        \brief Get the serial number of the ClearCore as an unsigned 32-bit
        integer value.

        \return An unsigned 32-bit integer representation of the ClearCore's
        serial number.
    **/
    uint32_t SerialNumber();

    bool FinishNvmWrite() {
        while (m_pageModified || m_writeState != IDLE) {
            if (WriteCacheToNvmProc()) {
                continue;
            }
            else {
                return false;
            }
        }
        return true;
    }

    bool Synchonized() const {
        return !m_pageModified;
    }

private:

    typedef enum {
        IDLE,
        CLEAR_PAGE_BUFFER,
        ERASE_PAGE,
        WRITE_DATA,
    } WriteCacheState;

    // Set to false outside of constructor too in case of read/write call before
    // constructor is called.
    bool m_cacheInitialized = false;
    // Page cache is a byte array.
    int8_t m_nvmPageCache[NVMCTRL_PAGE_SIZE];
    // The page cache gets written to NVM as 32-bit pieces
    int32_t *m_nvmPageCache32;
    WriteCacheState m_writeState;
    uint8_t m_quadWordIndex;
    bool m_pageModified;

    /**
        \brief Constructor

        Will initialize the page cache if not already done
    **/
    NvmManager();

    /**
        \brief Populates the nvmPageCache from NVM and sets m_cacheInitialized
        flag

        \note Will try to lock the mutex, make sure the calling function has
        released the lock.
    **/
    void PopulateCache();

    /**
        \brief Write the cache to NVM.

        \return Success

        \note Will try to lock the mutex, make sure the calling function has
        released the lock.
    **/
    bool WriteCacheToNvm();

    /**
        \brief State machine to write cache to NVM

        \return Cache was successfully written

        \note Using the state machine time splices writing, making it take longer
    **/
    bool WriteCacheToNvmProc();

    bool BlockWrite();
}; //NvmManager

} // ClearCore namespace
#endif // HIDE_FROM_DOXYGEN
#endif // __NVMMANAGER_H__
