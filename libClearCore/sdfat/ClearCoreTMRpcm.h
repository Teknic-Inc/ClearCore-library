/*Library by TMRh20, Released into the public domain.*/

#include "ClearCore.h"
#include "FatFile.h"

#ifndef tmrPCM_h
#define tmrPCM_h

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
    WavPlayer(int volume = 40, DigitalInOutHBridge audioOut = ConnectorIO5);
    void Play(const char *filename);
    void StopPlayback();
    bool PlaybackFinished();

private:
    const uint16_t WAVE_HEADER_LENGTH = 44;
    FatFile wavFile;

    void ParseHeader(FatFile *theFile);
    void ResumePlayback(uint8_t *data, int length);
    void StartPlayback(int length);
};

#endif
