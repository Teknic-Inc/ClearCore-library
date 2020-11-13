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

#define SD_READ_COMMAND 0xFF
#define SD_RESPONSE_ATTEMPTS 5
#define SD_READY_FLAG 0x00
#define SD_BLOCK_SIZE 512

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
                            if(SDReadByte[9] == SD_READY_FLAG){
                                SpiTransferDataAsync(SDBeginCommandByte,SDReadByte,7);
                                //set unused Read Byte for use as flag
                                SDReadByte[9] = SD_READ_COMMAND;
                                responseTimeOut = SD_RESPONSE_ATTEMPTS;
                            }
                            else if(SDReadByte[1]==0xFE){
                                currentState = SENDBLOCK;
                            }
                            else if(responseTimeOut == 0){
                                //if response from card takes too many cycles, end transfer
                                currentState = SENDCOMMAND;
                            }
                            else{
                                //send read commands until ready flag is received
                                SpiTransferDataAsync(SDWriteByte,SDReadByte,2);
                                responseTimeOut--;
                            }
                        }
                        break;
                    case SENDBLOCK:
                       //transfer a whole block
                       SpiTransferDataAsync(sdWriteData,dataReadBuffer,SD_BLOCK_SIZE);
                       dataReadBuffer += SD_BLOCK_SIZE;
                       bufCount--;
                       SDReadByte[9] = 0x0;
                       if(bufCount == 0x0){
                            currentState = SDState::SENDCOMMAND;
                            break;
                       }
                       responseTimeOut = SD_RESPONSE_ATTEMPTS;
                       currentState = PROCESSING;
                       break;
                    case PROCESSING:
                        if(SDTransferComplete){
                            if(SDReadByte[9]==0xFE){
                                currentState = SDState::SENDBLOCK;
                            }
                            else if(responseTimeOut == 0){
                                //if response from card takes too many cycles, end transfer
                                currentState = SENDCOMMAND;
                            }
                            else{
                                SpiTransferDataAsync(SDWriteByte,SDReadByte,10);
                                responseTimeOut--;
                            }
                        }
                        break;
                    case SENDCOMMAND:
                        if(SDTransferComplete){
                            if(SDReadByte[9] == SD_READY_FLAG){
                                SpiTransferDataAsync(SDEndCommandByte,SDReadByte,9);
                                SDReadByte[9] = SD_READ_COMMAND;
                                break;
                            }
                            SpiTransferDataAsync(SDWriteByte,SDReadByte,8);
                            currentState = FINISHED;
                        }
                        break;      
                    case FINISHED:
                        if(SDTransferComplete){
                            //decrement data read buffer by the nu
                            dataReadBuffer -= (((bufSize+bufOffset)>>9)+1)*SD_BLOCK_SIZE;
                            memcpy(dstBuf,dataReadBuffer+bufOffset,bufSize);
                                                        
                            dstBuf = NULL;
                            delete [] dataReadBuffer;
                            dataReadBuffer = NULL;
                            SDBlockTransferComplete = true;
                            SpiSsMode(SerialBase::CtrlLineModes::LINE_OFF);
                            currentState = SDState::IDLE;
                        } 
                        break;
                    case IDLE:
                        if(dstBuf != NULL && bufCount != 0){
                            currentState = INITIALIZING;
                        }
                        break;
                    default:
                        break;
                }
  
        }

        void receiveBlocksASync(uint32_t block, uint8_t *buf, size_t byteCount, uint16_t offset){
            
            dstBuf = buf;
            dataReadBuffer = new uint8_t[byteCount + (SD_BLOCK_SIZE*2)];
            bufOffset = offset;
            //Set SDBeginCommand:
            // send 0x52 (read multiple blocks) command
            SDBeginCommandByte[0] = (0X52);

            // send argument (block address in big endian)
            uint8_t *pb = reinterpret_cast<uint8_t *>(&block);
            SDBeginCommandByte[1] = pb[3];
            SDBeginCommandByte[2] = pb[2];
            SDBeginCommandByte[3] = pb[1];
            SDBeginCommandByte[4] = pb[0];

            // send CRC - correct for CMD0 with arg zero or CMD8 with arg 0X1AA
            SDBeginCommandByte[5] = (0X87);

            // discard first fill byte to avoid MISO pull-up problem.
            SDBeginCommandByte[6] = SD_READ_COMMAND;

            //Set SDEndCommand:
            //send two read commands to discard delimiter
            SDEndCommandByte[0] = SD_READ_COMMAND;
            SDEndCommandByte[1] = SD_READ_COMMAND;
            // send 0x4C (end transaction) command
            SDEndCommandByte[2] = (0X4C);

            // end command does not take argument, fill with NULL
            SDEndCommandByte[3] = 0X00;
            SDEndCommandByte[4] = 0X00;
            SDEndCommandByte[5] = 0X00;
            SDEndCommandByte[6] = 0X00;

            // send CRC - correct for CMD0 with arg zero or CMD8 with arg 0X1AA
            SDEndCommandByte[7] = (0X87);

            // discard first fill byte to avoid MISO pull-up problem.
            SDEndCommandByte[8] = SD_READ_COMMAND;
            //calculate # of full blocks to read
            bufCount = ((byteCount+offset)>>9)+1;
            bufSize = byteCount;
            SDReadByte[9] = SD_READY_FLAG;
            SDBlockTransferComplete = false;
        }

        void sendBlocskASync(uint32_t block, const uint8_t *buf, size_t byteCount, uint16_t offset){
             //TODO AW Finish implementation of asynchronous writing
             srcBuf = buf;
             dataReadBuffer = new uint8_t[byteCount + (SD_BLOCK_SIZE*2)];
             bufOffset = offset;
             //Set SDBeginCommand:
             // send command
             SDBeginCommandByte[0] = (0X59);

             // send argument
             uint8_t *pb = reinterpret_cast<uint8_t *>(&block);
             SDBeginCommandByte[1] = pb[3];
             SDBeginCommandByte[2] = pb[2];
             SDBeginCommandByte[3] = pb[1];
             SDBeginCommandByte[4] = pb[0];

             // send CRC - correct for CMD0 with arg zero or CMD8 with arg 0X1AA
             SDBeginCommandByte[5] = (0X87);

             // discard first fill byte to avoid MISO pull-up problem.
             SDBeginCommandByte[6] = SD_READ_COMMAND;
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
        const uint8_t *srcBuf;
        //size of the multi-byte buffers
        size_t bufCount;
        size_t bufSize;
        //offset data handlers:
        uint16_t bufOffset;
        uint8_t sdWriteData[SD_BLOCK_SIZE];
        uint8_t *dataReadBuffer;
        int responseTimeOut;




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