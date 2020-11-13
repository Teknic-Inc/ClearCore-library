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
    EthernetManager implementation

    Implements an Ethernet port.
**/

#include "SDManager.h"

namespace ClearCore {

SDManager::SDManager(){}
    
    bool SDManager::Initialize(){
        return SDLibrary.begin();  
    }

    int SDManager::Open(const char *fileName, oflag_t oflag){
        int i = 0;
        while(i < MAX_FILE_SIZE){
            if(!ActiveFiles[i].isOpen()){
                //TODO add file open flag options, defaults to RD_ONLY
                if(!ActiveFiles[i].open(fileName,oflag)){
                    return i;
                }
                else{
                    return -1;
                }
            }
        }
        //array is full, default to error
        return -1;

    }

    bool SDManager::IsOpen(int fd){
        return ActiveFiles[fd].isOpen();
    }

    bool SDManager::Close(int fd){
        //check to see if fd entry exists
        if(!ActiveFiles[fd].isOpen()){
            return false;
        }
        return ActiveFiles[fd].close();
    }

    bool SDManager::Exists(char *pathName){
        return SDLibrary.exists(pathName);
    }

    bool SDManager::RmDir(char *pathName){
        return SDLibrary.rmdir(pathName);
    }

    bool SDManager::MkDir(char *dirName){
        return SDLibrary.mkdir(dirName);
    }

    bool SDManager::ChDir(char *pathName){
        return SDLibrary.chdir(pathName);
    }


    bool SDManager::Rename(char *origName,char *newName){
        return SDLibrary.rename(origName, newName);
    }

    bool SDManager::Remove(int fd){
        return ActiveFiles[fd].remove();
    }

    int SDManager::Available(int fd){
        return ActiveFiles[fd].available();
    }

    bool SDManager::Flush(int fd){
        return ActiveFiles[fd].truncate(0);
    }

    int SDManager::Posn(int fd){
        return ActiveFiles[fd].curPosition();
    }

    bool SDManager::Seek(int fd, int offset,relPosition relPos){
        switch(relPos){
            case REL_START:
                return ActiveFiles[fd].seekSet(offset);
            case REL_CUR:
                return ActiveFiles[fd].seekCur(offset);
            case REL_END:
                return ActiveFiles[fd].seekEnd(offset);
            default:
                return false;
        }    
    }

    int SDManager::Read(int fd, void *dstBuf, size_t len, bool aSync){
        if(aSync){
            return ActiveFiles[fd].readASync(dstBuf,len);
        }
        else{
            return ActiveFiles[fd].read(dstBuf,len);
        }
    }


    int SDManager::Write(int fd, void *srcBuf, size_t len, bool aSync){
        if(aSync){
            //asynchronous write has not yet been implemented
            return ActiveFiles[fd].writeASync(srcBuf,len);
        }
        else{
            return ActiveFiles[fd].write(srcBuf,len);
        }
    }

    int SDManager::StringWrite(int fd, const char *str){
        return ActiveFiles[fd].write(str);
    }

    int SDManager::Size(int fd){
        return ActiveFiles[fd].fileSize();
    }

    bool SDManager::AsyncTransferComplete(int fd){
        return ActiveFiles[fd].readWriteComplete();
    }

    void SDManager::Play(int volume, DigitalInOutHBridge audioOut, const char *filename){
        AudioPlayer.Play(volume, audioOut, filename);
    }
        
    void SDManager::StopPlayback(){
        AudioPlayer.StopPlayback();
    }
        
    bool SDManager::PlaybackFinished(){
        return AudioPlayer.PlaybackFinished();
    }

   void SDManager::SetAudioVolume(int volume){
        AudioPlayer.SetPlaybackVolume(volume);
   }

   void SDManager::SetAudioConnector(DigitalInOutHBridge audioOut){
        AudioPlayer.SetPlaybackConnector(audioOut);
   }

} // ClearCore namespace