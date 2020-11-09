/*Library by TMRh20 2012
Released into the public domain.*/

#include "ClearCore.h"
#include "ClearCoreTMRpcm.h"
#include <stdint.h>
#include "FatFile.h"

#define BUF_SIZE 8192


volatile bool m_reallyDone = false;
volatile bool m_switchSample = true;
volatile uint16_t m_sample;
uint8_t SDsamples[BUF_SIZE];
uint8_t SDsamples2[BUF_SIZE];
uint32_t m_frequencyHz = 16000;
int m_soundDataLength = 0;
uint8_t *m_soundData = 0;
uint8_t m_volume = 40;
uint32_t m_endOfDataPosn = 0;
bool sixteenBitFile = false;
FatFile *sFile;
DigitalInOutHBridge g_wav_speaker = ConnectorIO5;
DigitalInOutHBridge g_wav_speaker2 = ConnectorIO4;

extern "C" void TCC2_0_Handler(void) __attribute__((alias("PeriodicInterrupt")));

ClearCoreTMRpcm::ClearCoreTMRpcm(int volume, DigitalInOutHBridge audioOut) {
    m_volume = (uint8_t) volume;
    g_wav_speaker = audioOut;
    sFile = &wavFile;
}

bool ClearCoreTMRpcm::PlaybackFinished() {
    if(m_reallyDone){
        if(sFile->readWriteComplete()&&sFile->close()){
            ClearCore::ConnectorUsb.SendLine("Playback Finished");
            m_reallyDone = false;
            return true;
        }
    }
    return false;
}

void ClearCoreTMRpcm::Play(const char *filename) {
    //set connector to wave output mode
    g_wav_speaker.Mode(Connector::OUTPUT_WAVE);
    ConnectorIO4.Mode(Connector::OUTPUT_WAVE);


    if(!sFile->open(filename)){
        ClearCore::ConnectorUsb.SendLine("SD File Open Fail");
        return;
    }

    if (sFile->isOpen()) {

        ClearCore::ConnectorUsb.SendLine("File Open!");

        // Read the wave format from the file header
        ParseHeader(sFile);


        //Read the specified file using two buffers. One buffer loads and starts playback
        //Playback is called on timer driven interrupts, so we are clear to buffer more data
        //into a second buffer while the file starts playing.
        m_switchSample = true;
        sFile->read(SDsamples, BUF_SIZE);
        StartPlayback(BUF_SIZE);
    }
    else{
        ClearCore::ConnectorUsb.SendLine("SD Read Fail");
    }
}



// Taken from PCM library on Internets based on the one from Arduino Site
// This interrupt is called at m_frequencyHz Hz to load the next sample.
extern "C" void PeriodicInterrupt(void) {
    // Perform periodic processing here
    if (m_sample >= m_soundDataLength) {
        //g_wav_speaker.State(0);
        continuePlayback();
    }
    else if (sixteenBitFile) {
        g_wav_speaker.State((int16_t)((uint16_t)m_soundData[m_sample] + ((uint16_t)m_soundData[m_sample + 1] << 8)) >> m_volume);
        ConnectorIO4.State((int16_t)((uint16_t)m_soundData[m_sample + 2] + ((uint16_t)m_soundData[m_sample + 3] << 8)) >> m_volume);
        m_sample += 4;
    }
    else {
        g_wav_speaker.State(((int16_t)m_soundData[m_sample]) * m_volume);
        ConnectorIO4.State(((int16_t)m_soundData[m_sample + 1]) * m_volume);
        m_sample += 2;
    }

    // Ack the interrupt to clear the flag and wait for the next interrupt.
    ACK_PERIODIC_INTERRUPT;
}



void ClearCoreTMRpcm::StartPlayback(int length) {
    ClearCore::ConnectorUsb.SendLine("Start Playback");
    m_soundDataLength = length;

    // Enable the TCC2 peripheral
    // TCC2 and TCC3 share their clock configuration and they
    // are already configured to be clocked at 120 MHz from GCLK0.
    CLOCK_ENABLE(APBCMASK, TCC2_);

    TCC2->CTRLA.bit.ENABLE = 0; // Disable TCC2
    SYNCBUSY_WAIT(TCC2, TCC_SYNCBUSY_ENABLE);

    // Reset the TCC module so we know we are starting from a clean state
    TCC2->CTRLA.bit.SWRST = 1;
    while (TCC2->CTRLA.bit.SWRST) {
        continue;
    }

    // If the frequency requested is zero, disable the interrupt and bail out.
    if (m_frequencyHz == 0) {
        NVIC_DisableIRQ(TCC2_0_IRQn);
        return;
    }

    // Determine the clock prescaler and period value needed to achieve the
    // requested frequency.
    uint32_t period = (CPU_CLK + m_frequencyHz / 2) / m_frequencyHz;
    uint8_t prescale;
    // Make sure period is >= 1
    period = max(period, 1U);

    // Prescale values 0-4 map to prescale divisors of 1-16,
    // dividing by 2 each increment
    for (prescale = TCC_CTRLA_PRESCALER_DIV1_Val;
            prescale < TCC_CTRLA_PRESCALER_DIV16_Val && (period - 1) > UINT16_MAX;
            prescale++) {
        period = period >> 1;
    }
    // Prescale values 5-7 map to prescale divisors of 64-1024,
    // dividing by 4 each increment
    for (; prescale < TCC_CTRLA_PRESCALER_DIV1024_Val && (period - 1) > UINT16_MAX;
            prescale++) {
        period = period >> 2;
    }
    // If we have maxed out the prescaler and the period is still too big,
    // use the maximum period. This results in a ~1.788 Hz interrupt.
    if (period > UINT16_MAX) {
        TCC2->PER.reg = UINT16_MAX;
    }
    else {
        TCC2->PER.reg = period - 1;
    }
    TCC2->CTRLA.bit.PRESCALER = prescale;

    // Interrupt every period on counter overflow
    TCC2->INTENSET.bit.OVF = 1;
    TCC2->CTRLA.bit.ENABLE = 1; // Enable TCC2

    /* Set the interrupt priority and enable it */
    NVIC_SetPriority(TCC2_0_IRQn, 0);
    NVIC_EnableIRQ(TCC2_0_IRQn);

    m_soundDataLength = BUF_SIZE;
    m_sample = BUF_SIZE;
}

uint32_t ReadLE32(FatFile *theFile) {
    uint8_t buf[4];
    uint32_t value = 0;
    theFile->read(buf, sizeof(buf));
    for (int i = 0; i < 4; i++) {
        value += (uint32_t)buf[i] << (i * 8);
    }
    return value;
}

void ClearCoreTMRpcm::ParseHeader(FatFile *theFile) {
    uint32_t sampleRate = 0;
    uint32_t marker;
    uint32_t sampleBits;
    uint32_t chunkSize;

    theFile->seekSet(24);
    // SampleRate is bytes 24-27, little endian, hex
    sampleRate = ReadLE32(theFile);

    // Set the new sample frequency
    m_frequencyHz = sampleRate;
    ClearCore::ConnectorUsb.Send("Freq: ");
    ClearCore::ConnectorUsb.Send(sampleRate);
    ClearCore::ConnectorUsb.SendLine("  0x");

    //get Bits per Sample
    theFile->seekSet(32);
    sampleBits = ReadLE32(theFile);
    //get 16 MSB alone
    sampleBits = sampleBits >> 16;
    if (sampleBits == 16) {
        sixteenBitFile = true;
        if (m_volume == 0) {
            m_frequencyHz = 0;
        }
        else {
            m_volume = 5 - (m_volume / 20);
        }
    }
    else {
        sixteenBitFile = false;
    }
    ClearCore::ConnectorUsb.Send("Bits per Sample: ");
    ClearCore::ConnectorUsb.SendLine(sampleBits);
    marker = ReadLE32(theFile);
    chunkSize = ReadLE32(theFile);

    ClearCore::ConnectorUsb.Send(theFile->curPosition());
    ClearCore::ConnectorUsb.Send(": Marker: 0x");
    ClearCore::ConnectorUsb.Send(marker);
    ClearCore::ConnectorUsb.Send(" Size: 0x");
    ClearCore::ConnectorUsb.SendLine(chunkSize);
    m_endOfDataPosn = theFile->curPosition() + chunkSize;


}

void ClearCoreTMRpcm::StopPlayback() {
    ClearCore::ConnectorUsb.SendLine("Stop Playback");
    m_reallyDone = true;
    // Disable playback per-m_sample interrupt.
    g_wav_speaker.State(0);
    NVIC_DisableIRQ(TCC2_0_IRQn);
}

void ClearCoreTMRpcm::ResumePlayback(uint8_t *data, int length) {
    /*  ClearCore::ConnectorUsb.SendLine("Start Resume");*/
    m_soundData = data;
    m_soundDataLength = length;
    m_sample = 0;
}

void continuePlayback() {
    //ClearCore::ConnectorUsb.SendLine("Stop Playback Temp");
    if(sFile->available() && (sFile->curPosition() < (m_endOfDataPosn))) {
        size_t nbyte = BUF_SIZE;
        uint32_t temp = sFile->curPosition() + BUF_SIZE;
        //Check to see if less than one buffer is left
        if(temp > m_endOfDataPosn){
            //read the rest of the file from current position to end position
            nbyte = m_endOfDataPosn - sFile->curPosition();
        }
        if (m_switchSample) {     //play sample buffer 1, load sample buffer2
            if(sFile->readWriteComplete()) {
                sFile->readASync(SDsamples2, nbyte);
                m_soundData = SDsamples;
                m_sample = 0;
                m_switchSample = !m_switchSample;
            }
            return;
        }
        else { //play sample buffer 2, load sample buffer1
            if(sFile->readWriteComplete()) {
                sFile->readASync(SDsamples, nbyte);
                m_soundData = SDsamples2;
                m_sample = 0;
                m_switchSample = !m_switchSample;
            }
            return;
        }
    }
    else{
        m_reallyDone = true;
        // Disable playback per-m_sample interrupt.
        g_wav_speaker.State(0);
        NVIC_DisableIRQ(TCC2_0_IRQn);
        
    }
}
