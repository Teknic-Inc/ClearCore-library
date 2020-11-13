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
    \file SDManager.h
    This class acts as a user friendly API for the SDFat library.

    This manager will allow you to use a micro SD card's file system with
    the ClearCore. It's most notable uses are data reading/writing, directory 
    navigation/editing, and .wav file playback through ports IO4 or IO5.
**/


#ifndef SDMANAGER_H_
#define SDMANAGER_H_

#include "SdFat.h"
#include "FatFile.h"
#include "WavPlayer.h"
#include <stdint.h>

//increase if more than 16 open files are needed at one time
#define MAX_FILE_SIZE 16

namespace ClearCore{

class SDManager {

public:

    #define O_RDONLY  0X00  ///< Open for reading only.
    #define O_WRONLY  0X01  ///< Open for writing only.
    #define O_RDWR    0X02  ///< Open for reading and writing.
    #define O_AT_END  0X04  ///< Open at EOF.
    #define O_APPEND  0X08  ///< Set append mode.
    #define O_CREAT   0x10  ///< Create file if it does not exist.
    #define O_TRUNC   0x20  ///< Truncate file to zero length.
    #define O_EXCL    0x40  ///< Fail if the file exists.
    #define O_SYNC    0x80  ///< Synchronized write I/O operations.

    #define O_ACCMODE (O_RDONLY|O_WRONLY|O_RDWR)  ///< Mask for access mode.
    typedef uint8_t oflag_t;

    typedef enum {
        REL_START,
        REL_END,
        REL_CUR,
    } relPosition;

    SDManager();

    /**
        \brief sets up SPI connection with SDCard

        \return true if success, false if failure
    **/
    bool Initialize();

    /**
        \brief Opens file on SD card

        \param[in] name of file to be opened or created

        \param[in] oflag, binary flag to set settings of the file

        \returns file index that is use with other SDManager functions,
                 -1 for failure

        \note oflags can be OR'd together to set multiple settings
              i.e: O_WRONLY|O_AT_END|O_CREAT
    **/
    int Open(const char *fileName, oflag_t oflag = O_RDONLY);

    /**
        \brief checks if a given file is open

        \param[in] fd, index of file to be checked

        \return true if open, false if closed
    **/
    bool IsOpen(int fd);

    /**
        \brief closes file on SD card

        \param[in] fd, index of file to be closed

        \return true if successful, false if failed

        \note after closing a file the index of the file
         can not be used again (unless the index is returned
         again by Open)
    **/
    bool Close(int fd);

    /**
        \brief checks for the existence of a file

        \param[in] pathName, name of path or file to check

        \return true if a file with the given path exists, else false

        \note works with directories or files
    **/
    bool Exists(char *pathName);

    /**
        \brief deletes current directory iff it is empty

        \param[in] pathName, path to directory to remove

        \return true if success, false if failed
    **/
    bool RmDir(char *pathName);

    /**
        \brief creates new directory with specified name

        \param[in] dirName, directory name

        \return true if successful, false if failed
    **/
    bool MkDir(char *dirName);

    /**
        \brief changes current working directory to specified path

        \param[in] pathName name of path the cwd is set to

        \return true if successful, false if failed
    **/
    bool ChDir(char *pathName);

    /**
        \brief path or file at specified location

        \param[in] origName, original file/dir name

        \param[in] newName, new file/dir name

        \return true for success, false for failure
    **/
    bool Rename(char *origName, char *newName);

    /**
        \brief deletes specified file

         \param[in] fd, this is the index of the file to be removed

        \return true for success, false for failure
    **/
    bool Remove(int fd);

    /**
        \brief checks if the specified file is available for
         reading or writing

        \param[in] fd, file index

        \return int of remaining bytes in the file
    **/
    int Available(int fd);

    /**
        \brief empties a file currently open

        \param[in] fd, index of file to flush

        \return true for success, false for failure
    **/
    bool Flush(int fd);

    /**
        \brief gives current position of specified file

        \param[in] fd, index of the file being checked

        \return int position of open file relative to the start
         of the file
    **/
    int Posn(int fd);

    /**
        \brief sets the current position of a specified file
         to a specified position 

        \param[in] fd index of the file 

        \param[in] offset, distance from relative position
                   that the current position set to

        \param[in] relPos enum set to beginning, end, or current position

        \return int distance from beginning
    **/
    bool Seek(int fd, int offset,relPosition relPos = REL_START);

    /**
        \brief reads a specified length of bytes from a specified file

        \param[in] fd index of the file 

        \param[in] dstBuf, array of bytes used to read data into

        \param[in] len, length in bytes of the data to be read
                   !Can not be longer than size of dstBuf!

        \param[in] aSync, bool to specify whether the data transfer
                   is done synchronously (blocks all code from running 
                   while transferring) or asynchronously (does not block code, 
                   but must be checked for completion before data is ready or 
                   another transfer is attempted)

        \param[out] dstBuf, on transfer completion the read data will be stored
                    in the given array ot pointer of bytes

        \return int number of bytes successfully read, -1 for failure
    **/
    int Read(int fd, void *dstBuf, size_t len, bool aSync = false);

    /**
        \brief writes a specified length of bytes to a specified file

        \param[in] fd index of the file 

        \param[in] srcBuf, array of bytes filled with data to write with

        \param[in] len, length in bytes of the data to be written
                   !Can not be longer than size of srcBuf!

        \param[in] aSync, TODO AW asynchronous write has not yet been implemented

        \return int number of bytes successfully written, -1 for failure
    **/
    int Write(int fd, void *srcBuf, size_t len, bool aSync = false);

        /**
        \brief writes a string to a specified file

        \param[in] fd index of the file 

        \param[in] str string constant to write

        \return int number of bytes successfully written, -1 for failure
    **/
    int StringWrite(int fd, const char *str);

    /**
        \brief gives the size in bytes of a specified file

        \param[in] fd index of the file 

        \return int size of file
    **/
    int Size(int fd);

    /**
        \brief returns the status of an asynchronous transfer

        \param[in] fd index of the file transferring

        \return bool true if no transfer is progress, 
                false if a transfer is in progress
    **/
    bool AsyncTransferComplete(int fd);

    /**
        \brief begins playing a specified .wav file in the current directory

        \param[in] volume value from 0 - 100 (!70 - 100 can be very loud!)

        \param[in] audioOut connector on the CLeaCore to play audio out of 
                   options: ConnectorIO4 or ConnectorIO5

        \param[in] filename name of wav file to play, i.e: Ring.wav

        \note volume increases exponentially, be careful with higher volume ranges
    **/
    void Play(int volume, DigitalInOutHBridge audioOut, const char *filename);
        
    /**
        \brief halts the playback of a wav file in progress
    **/
    void StopPlayback();
        
    /**
        \brief returns whether the audio player is finished

        \return bool true if no file is playing, 
                false if playback of a file is in progress

        \note PlaybackFinished must be called before playing another file
    **/
    bool PlaybackFinished();

    /**
        \brief adjusts the volume of the audio player

        \param[in] volume value from 0 - 100 (!70 - 100 can be very loud!)

        \note volume increases exponentially, be careful with higher volume ranges
    **/
    void SetAudioVolume(int volume);

    /**
        \brief changes the connector that the audio player uses

        \param[in] audioOut connector on the CLeaCore to play audio out of
        options: ConnectorIO4 or ConnectorIO5
    **/
    void SetAudioConnector(DigitalInOutHBridge audioOut);

private:
    
    //array of files, edit MAX_FILE_SIZE if more than 16 files are needed at one time
    FatFile ActiveFiles[MAX_FILE_SIZE];
    //SD Fat object
    SdFat SDLibrary;

    WavPlayer AudioPlayer;


}; // SDManager

}//Clear Core Namespace



#endif /* SDMANAGER_H_ */