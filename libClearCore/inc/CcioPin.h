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
    \file CcioPin.h
    \brief Connector class for an individual CCIO-8 pin.

    This manages the digital input and output for the CCIO-8 pins.
**/

#ifndef __CCIOPIN_H__
#define __CCIOPIN_H__

#include <sam.h>
#include <stdint.h>

#include "Connector.h"
#include "SysConnectors.h"

namespace ClearCore {

/**
    \class CcioPin
    \brief Connector class for an individual CCIO-8 pin.

    Manages individual CCIO-8 pins.

    For more detailed information on the CCIO-8 system, check out the \ref
    CCIOMain informational page.

    For more detailed information on the ClearCore Connector interface, check
    out the \ref ConnectorMain informational page.
**/
class CcioPin : public Connector {
    friend class CcioBoardManager;

public:

    /**
        \brief Get the connector's operational mode.

        \code{.cpp}
        if (CcioMgr.PinByIndex(CLEARCORE_PIN_CCIOA0)->Mode() ==
                                Connector::INPUT_DIGITAL) {
            // Connector 0 on the first CCIO-8 board is currently configured
            // as a digital input.
        }
        \endcode

        \return The connector's current operational mode.
    **/
    virtual ConnectorModes Mode() override {
        return Connector::Mode();
    }

    /**
        \brief Set the connector's operational mode.

        Configure the connector to operate in a new I/O mode.

        \code{.cpp}
        // Configure the first CCIO-8 board's connector 0 as a digital output.
        CcioMgr.PinByIndex(CLEARCORE_PIN_CCIOA0)->Mode(Connector::OUTPUT_DIGITAL);
        \endcode

        \param[in] newMode The new mode to be set.
        The valid modes for this connector type are:
        - #INPUT_DIGITAL
        - #OUTPUT_DIGITAL.
        \return Returns false if the mode is invalid or setup fails.
    **/
    virtual bool Mode(ConnectorModes newMode) override;

    /**
        \brief Get connector type.

        \code{.cpp}
        if (ConnectorAlias->Type() == CCIO_DIGITAL_IN_OUT_TYPE) {
            // This generic connector variable is a CCIO-8 pin.
        }
        \endcode

        \return The type of the current connector.
    **/
    Connector::ConnectorTypes Type() override {
        return Connector::CCIO_DIGITAL_IN_OUT_TYPE;
    }

    /**
        \brief Get R/W status of the connector.

        \code{.cpp}
        if (CcioMgr.PinByIndex(CLEARCORE_PIN_CCIOA0)->IsWritable()) {
            // The first CCIO-8 board's connector 0 is currently set as an
            // output.
        }
        \endcode

        \return True if in #OUTPUT_DIGITAL mode, false otherwise
    **/
    bool IsWritable() override {
        return m_mode == OUTPUT_DIGITAL;
    }

    /**
        \brief In input mode, get the connector's last filtered
        sampled value. In output mode, get the connector's output state.

        \code{.cpp}
        if (CcioMgr.PinByIndex(CLEARCORE_PIN_CCIOA0)->State()) {
            // The first CCIO-8 board's connector 0 input is high.
        }
        \endcode

        \return The latest filtered value on this connector when in input mode;
        the output state when in output mode.
    **/
    int16_t State() override;
    /**
        \brief Set the output state of the connector.

        This allows you to change the output value of the connector item.

        \code{.cpp}
        // Set the first CCIO-8 board's connector 0 output to high.
        CcioMgr.PinByIndex(CLEARCORE_PIN_CCIOA0)->State(1);
        \endcode

        \param[in] newState The value to be output.
    **/
    bool State(int16_t newState) override;

    /**
        \brief Set the connector's digital filter length in samples. The default
        is 3 samples.

        This will set the length of the filter equal to
        (\a samples * CCIO-8 refresh rate) for this connector.

        Restarts any filtering in progress.

        \code{.cpp}
        // Sets the filter to 10 samples (2ms)
        CcioMgr.PinByIndex(CLEARCORE_PIN_CCIOA0)->FilterLength(10);
        \endcode

        \param[in] samples The number of samples to filter.

        \note One sample time is 200 microseconds.
    **/
    void FilterLength(uint16_t samples) {
        m_filterLength = samples;
        m_filterTicksLeft = samples;
    }

    /**
        \brief Set the connector's digital filter length in ms.

        Restarts any filtering in progress.

        \code{.cpp}
        // Sets the filter to 10ms
        CcioMgr.PinByIndex(CLEARCORE_PIN_CCIOA0)->Filter_ms(10);
        \endcode

        \param[in] len The length of the filter in ms.

        \note The maximum filter time is 13000 milliseconds.
    **/
    void Filter_ms(uint16_t len);

    /**
        \brief Check whether the connector is in a hardware fault state.

        \code{.cpp}
        if (CcioMgr.PinByIndex(CLEARCORE_PIN_CCIOA0)->IsInHwFault()) {
            // The first CCIO-8 board's connector 0 is in a fault state.
        }
        \endcode

        \return Connector is in fault
    **/
    bool IsInHwFault() override;

    /**
        \brief Clear on read accessor for this connector's rising input state.

        \code{.cpp}
        if (CcioMgr.PinByIndex(CLEARCORE_PIN_CCIOA0)->InputRisen()) {
            // The first CCIO-8 board's connector 0 input has transitioned from
            // FALSE to TRUE since the last read.
        }
        \endcode

        \return true if the input has risen since the last call
    **/
    bool InputRisen();

    /**
        \brief Clear on read accessor for this connector's falling input state.

        \code{.cpp}
        if (CcioMgr.PinByIndex(CLEARCORE_PIN_CCIOA0)->InputFallen()) {
            // The first CCIO-8 board's connector 0 input has transitioned from
            // TRUE to FALSE since the last read.
        }
        \endcode

        \return true if the input has fallen since the last call
    **/
    bool InputFallen();

    /**
        \brief Start an output pulse.

        This allows you to start a pulse on the output that is on for \a onTime
        milliseconds and off for \a offTime milliseconds and will stop after
        \a pulseCount cycles.
        A \a pulseCount of 0 will cause the pulse to run endlessly. If a pulse
        is already running, calling this will allow you to override the previous
        pulse (after the next change in state).

        \code{.cpp}
        // Begin a 100ms on/200ms off pulse on the first CCIO-8 board's
        // connector 0 output that will complete 20 cycles and prevent further
        // code execution until the cycles are complete.
        CcioMgr.PinByIndex(CLEARCORE_PIN_CCIOA0)->OutputPulsesStart(100, 200,
                                                                    20, true);
        \endcode

        \param[in] onTime The amount of time the input will be held on [ms].
        \param[in] offTime The amount of time the input will be held off [ms].
        \param[in] pulseCount (optional) The amount of cycles the pulse will run
        for. Default: 0 (pulse runs endlessly).
        \param[in] blockUntilDone (optional) If true, the function doesn't
        return until the pulses have been sent. Default: false.
    **/
    void OutputPulsesStart(uint32_t onTime, uint32_t offTime,
                           uint16_t pulseCount = 0, bool blockUntilDone = false);

    /**
        \brief Stop an output pulse.

        This allows you to stop the currently running pulse on this output. The
        output will always be set to FALSE after canceling a pulse.

        \code{.cpp}
        // Stop the active output pulse on the first CCIO-8's connector 0
        CcioMgr.PinByIndex(CLEARCORE_PIN_CCIOA0)->OutputPulsesStop();
        \endcode

        \param[in] stopImmediately (optional) If true, the output pulses will
        be stopped immediately; if false, any active pulse will be completed
        first. Default: true.
    **/
    void OutputPulsesStop(bool stopImmediately = true);

protected:
#ifndef HIDE_FROM_DOXYGEN
    /**
        Construct the CCIO-8 pin.
    **/
    CcioPin();

    /**
        Initialize hardware and/or internal state.
    **/
    void Initialize(ClearCorePins ccioPin) override;

    /**
        \brief Update connector's state.

        Poll the underlying connector for new state update.

        This is typically called from a timer or main loop to update the
        underlying value.
    **/
    void Refresh() override {}
#endif

private:
    // Port access
    uint64_t m_dataBit;

    // Stability filter
    uint16_t m_filterLength;
    // Set to filter length on input change
    uint16_t m_filterTicksLeft;
    // Consecutive sample filter for tripping overload conditions
    uint16_t m_overloadTripCnt;
    // Overload condition output throttling
    uint16_t m_overloadFoldbackCnt;

    uint32_t m_pulseOnTicks;
    uint32_t m_pulseOffTicks;
    uint32_t m_pulseTicksRemaining;
    uint16_t m_pulseStopCount;
    uint16_t m_pulseCounter;
};

} // ClearCore namespace

#endif // __CCIOPIN_H__