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

#ifndef WAVPLAYER_h
#define WAVPLAYER_h

#include "FatFile.h"
#include "SysManager.h"
#include "DigitalInOutHBridge.h"
#include <stdint.h>

namespace ClearCore{

// Periodic interrupt priority
// 0 is highest priority, 7 is lowest priority
// Recommended priority is >= 4 to not interfere with other processing
#define PERIODIC_INTERRUPT_PRIORITY     4
#define ACK_PERIODIC_INTERRUPT  TCC2->INTFLAG.reg = TCC_INTFLAG_MASK

// Not sure why, but the interrupt function would not see the function unless
// I declared the variable this way...
extern "C" {
    void continuePlayback();

}

// Define what HBridge output to use (either IO4 or IO5)

class WavPlayer {
public:
    WavPlayer();
    void Play(int volume, DigitalInOutHBridge audioOut, const char *filename);
    void StopPlayback();
    void SetPlaybackVolume(int volume);
    void SetPlaybackConnector(DigitalInOutHBridge audioOut);
    bool PlaybackFinished();

private:
    const uint16_t WAVE_HEADER_LENGTH = 44;
    FatFile wavFile;

    void ParseHeader(FatFile *theFile);
    void ResumePlayback(uint8_t *data, int length);
    void StartPlayback(int length);

    uint32_t m_frequencyHz = 16000;
};

}//ClearCore Namespace

#endif
