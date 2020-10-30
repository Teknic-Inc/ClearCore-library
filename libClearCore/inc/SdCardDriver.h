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
    \file SdCardDriver.h
    This class controls access to the micro SD Card reader.

    The class will provide SD card support for data logging, configuration
    files, and disk emulation.
**/

#ifndef __SDCARDDRIVER_H__
#define __SDCARDDRIVER_H__

#include <stdint.h>
#include "PeripheralRoute.h"
#include "SerialBase.h"


namespace ClearCore {

    /**
        \brief ClearCore SD card interface

        This class manages access to the micro SD Card reader.
    **/
    class SdCardDriver : public SerialBase {
        friend class SysManager;

    public:
#ifndef HIDE_FROM_DOXYGEN

        
        /**
            \brief Default constructor so this connector can be a global and
            constructed by SysManager
        **/
        SdCardDriver() {};

        /**
            \brief Signal an error in the SD card

            \param[in] errorCode An error code; constants are defined in SD.h
        **/
        void SetErrorCode(uint8_t errorCode) {
            m_errorCode = errorCode;
        }

        bool getSDTransferComplete() {
            return SDTransferComplete;
        }

        bool getSDBlockTransferComplete() {
            return SDBlockTransferComplete;
        }

        /**
            \brief Check if the SD card is in a fault state

            \return True if an error code is present
        **/
        bool IsInFault() {
            return (m_errorCode != 0);
        }

        /**
            \brief Checks if the DMA is actively in use

        **/
        void Refresh() {
                SDTransferComplete = this->SpiAsyncCheckComplete();

                switch (currentState) {
                    case INITIALIZING:
                     SpiTransferDataAsync(srcBuf,dstBuf,512);
                     bufCount--;
                     dstBuf += 512;
                     srcBuf += 512;
                     SDReadByte = 0x0;
                     if(bufCount == 0x0){
                         currentState = SDState::FINISHED;
                     }
                     else{
                         currentState = PROCESSING;
                     }
                        break;
                    case PROCESSING:
                        if(SDTransferComplete){
                            if(SDReadByte==0xFE){
                                currentState = SDState::INITIALIZING;
                            }
                            else{
                                SpiTransferDataAsync(&SDWriteByte,&SDReadByte,0x1);
                                
                            }
                        }
                        break;
                    case FINISHED:
                        if(SDTransferComplete){
                            SDBlockTransferComplete = true;
                            dstBuf = NULL;
                            srcBuf = NULL;
                            currentState = SDState::IDLE;
                        }
                        break;
                    case IDLE:
                        if(dstBuf != NULL && srcBuf!=NULL && bufCount != 0){
                            currentState = INITIALIZING;
                        }
                        break;
                    default:
                        break;
                }
  
        }

        void sendBlockASync(uint8_t *buf, size_t blockCount){
            for (size_t i = 0; i < blockCount*512; i++) {
                buf[i] = 0xFF;
            }
            dstBuf = buf;
            srcBuf = buf;
            bufCount = blockCount;
            SDBlockTransferComplete = false;
        }

        void receiveBlockASync(const uint8_t *buf, size_t count){
            //TODO AW srcbuf type changed
            //srcBuf = buf;
            dstBuf = NULL;
            bufCount = count;
            SDBlockTransferComplete = false;
            currentState = INITIALIZING;
        }


#endif // HIDE_FROM_DOXYGEN

    private:
        uint8_t m_errorCode;
        //flag accessed by the SDfat library to check if transfered SD data is done
        volatile bool SDTransferComplete = false;
        volatile bool SDBlockTransferComplete = true;
//         typedef enum {
//             TRANSFER_PENDING = 0,
//             TRANSFER_STARTED = 1,
//             TRANSFER_DONE = 2,
//             TRANSFER_ERR = 3,
//             TRANSFER_ERR_SD_NOT_PRESENT = 4,
//         } Status;

        typedef enum {
            INITIALIZING,
            PROCESSING,
            FINISHED,
            IDLE,
        } SDState;

        //Status currentStatus = TRANSFER_DONE;
        SDState currentState;
        //The single byte variables used for asynchronous transfer
        uint8_t SDReadByte;
        uint8_t SDWriteByte;
        //The multi-byte pointers used for asynchronous transfer
        uint8_t *dstBuf;
        uint8_t *srcBuf;
        //size of the multi-byte buffers
        size_t bufCount;


        /**
            Construction, wires in pins and non-volatile info.
        **/
        SdCardDriver(const PeripheralRoute *misoPin,
                     const PeripheralRoute *ssPin,
                     const PeripheralRoute *sckPin,
                     const PeripheralRoute *mosiPin,
                     uint8_t peripheral);
    };

} // ClearCore namespace

#endif // __SDCARDDRIVER_H__