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
    ADC Peripheral Manager for the ClearCore Board

    This class manages the ADC peripheral on the Teknic ClearCore.
**/

#include "AdcManager.h"
#include <cstring>
#include <stdio.h>
#include <sam.h>
#include "DmaManager.h"
#include "HardwareMapping.h"
#include "ShiftRegister.h"
#include "StatusManager.h"
#include "SysUtils.h"

namespace ClearCore {

extern ShiftRegister ShiftReg;
extern StatusManager &StatusMgr;
AdcManager &AdcMgr = AdcManager::Instance();

constexpr float AdcManager::ADC_INITIAL_FILTER_VALUE_V[ADC_CHANNEL_COUNT];
constexpr float AdcManager::ADC_CHANNEL_MAX_FLOAT[ADC_CHANNEL_COUNT];

// Uncomment to generate an interrupt when the ADC result DMA transfer completes
//#define DEBUG_ADC_RESULT_TIMING

/**
    ADC conversion results DMA data destination
**/
volatile uint16_t AdcResultsRaw[AdcManager::ADC_CHANNEL_COUNT] = {0};

/**
    ADC channel selection DMA data source structure
**/
union adcDSeqCfg {
    struct {
        ADC_INPUTCTRL_Type INPUTCTRL; ///< Offset: 0x04 (R/W 16) Input Control
        ADC_CTRLB_Type CTRLB;         ///< Offset: 0x06 (R/W 16) Control B
    } bit;
    uint32_t reg;
#ifdef __cplusplus
    adcDSeqCfg() {
        reg = 0;
    }
    adcDSeqCfg(uint16_t inputCtrl, uint16_t ctrlb) {
        bit.INPUTCTRL.reg = inputCtrl;
        bit.CTRLB.reg = ctrlb;
    }
#endif
};

// ADC channel selection DMA data source
// The first argument is the full INPUTCTRL register value that will be loaded
// into the ADC
// Note: The last position also has the Sequence stop bit enabled
//       to alert the ADC that the sequence is finished
// Note: index matched to AdcChannels
static const adcDSeqCfg adcSequence[AdcManager::ADC_CHANNEL_COUNT] = {
    adcDSeqCfg(ADC_INPUTCTRL_MUXPOS_AIN4, 0),
    adcDSeqCfg(ADC_INPUTCTRL_MUXPOS_AIN5, 0),
    adcDSeqCfg(ADC_INPUTCTRL_MUXPOS_AIN6, 0),
    adcDSeqCfg(ADC_INPUTCTRL_MUXPOS_AIN7, 0),
    adcDSeqCfg(ADC_INPUTCTRL_MUXPOS_AIN8, 0),
    adcDSeqCfg(ADC_INPUTCTRL_MUXPOS_AIN9, 0),
    adcDSeqCfg(ADC_INPUTCTRL_MUXPOS_AIN10, 0),
    adcDSeqCfg(ADC_INPUTCTRL_MUXPOS_AIN11 | ADC_INPUTCTRL_DSEQSTOP, 0),
};

static inline void WaitAdc() {
    while (ADC1->STATUS.bit.ADCBUSY) {
        continue;
    }
}

AdcManager &AdcManager::Instance() {
    static AdcManager *instance = new AdcManager();
    return *instance;
}

/**
    Constructor
**/
AdcManager::AdcManager()
    : m_initialized(false),
      m_AdcTimeout(false),
      m_shiftRegSnapshot(UINT32_MAX),
      m_shiftRegPending(UINT32_MAX),
      m_AdcResolution(ADC_RESOLUTION_DEFAULT),
      m_AdcResPending(ADC_RESOLUTION_DEFAULT),
      m_AdcTimeoutLimit(ADC_TIMEOUT_DEFAULT),
      m_AdcBusyCount(0) {}

/**
    Initialize the ADC to power-up state.
**/
void AdcManager::Initialize() {
    m_initialized = false;
    m_AdcTimeout = false;
    m_shiftRegSnapshot = UINT32_MAX;
    m_shiftRegPending = UINT32_MAX;
    m_AdcResolution = ADC_RESOLUTION_DEFAULT;
    m_AdcResPending = ADC_RESOLUTION_DEFAULT;
    m_AdcTimeoutLimit = ADC_TIMEOUT_DEFAULT;
    m_AdcBusyCount = 0;

    // Set default filter constants
    for (uint8_t i = 0; i < ADC_CHANNEL_COUNT; i++) {
        m_analogFilter[i].Tc_ms(ADC_IIR_FILTER_TC_MS);
    }

    // Configure internal analog inputs: Sdrvr2, Sdrvr3, VBus, 5V Ob monitor
    const uint8_t INTERNAL_ADC_INPUTS = 4;
    const PeripheralRoute *adcsToSetup[INTERNAL_ADC_INPUTS] = {
        &_5VOB_MON, &Vsupply_MON_IO_4and5_RST,
        &Sdrvr2_iMon, &Sdrvr3_iMon
    };

    for (uint8_t i = 0; i < INTERNAL_ADC_INPUTS; i++) {
        const PeripheralRoute *prPtr = adcsToSetup[i];

        ClearCorePorts adcGpioPort = prPtr->gpioPort;
        uint8_t adcGpioPin = prPtr->gpioPin;

        PMUX_SELECTION(adcGpioPort, adcGpioPin, PER_ANALOG);
        PMUX_ENABLE(adcGpioPort, adcGpioPin);
    }

    // Set up ADC

    // Set the clock source for ADC1 to GCLK4 (48 MHz) and enable
    // the peripheral channel
    SET_CLOCK_SOURCE(ADC1_GCLK_ID, 4);

    // Enables the peripheral clock to ADC1
    CLOCK_ENABLE(APBDMASK, ADC1_);

    // Reset the ADC1 module
    ADC1->CTRLA.bit.SWRST = 1;
    SYNCBUSY_WAIT(ADC1, ADC_SYNCBUSY_SWRST);

    // Configure the ADC read resolution
    AdcResChange();

    // Set clock pre-scaler to 4 to result in a clock signal of 48/4 = 12 MHz
    // Note: ADC can pre-scale the clock a minimum of 2.
    ADC1->CTRLA.bit.PRESCALER = ADC_CTRLA_PRESCALER_DIV4_Val;

    // Set input to channel AIN4 and pause the DMA input sequencing
    ADC1->INPUTCTRL.reg |= ADC_INPUTCTRL_MUXPOS_AIN4 | ADC_INPUTCTRL_DSEQSTOP;
    SYNCBUSY_WAIT(ADC1, ADC_SYNCBUSY_INPUTCTRL);

    // Setup the DMA input/result transfers
    DmaInit();

    // Update INPUTCTRL from the DMA engine
    ADC1->DSEQCTRL.bit.INPUTCTRL = 1;
    SYNCBUSY_WAIT(ADC1, ADC_SYNCBUSY_INPUTCTRL);
    ADC1->DSEQCTRL.bit.AUTOSTART = 1;

    // Enable reference buffer compensation and set the reference to VDDANA
    ADC1->REFCTRL.reg |= ADC_REFCTRL_REFCOMP | ADC_REFCTRL_REFSEL_INTVCC1;
    SYNCBUSY_WAIT(ADC1, ADC_SYNCBUSY_REFCTRL);

    // Enable sampling period offset compensation
    // Note: setting sample length fairly long since the ADC readings will be
    // performed in the background, which results in more reliable readings.
    // Setting the sample length to 31 uses approximately 20% of the available
    // time when doing 8 12-bit readings per 5 kHz interrupt slot.
    ADC1->SAMPCTRL.reg = ADC_SAMPCTRL_SAMPLEN(31);
    SYNCBUSY_WAIT(ADC1, ADC_SYNCBUSY_SAMPCTRL);

    ADC1->DBGCTRL.bit.DBGRUN = 1;

    // Kick off the first ADC conversion
    DmaUpdate();

    // Enable ADC
    ADC1->CTRLA.bit.ENABLE = 0x01;
    SYNCBUSY_WAIT(ADC1, ADC_SYNCBUSY_ENABLE);

    // Wait for the first conversion to complete
    while (DmaManager::Channel(DMA_ADC_RESULTS)->CHCTRLA.bit.ENABLE) {
        continue;
    }
    while (DmaManager::Channel(DMA_ADC_SEQUENCE)->CHCTRLA.bit.ENABLE) {
        continue;
    }

    WaitAdc();

    // Populate initial values
    for (uint32_t i = 0; i < ADC_CHANNEL_COUNT; i++) {
        uint16_t val = ADC_INITIAL_FILTER_VALUE_V[i] * (1 << 15) /
                       ADC_CHANNEL_MAX_FLOAT[i];
        m_AdcResultsConverted[i] = val;
        m_AdcResultsConvertedFiltered[i] = val;
        m_analogFilter[i].Reset(val);
    }

    m_initialized = true;
}

/**
    Update systems at the sample rate
**/
void AdcManager::Update() {
    if (!m_initialized) {
        return;
    }

    // If the previous conversion isn't complete or there are more conversions
    // still to be performed, increment the timeout counter
    if (ADC1->STATUS.bit.ADCBUSY ||
            DmaManager::Channel(DMA_ADC_RESULTS)->CHCTRLA.bit.ENABLE) {
        // If the counter is greater than the timeout, throw an error
        if (++m_AdcBusyCount >= m_AdcTimeoutLimit) {
            m_AdcTimeout = true;
        }
    }
    else {
        // Clear the error and reset the counter
        m_AdcBusyCount = 0;
        m_AdcTimeout = false;

        // Copy the finished results into m_AdcResultsConverted and convert to
        // Q15
        for (uint8_t i = 0; i < ADC_CHANNEL_COUNT; i++) {
            // If HBridgeReset is set, do not update the VSupply value
            if (i == ADC_VSUPPLY_MON && StatusMgr.StatusRT().bit.HBridgeReset) {
                continue;
            }
            // Normalize the ADC results to a Q15 value
            m_AdcResultsConverted[i] =
                AdcResultsRaw[i] << (15 - m_AdcResolution);
        }

        // Kick off next conversion sequence
        if (m_AdcResolution != m_AdcResPending) {
            AdcResChange();
        }
        m_shiftRegSnapshot = m_shiftRegPending;
        m_shiftRegPending = ShiftReg.LastOutput();
        DmaUpdate();
    }

    // Apply IIR filtering even if the ADC values have not been updated
    for (uint8_t i = 0; i < ADC_CHANNEL_COUNT; i++) {
        m_analogFilter[i].Update(m_AdcResultsConverted[i]);
        m_AdcResultsConvertedFiltered[i] = m_analogFilter[i].LastOutput();
    }
}

/**
    Initialize the DMA engine to stream ADC conversions and results.
**/
void AdcManager::DmaInit() {
    // DMAC channel 0 is used to retrieve results from the ADC.
    // Channel 0 is triggered by the ADC result ready flag.
    // When triggered, the DMAC CH 0 should move the ADC result
    // to the program-created AdcResultsRaw array. The DMAC needs to
    // move 16-bits(one HWORD) of data(max output from the ADC).

    /***************************************************************
     * DMA_ADC_RESULTS Channel
     * Read each ADC result and store it into the AdcResult array.
     ***************************************************************/
    DmacChannel *channel;
    DmacDescriptor *baseDesc;
    channel = DmaManager::Channel(DMA_ADC_RESULTS);
    baseDesc = DmaManager::BaseDescriptor(DMA_ADC_RESULTS);
    // Configure DMA channel 0 to stream results from the ADC
    // Disable the channel so it can be written to
    channel->CHCTRLA.reg &= ~DMAC_CHCTRLA_ENABLE;

    channel->CHCTRLA.reg = DMAC_CHCTRLA_SWRST;
    // Wait for the reset to finish
    while (channel->CHCTRLA.reg == DMAC_CHCTRLA_SWRST) {
        continue;
    }

    channel->CHCTRLA.reg = DMAC_CHCTRLA_TRIGSRC(ADC1_DMAC_ID_RESRDY) |
                           DMAC_CHCTRLA_TRIGACT_BURST |
                           DMAC_CHCTRLA_BURSTLEN_SINGLE;

#ifdef DEBUG_ADC_RESULT_TIMING
    // Enable channel completion interrupts
    channel->CHINTENSET.reg = DMAC_CHINTENSET_TCMPL;
    NVIC_EnableIRQ(static_cast<IRQn_Type>(DMAC_0_IRQn + DMA_ADC_RESULTS));
#endif

    // Descriptors can work like linked lists. Since this is the only
    // transfer descriptor for the channel, point to 0 to stop transactions
    baseDesc->DESCADDR.reg = static_cast<uint32_t>(0);
    baseDesc->SRCADDR.reg = (uint32_t)&ADC1->RESULT.reg;
    baseDesc->BTCNT.reg = ADC_CHANNEL_COUNT;

    // End address
    baseDesc->DSTADDR.reg = (uint32_t)(AdcResultsRaw + ADC_CHANNEL_COUNT);
    baseDesc->BTCTRL.reg =
        DMAC_BTCTRL_BEATSIZE_HWORD | DMAC_BTCTRL_DSTINC | DMAC_BTCTRL_VALID;

    /***************************************************************
     * DMA_ADC_SEQUENCE Channel
     * This tells the ADC what channel to read
     ***************************************************************/
    channel = DmaManager::Channel(DMA_ADC_SEQUENCE);
    // Disable and reset the channel so it is clean to setup
    channel->CHCTRLA.reg &= ~DMAC_CHCTRLA_ENABLE;
    channel->CHCTRLA.reg = DMAC_CHCTRLA_SWRST;
    // Wait for the reset to finish
    while (channel->CHCTRLA.reg == DMAC_CHCTRLA_SWRST) {
        continue;
    }

    // Configure the channel to be triggered from the ADC1 DMAC sequence start
    // Trigger will cause a single burst(one transfer)
    channel->CHCTRLA.reg = DMAC_CHCTRLA_TRIGSRC(ADC1_DMAC_ID_SEQ) |
                           DMAC_CHCTRLA_TRIGACT_BURST |
                           DMAC_CHCTRLA_BURSTLEN_SINGLE;

    baseDesc = DmaManager::BaseDescriptor(DMA_ADC_SEQUENCE);
    // Descriptors can work like linked lists. Since this is the only
    // transfer descriptor for the channel, point to 0 to stop transactions
    baseDesc->DESCADDR.reg = static_cast<uint32_t>(0);
    // Point the src to the end of the adcSequence. The transfer will copy the
    // data before the src addr.
    baseDesc->SRCADDR.reg =
        (reinterpret_cast<uint32_t>(&adcSequence)) + sizeof(adcSequence);
    baseDesc->BTCNT.reg = ADC_CHANNEL_COUNT;
    // The Destination is the ADC register for sequence data.
    // The sequence data is what will be moved into the ADC.
    baseDesc->DSTADDR.reg =
        reinterpret_cast<uint32_t>(&REG_ADC1_DSEQDATA); // end address
    baseDesc->BTCTRL.reg = DMAC_BTCTRL_BEATSIZE_WORD | DMAC_BTCTRL_STEPSEL_SRC |
                           DMAC_BTCTRL_VALID | DMAC_BTCTRL_SRCINC;
}

/**
    Initialize the DMA engine to stream ADC conversions and results.
**/
void AdcManager::DmaUpdate() {
    // Start channel
    DmaManager::Channel(DMA_ADC_RESULTS)->CHCTRLA.reg |= DMAC_CHCTRLA_ENABLE;
    DmaManager::Channel(DMA_ADC_SEQUENCE)->CHCTRLA.reg |= DMAC_CHCTRLA_ENABLE;

    // Restart the DMA sequence to the ADC
    ADC1->INPUTCTRL.reg &= ~ADC_INPUTCTRL_DSEQSTOP;
}

bool AdcManager::AdcResolution(uint8_t resolution) {
    switch (resolution) {
        case 8:
        case 10:
        case 12:
            m_AdcResPending = resolution;
            break;
        // 16 bit is not supported
        default:
            // Invalid value
            return false;
    }
    // Wait for the change to be applied in the interrupt
    while (m_AdcResPending != AdcResolution()) {
        continue;
    }
    return true;
}

bool AdcManager::AdcResChange() {
    switch (m_AdcResPending) {
        case 8:
            ADC1->CTRLB.bit.RESSEL = ADC_CTRLB_RESSEL_8BIT_Val;
            break;
        case 10:
            ADC1->CTRLB.bit.RESSEL = ADC_CTRLB_RESSEL_10BIT_Val;
            break;
        case 12:
            ADC1->CTRLB.bit.RESSEL = ADC_CTRLB_RESSEL_12BIT_Val;
            break;
        // 16 bit is not supported
        default:
            // Invalid value
            return false;
    }

    m_AdcResolution = m_AdcResPending;

    return true;
}

bool AdcManager::FilterTc(AdcChannels adcChannel,
                          uint16_t tc,
                          FilterUnits theUnits) {
    if (adcChannel >= ADC_CHANNEL_COUNT) {
        return false;
    }

    switch (theUnits) {
        case AdcManager::FilterUnits::FILTER_UNIT_RAW:
            m_analogFilter[adcChannel].Tc(tc);
            return true;
        case AdcManager::FilterUnits::FILTER_UNIT_MS:
            m_analogFilter[adcChannel].Tc_ms(tc);
            return true;
        case AdcManager::FilterUnits::FILTER_UNIT_SAMPLES:
            m_analogFilter[adcChannel].TcSamples(tc);
            return true;
        default:
            // Error
            return false;
    }
}

uint16_t AdcManager::FilterTc(AdcChannels adcChannel,
                              FilterUnits theUnits) {
    if (adcChannel >= ADC_CHANNEL_COUNT) {
        return false;
    }

    switch (theUnits) {
        case AdcManager::FilterUnits::FILTER_UNIT_RAW:
            return m_analogFilter[adcChannel].Tc();
        case AdcManager::FilterUnits::FILTER_UNIT_MS:
            return m_analogFilter[adcChannel].Tc_ms();
        case AdcManager::FilterUnits::FILTER_UNIT_SAMPLES:
            return m_analogFilter[adcChannel].TcSamples();
        default:
            // Error
            return 0;
    }
}

#ifdef DEBUG_ADC_RESULT_TIMING

extern "C" void DMAC_0_Handler() {
    // DMAC interrupts TERR TCMPL SUSP
    DmaManager::Channel(DMA_ADC_RESULTS)->CHINTFLAG.reg =
        DMAC_CHINTENCLR_TCMPL; // clear interrupt
}
#endif

} // ClearCore namespace