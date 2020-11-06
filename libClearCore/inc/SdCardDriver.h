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
        /**
            \brief Check the Refresh state machine for transfer completion

            \return True if transfer complete
        **/

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
                        if(SDTransferComplete){
                            if(SDReadByte[9] == 0x00){
                                SpiTransferDataAsync(SDBeginCommandByte,SDReadByte,7);
                                SDReadByte[9] = 0xFF;
                                break;
                            }
                            else if(SDReadByte[1]==0xFE){
                                currentState = SENDBLOCK;
                                break;
                            }
                            else{
                                SpiTransferDataAsync(SDWriteByte,SDReadByte,2);
                            }
                        }
                        break;
                    case SENDBLOCK:
                        bufCount--;
                        if(bufCount == 0x0){
                            if(bufOffset>0){
                                //if last block use offsetData
                                SpiTransferDataAsync(offsetData,offsetData,512);
                            }
                            SDReadByte[9] = 0x0;
                            currentState = SDState::SENDCOMMAND;
                            break;
                        }
                       //transfer a whole block
                       SpiTransferDataAsync(srcBuf,dstBuf,512);
                       dstBuf += 512;
                       srcBuf += 512;
                       SDReadByte[9] = 0x0;
                       currentState = PROCESSING;
                        break;
                    case PROCESSING:
                        if(SDTransferComplete){
                            if(SDReadByte[9]==0xFE){
                                currentState = SDState::SENDBLOCK;
                            }
                            else{
                                SpiTransferDataAsync(SDWriteByte,SDReadByte,10);
                                
                            }
                        }
                        break;
                    case SENDCOMMAND:
                        if(SDTransferComplete){
                            if(SDReadByte[9] == 0x00){
                                SpiTransferDataAsync(SDEndCommandByte,SDReadByte,9);
                                SDReadByte[9] = 0xFF;
                                break;
                            }
                            SpiTransferDataAsync(SDWriteByte,SDReadByte,8);
                            currentState = FINISHED;
                        }
                        break;      
                    case FINISHED:
                        if(SDTransferComplete){
                            if(bufOffset>0){
                                dstBuf -= bufSize;
                                //only shift buffer by offset if there is an offset
                                memmove(dstBuf,(dstBuf+bufOffset),(bufSize-bufOffset));
                                memcpy(dstBuf+bufSize-bufOffset,offsetData,bufOffset);
                            }
                                                        
                            dstBuf = NULL;
                            srcBuf = NULL;
                            SDBlockTransferComplete = true;
                            SpiSsMode(SerialBase::CtrlLineModes::LINE_OFF);
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

        void receiveBlocksASync(uint32_t block, uint8_t *buf, size_t blockCount, uint16_t offset){
            for (size_t i = 0; i < blockCount*512; i++) {
                buf[i] = 0xFF;
            }
            
            dstBuf = buf;
            srcBuf = buf;
            //Initialize offset data buffer
            for(int i = 0; i < 512; i++){
                offsetData[i] = 0xFF;
            }
            bufOffset = offset;
            //Set SDBeginCommand:
            // send command
            SDBeginCommandByte[0] = (0X52);

            // send argument
            uint8_t *pb = reinterpret_cast<uint8_t *>(&block);
            SDBeginCommandByte[1] = pb[3];
            SDBeginCommandByte[2] = pb[2];
            SDBeginCommandByte[3] = pb[1];
            SDBeginCommandByte[4] = pb[0];

            // send CRC - correct for CMD0 with arg zero or CMD8 with arg 0X1AA
            SDBeginCommandByte[5] = (0X87);

            // discard first fill byte to avoid MISO pull-up problem.
            SDBeginCommandByte[6] = 0xFF;

            //Set SDEndCommand:
            SDEndCommandByte[0] = 0xFF;
            SDEndCommandByte[1] = 0xFF;
            // send command
            SDEndCommandByte[2] = (0X4C);

            // send argument
            SDEndCommandByte[3] = 0X00;
            SDEndCommandByte[4] = 0X00;
            SDEndCommandByte[5] = 0X00;
            SDEndCommandByte[6] = 0X00;

            // send CRC - correct for CMD0 with arg zero or CMD8 with arg 0X1AA
            SDEndCommandByte[7] = (0X87);

            // discard first fill byte to avoid MISO pull-up problem.
            SDEndCommandByte[8] = 0xFF;
            bufCount = blockCount + 1;
            bufSize = blockCount * 512;
            SDReadByte[9] = 0x00;
            SDBlockTransferComplete = false;
        }

        void sendBlocskASync(uint8_t *buf, size_t count){
            //TODO AW Eventually implement ASync write functionality? 
            srcBuf = buf;
            dstBuf = NULL;
            bufCount = count + 1;
            SDBlockTransferComplete = false;
        }



#endif // HIDE_FROM_DOXYGEN

    private:
        uint8_t m_errorCode;
        //flag accessed by the SDfat library to check if transfered SD data is done
        volatile bool SDTransferComplete = false;
        volatile bool SDBlockTransferComplete = true;

        typedef enum {
            INITIALIZING,
            SENDBLOCK,
            PROCESSING,
            FINISHED,
            SENDCOMMAND,
            IDLE,
        } SDState;

        //Status currentStatus = TRANSFER_DONE;
        SDState currentState;
        //The single byte variables used for asynchronous transfer
        uint8_t SDReadByte[10];
        uint8_t SDWriteByte[10];
        uint8_t SDBeginCommandByte[10];
        uint8_t SDEndCommandByte[10];
        //The multi-byte pointers used for asynchronous transfer
        uint8_t *dstBuf;
        uint8_t *srcBuf;
        //size of the multi-byte buffers
        size_t bufCount;
        size_t bufSize;
        //offset data handlers:
        uint16_t bufOffset;
        uint8_t offsetData[512];




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