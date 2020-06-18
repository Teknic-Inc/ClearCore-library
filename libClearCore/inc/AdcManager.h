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
    \file AdcManager.h
    \brief ADC Peripheral Manager for the ClearCore Board

    This class manages the ADC conversions on the Teknic ClearCore.
**/

#ifndef __ADCMANAGER_H__
#define __ADCMANAGER_H__

#include <stdint.h>
#include "IirFilter.h"

namespace ClearCore {

/**
    \brief ADC Peripheral Manager for the ClearCore Board

    This class manages the ADC peripheral on the Teknic ClearCore.

    Utilizes DMA sequence to configure channels, trigger conversion, and
    read results.

    Uses DMAC Channels: 0,1
**/
class AdcManager {
public:
    /**
        \enum AdcChannels

        ADC conversion channels.

        \note These are currently assumed to be unsigned values.
    **/
    typedef enum {
        ADC_VSUPPLY_MON = 0, ///< Supply voltage monitor
        ADC_AIN12,           ///< Analog input A-12
        ADC_5VOB_MON,        ///< 5V off-board monitor
        ADC_AIN11,           ///< Analog input A-11
        ADC_AIN10,           ///< Analog input A-10
        ADC_AIN09,           ///< Analog input A-9
        ADC_SDRVR3_IMON,     ///< Screwdriver M-3 current monitor
        ADC_SDRVR2_IMON,     ///< Screwdriver M-2 current monitor
        ADC_CHANNEL_COUNT // Keep at end
    } AdcChannels;

    /**
        \enum FilterUnits
        \brief Units for the filter time constant.

        \note One sample time is 200 microseconds, so 1 ms = 5 sample times.
    **/
    typedef enum {
        /** Raw units. **/
        FILTER_UNIT_RAW,
        /** Milliseconds. **/
        FILTER_UNIT_MS,
        /** Sample times. **/
        FILTER_UNIT_SAMPLES,
    } FilterUnits;

    /**
        The default resolution of the ADC, in bits.
    **/
    static const uint8_t ADC_RESOLUTION_DEFAULT = 12;
    /**
        The default timeout for waiting on an ADC conversion, in samples.
    **/
    static const uint8_t ADC_TIMEOUT_DEFAULT = 3;
    /**
        The default ADC filter time constant, in milliseconds.
    **/
    static const uint32_t ADC_IIR_FILTER_TC_MS = 2;

#ifndef HIDE_FROM_DOXYGEN
    // Max voltage that a channel can read.
    // This will probably need a scale rework once populated
    // Calculated by multiplying the ratio by the ADC supply which is
    // 3.3v
    static constexpr float ADC_CHANNEL_MAX_FLOAT[ADC_CHANNEL_COUNT] = {
        80.85,      // ADC_VSUPPLY_MON
        10.0,       // ADC_AIN12
        6.6,        // ADC_5VOB_MON
        10.0,       // ADC_AIN11
        10.0,       // ADC_AIN10
        10.0,       // ADC_AIN09
        10.0,       // ADC_SDRVR3_IMON
        10.0        // ADC_SDRVR2_IMON
    };

    /**
        \brief One-time initialization of the ADC.

        Enables Peripheral clock, configures multiplexers,
        configures DMAC (Direct Memory Access Controller),
        and configures ADC module.
    **/
    void Initialize();

    /**
        \brief Updates the ADC Manager.

        If a conversion has been performed, restarts the DMA sequence.
        DMA sequence performs all of the actual work to setup channels,
        trigger conversions, and read results.

        \note: Should be called at the ClearCore sample rate.
    **/
    void Update();
#endif

    /**
        \brief Configure the ADC conversion resolution.

        ADC naturally supports 12-bit operation. Lower resolution is used
        to decrease sample time.

        Passing an unsupported resolution yields no change and returns false.

        Acceptable Inputs:
        - 8-bit
        - 10-bit
        - 12-bit

        \note The default resolution is 12-bit.

        \code{.cpp}
        // Set the ADC resolution to 10-bit
        AdcMgr.AdcResolution(10);
        \endcode

        \param resolution Bit resolution. Acceptable values: 8, 10, and 12.

        \return Success
    **/
    bool AdcResolution(uint8_t resolution);

    /**
        \brief Returns the resolution of the ADC.

        Possible values are: 8, 10, and 12.

        \code{.cpp}
        // Save the ADC resolution
        uint8_t currentResolution = AdcMgr.AdcResolution();
        \endcode
    **/
    volatile const uint8_t &AdcResolution() {
        return m_AdcResolution;
    }

    /**
        \brief Returns the filtered ADC result of a specific channel.

        \code{.cpp}
        // Save the filtered ADC reading of analog input A-10.
        uint16_t result = AdcMgr.FilteredResult(AdcManager::ADC_AIN10);
        \endcode

        \param[in] adcChannel ADC channel to get the filtered results of.
        \return Filtered result.

        \note For performance reasons, does not perform any bounds checking.
    **/
    volatile const uint16_t &FilteredResult(AdcChannels adcChannel) {
        return m_AdcResultsConvertedFiltered[adcChannel];
    }

    /**
        \brief Returns the converted ADC result of a specific channel.

        \code{.cpp}
        // Save the converted ADC reading of analog input A-10.
        uint16_t result = AdcMgr.ConvertedResult(AdcManager::ADC_AIN10);
        \endcode

        \param[in] adcChannel ADC channel to get the results of.
        \return Converted result.

        \note For performance reasons, does not perform any bounds checking.
    **/
    volatile const uint16_t &ConvertedResult(AdcChannels adcChannel) {
        return m_AdcResultsConverted[adcChannel];
    }

    /**
        \brief Sets the IIR filter time constant for an ADC channel.

        \code{.cpp}
        // Set the filter time constant of A-10 to 15ms and save its success
        // status.
        bool succeeded = AdcMgr.FilterTc(AdcManager::ADC_AIN10, 15,
                                            AdcManager::FILTER_UNIT_MS);
        \endcode

        \param[in] adcChannel ADC channel to set filter constant for
        \param[in] tc The new filter time constant (99% step change time)
        \param[in] theUnits The units of the time constant \a tc.
        See #FilterUnits.
        \return Success.
    **/
    bool FilterTc(AdcChannels adcChannel, uint16_t tc, FilterUnits theUnits);

    /**
        \brief Gets the IIR filter time constant of an ADC channel.

        \code{.cpp}
        // Save the time constant in ms for A-10.
        uint16_t tc = AdcMgr.FilterTc(AdcManager::ADC_AIN10,
                                        AdcManager::FILTER_UNIT_MS);
        \endcode

        \param[in] adcChannel ADC channel to get filter constant for.
        \param[in] theUnits The units of the returned time constant.
        See #FilterUnits.

        \return Time constant value in the specified units, or 0 if the
        ADC channel is invalid.
    **/
    uint16_t FilterTc(AdcChannels adcChannel, FilterUnits theUnits);

    /**
        \brief Resets the filter for an ADC Channel.

        \code{.cpp}
        // Resets the filter for A-10 and saves the success status.
        bool success = AdcMgr.FilterReset(AdcManager::ADC_AIN10, 10);
        \endcode

        \param[in] adcChannel ADC Channel filter to reset.
        \param[in] newSetting The initial filter value.
        \return Success.
    **/
    bool FilterReset(AdcChannels adcChannel, uint16_t newSetting) {
        if (adcChannel >= ADC_CHANNEL_COUNT) {
            return false;
        }
        m_analogFilter[adcChannel].Reset(newSetting);
        m_AdcResultsConvertedFiltered[adcChannel] =
            m_analogFilter[adcChannel].LastOutput();
        return true;
    }

    /**
        \brief Configure the ADC conversion timeout.

        The ADC will post an error to the StatusReg if it is unable to
        complete a conversion within the number of samples specified by the
        input of this function.

        \code{.cpp}
        // Sets the timeout to 10 samples
        AdcMgr.AdcTimeoutLimit(10);
        \endcode

        \param[in] timeout The length of the timeout in samples.
    **/
    void AdcTimeoutLimit(uint8_t timeout) {
        m_AdcTimeoutLimit = timeout;
    }

    /**
        \brief Determine whether the ADC has timed out.

        \code{.cpp}
        if (AdcMgr.AdcTimeout()) {
            // The ADC has timed out
        }
        \endcode

        \return true if the ADC has time out, false otherwise.
    **/
    volatile const bool &AdcTimeout() {
        return m_AdcTimeout;
    }

    /**
        \brief Returns the filtered ADC result of a specific channel in volts.

        \code{.cpp}
        // Save the filtered voltage reading on A-10.
        float a10Volts = AdcMgr.AnalogVoltage(AdcManager::ADC_AIN10);
        \endcode

        \param[in] adcChannel ADC channel to get the voltage reading of.
        \return Voltage on the given ADC channel.

        \note For performance reasons, does not perform any bounds checking.
    **/
    float AnalogVoltage(AdcChannels adcChannel) {
        uint16_t maxReading = INT16_MAX & ~(INT16_MAX >> m_AdcResolution);
        float voltage = ADC_CHANNEL_MAX_FLOAT[adcChannel] *
                        m_AdcResultsConvertedFiltered[adcChannel] / maxReading;
        return voltage;
    }

#ifndef HIDE_FROM_DOXYGEN
    /**
        Public accessor for the AdcManager singleton instance.
    **/
    static AdcManager &Instance();

    /**
        \brief Get a copy of the shift register state last written to the SPI
        data register.

        \return The last-written shift register state.
    **/
    volatile const uint32_t &ShiftRegSnapshot() {
        return m_shiftRegSnapshot;
    }
#endif
private:

    /**
        Used to populate filters with initial values. Float to make human
        readable. Conversion to ADC counts only happens once during init so
        performance loss is minimal for the readability.
    **/
    static constexpr float ADC_INITIAL_FILTER_VALUE_V[ADC_CHANNEL_COUNT] = {
        24.0, // ADC_VSUPPLY_MON
        0.0,  // ADC_AIN12
        5.0,  // ADC_5VOB_MON
        0.0,  // ADC_AIN11
        0.0,  // ADC_AIN10
        0.0,  // ADC_AIN09
        0.0,  // ADC_SDRVR3_IMON
        0.0,  // ADC_SDRVR2_IMON
    };

    // ADC state holders in Q15. ADC logic has already been performed
    volatile uint16_t m_AdcResultsConverted[ADC_CHANNEL_COUNT] = {0};
    volatile uint16_t m_AdcResultsConvertedFiltered[ADC_CHANNEL_COUNT] = {0};
    Iir16 m_analogFilter[ADC_CHANNEL_COUNT];

    bool m_initialized;

    bool m_AdcTimeout;
    uint32_t m_shiftRegSnapshot;
    uint32_t m_shiftRegPending;

    /** Resolution of the ADC interface **/
    uint8_t m_AdcResolution;
    uint8_t m_AdcResPending;

    /** ADC Conversion timeout. Timeout will trip if conversion is not done in
        this number of samples.  **/
    uint8_t m_AdcTimeoutLimit;

    /** Count of samples since last ADC conversion. **/
    uint32_t m_AdcBusyCount;

    /**
        \brief Constructor for AdcManager.

        Initializes member variables, doesn't do any work.

        \note Should not be called by anything other than SysManager.
    **/
    AdcManager();

    void DmaInit();
    void DmaUpdate();

    /**
        \brief Apply the ADC conversion resolution change.

        Called from within the main interrupt to apply pending
        resolution changes when the ADC conversion is idle.

        \return Success
    **/
    bool AdcResChange();

}; // AdcManager

} // ClearCore namespace

#endif // __ADCMANAGER_H__
