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
#ifndef SdFat_h
#define SdFat_h
/**
 * \file
 * \brief SdFat class
 */
#include "SysCall.h"
//#include "WavPlayer.h"
#include "BlockDriver.h"
#include "FatLib/FatLib.h"
//------------------------------------------------------------------------------
/** SdFat version 1.1.2 */
#define SD_FAT_VERSION 10102
//==============================================================================
/**
 * \class SdBaseFile
 * \brief Class for backward compatibility.
 */
class SdBaseFile : public FatFile {
public:
    SdBaseFile() {}
    /**  Create a file object and open it in the current working directory.
     *
     * \param[in] path A path for a file to be opened.
     *
     * \param[in] oflag Values for \a oflag are constructed by a
     * bitwise-inclusive OR of open flags. see
     * FatFile::open(FatFile*, const char*, oflag_t).
     */
    SdBaseFile(const char *path, oflag_t oflag) : FatFile(path, oflag) {}
};
//-----------------------------------------------------------------------------
/**
 * \class SdFileSystem
 * \brief Virtual base class for %SdFat library.
 */
template<class SdDriverClass>
class SdFileSystem : public FatFileSystem {
public:
    /** Initialize file system.
     * \return true for success else false.
     */
    bool begin() {
        return FatFileSystem::begin(&m_card);
    }
    /** \return Pointer to SD card object */
    SdDriverClass *card() {
        m_card.syncBlocks();
        return &m_card;
    }
    /** \return The card error code */
    uint8_t cardErrorCode() {
        return m_card.errorCode();
    }
    /** \return the card error data */
    uint32_t cardErrorData() {
        return m_card.errorData();
    }

protected:
    SdDriverClass m_card;
};
//==============================================================================
/**
 * \class SdFat
 * \brief Main file system class for %SdFat library.
 */
class SdFat : public SdFileSystem<SdSpiCard> {
public:
    /** Initialize SD card and file system.
     *
     * \param[in] csPin SD card chip select pin.
     * \param[in] spiSettings SPI speed, mode, and bit order.
     * \return true for success else false.
     */
    bool begin(uint32_t clockSpeed = SPI_FULL_SPEED) {
        if(m_card.begin(clockSpeed) &&
            SdFileSystem::begin()){
            return true;
        }
        else{
            return false;
        }


    }
    /** Initialize SD card for diagnostic use only.
     *
     * \param[in] csPin SD card chip select pin.
     * \param[in] settings SPI speed, mode, and bit order.
     * \return true for success else false.
     */
    bool cardBegin(uint32_t clockSpeed = SPI_FULL_SPEED) {
        return m_card.begin(clockSpeed);
    }
    /** Initialize file system for diagnostic use only.
     * \return true for success else false.
     */
    bool fsBegin() {
        return FatFileSystem::begin(card());
    }

    /** Initialize and play WAV file using WavPlayer
     *  Make sure to initialize the SD card before playing a file
     *
     * \param[in] char* filename, name of file to open
     * \param[in] int volume, value from 0 to 100 that controls the volume of the WAV file playback
     * \param[in] DigitalInOutHBridge audioOut, specify the connector to play audio out of (IO4 to IO5)
     */
//     void playFile(const char *filename, int volume = 40, DigitalInOutHBridge audioOut = ConnectorIO5) {
//         WavPlayer player(volume, audioOut);
//         player.Play(filename);
//         while(!player.PlaybackFinished()){continue;}
//     }    
};

#endif  // SdFat_h
