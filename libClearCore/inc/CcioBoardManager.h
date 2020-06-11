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
    \file CcioBoardManager.h

    This class manages control of the ClearCore I/O (CCIO-8) Expansion Boards
    connected to a serial port on the ClearCore.

    Provides support for CCIO-8 discovery and non-PWM digital I/O on CCIO-8
    board pins.
**/

#ifndef __CCIOBOARDMANAGER_H__
#define __CCIOBOARDMANAGER_H__

#include <stddef.h>
#include <stdint.h>
#include "CcioPin.h"
#include "SerialDriver.h"
#include "SysConnectors.h"
#include "SysTiming.h"

namespace ClearCore {

/** The number of I/O pins on one CCIO-8 Expansion Board. **/
#define CCIO_PINS_PER_BOARD  8
#ifndef MAX_CCIO_DEVICES
/** The maximum number CCIO-8 Expansion Boards that can be chained to the
    ClearCore at any given time. **/
#define MAX_CCIO_DEVICES 8
/**
    The maximum number of CCIO-8 pins that can be addressed at any given time.
**/
#define CCIO_PIN_CNT (CCIO_PINS_PER_BOARD * MAX_CCIO_DEVICES)
#endif

/** The maximum number of times to attempt to flush data through the chain of
    connected  CCIO-8 boards during the discover process before bailing out. **/
#ifndef MAX_FLUSH_ATTEMPTS
#define MAX_FLUSH_ATTEMPTS 4
#endif

/** The maximum number of allowable data glitches to handle during the discover
    process before bailing out. **/
#ifndef MAX_GLITCH_LIM
#define MAX_GLITCH_LIM 4
#endif

/** The number of consecutive samples having the output asserted with the input
    deasserted before the output is flagged as being overloaded. **/
#ifndef CCIO_OVERLOAD_TRIP_TICKS
#define CCIO_OVERLOAD_TRIP_TICKS ((uint8_t)(2.4 * MS_TO_SAMPLES))
#endif
/** The number of samples to force the output to be deasserted when an
    overload condition occurs on that output. **/
#ifndef CCIO_OVERLOAD_FOLDBACK_TICKS
#define CCIO_OVERLOAD_FOLDBACK_TICKS (100 * MS_TO_SAMPLES)
#endif
/**
    \brief ClearCore I/O Expansion Board Manager Class

    This is the manager class for all the CCIO-8 pin connectors. The CCIO-8 link
    state is established and queried through this class, as well as the filter
    setting for all of the CCIO-8 connectors. Each CCIO-8 connector can be
    accessed through this class, in addition to the individual access available
    through the CcioPin class.

    For more detailed information on the CCIO-8 system, check out the \ref
    CCIOMain informational page.
**/
class CcioBoardManager {
    friend class SysManager;
    friend class CcioPin;

public:
#ifndef HIDE_FROM_DOXYGEN
    /**
        Public accessor for singleton instance
    **/
    static CcioBoardManager &Instance();
#endif

    /**
        \brief Read the digital state of the specified CCIO-8 pin.

        \code{.cpp}
        if (CcioMgr.PinState(CLEARCORE_PIN_CCIOA0)) {
            // The state of connector 0 on the first CCIO-8 board is high.
        }
        \endcode

        \param[in] pinNum The CCIO-8 pin number

        \return Returns true if the pin state is TRUE, and false if the pin
                state is FALSE, or if the specified CCIO-8 pin does not exist, or
                if the CcioBoardManager is not yet initialized.
    **/
    bool PinState(ClearCorePins pinNum);

    /**
        \brief Write the digital state to the specified CCIO-8 pin.

        \code{.cpp}
        // Set the first CCIO-8 board's connector 0 output FALSE.
        CcioMgr.PinState(CLEARCORE_PIN_CCIOA0, false);
        \endcode

        \param[in] pinNum The CCIO-8 pin number
        \param[in] newState The digital value to be written to the connector.
    **/
    void PinState(ClearCorePins pinNum, bool newState);

    /**
        \brief Start an output pulse.

        This allows you to start a pulse on a CCIO-8 connector that is on for
        \a onTime milliseconds and off for \a offTime milliseconds and will stop
        after \a pulseCount cycles.
        A \a pulseCount of 0 will cause the pulse to run endlessly.

        If a pulse is already running, calling this will allow you to override
        the previous pulse (after the next change in state).

        \code{.cpp}
        // Begin a 100ms on/200ms off pulse on the first CCIO-8 board's
        // connector 0 output that will complete 20 cycles and prevent further
        // code execution until the cycles are complete.
        CcioMgr.OutputPulsesStart(CLEARCORE_PIN_CCIOA0, 100, 200, 20, true);
        \endcode

        \param[in] pinNum The CCIO-8 pin number.
        \param[in] onTime The amount of time the input will be held on [ms].
        \param[in] offTime The amount of time the input will be held off [ms].
        \param[in] pulseCount (optional) The amount of cycles the pulse will
        run for. Default: 0 (pulse runs endlessly).
        \param[in] blockUntilDone (optional) If true, the function doesn't
        return until the pulses have been sent. Default: false.
    **/
    void OutputPulsesStart(ClearCorePins pinNum, uint32_t onTime,
                           uint32_t offTime, uint16_t pulseCount = 0,
                           bool blockUntilDone = false);
    /**
        \brief Stop an output pulse.

        This allows you to stop the currently running pulse on a CCIO-8 output.
        The output will always be set to FALSE after canceling a pulse.

        \code{.cpp}
        // Stop the active output pulse on the first CCIO-8 board's connector 0.
        CcioMgr.OutputPulsesStop(CLEARCORE_PIN_CCIOA0);
        \endcode

        \param[in] pinNum The CCIO-8 pin number
        \param[in] stopImmediately (optional) If true, the output pulses will
        be stopped immediately; if false, any active pulse will be completed
        first. Default: true.
    **/
    void OutputPulsesStop(ClearCorePins pinNum, bool stopImmediately = true);

    /**
        \brief Check the output pulse state.

        This allows you to see which CCIO-8 pins have active output pulses
        being sent.

        \code{.cpp}
        if (CcioMgr.OutputPulsesActive()) {
            // If there's an output pulse active on any output, do something
        }
        \endcode

        \return A bitmask representing which pins are sending output pulses.
    **/
    volatile const uint64_t &OutputPulsesActive() {
        return m_pulseActive;
    }

    /**
        \brief Polls for and discovers all CCIO-8 boards connected to the
        ClearCore.

        \code{.cpp}
        // Discover and create a CCIO-8 link on COM-1
        CcioMgr.CcioDiscover(&ConnectorCOM1);
        \endcode

        \note Typically this function will not need to be called. Opening a
        SerialDriver port in CCIO mode will automatically discover all CCIO
        connections on that port. This function is only necessary when the Auto-
        Rediscover feature is disabled and a broken CCIO link needs to be
        rebuilt.

        \param[in] comInstance A pointer to the SerialDriver that controls the
                               serial port that the CCIO-8 is plugged into
        \return Number of CCIO-8 devices connected.
    **/
    uint8_t CcioDiscover(SerialDriver *comInstance);

#ifndef HIDE_FROM_DOXYGEN
    /**
        Cleanly closes the CCIO-8 connection.

        \code{.cpp}
        // Kill the CCIO-8 link
        CcioMgr.LinkClose();
        \endcode
    **/
    void LinkClose();
#endif

    /**
        \brief Accessor for the number of CCIO-8 boards connected to the
        ClearCore.

        \code{.cpp}
        // Save how many CCIO-8 boards are in the link
        uint8_t boardCount = CcioMgr.CcioCount();
        \endcode

        \note If the CCIO link is broken CcioCount will return the number of
        CCIO-8 boards in the previously working link. CcioCount will only update
        when a new, healthy CCIO link network is detected.

        \return Number of CCIO-8 devices connected.
    **/
    volatile const uint8_t &CcioCount() {
        return m_ccioCnt;
    }

    /**
        \brief Accessor for the CCIO-8 link status.

        \code{.cpp}
        if (CcioMgr.LinkBroken()) {
            // The link is down, handle it somehow
        }
        \endcode

        \return True if the CCIO-8 link is broken.
    **/
    volatile const bool &LinkBroken() {
        return m_ccioLinkBroken;
    }

    /**
        \brief Accessor for all the CCIO-8 pins' overloaded states.

        \code{.cpp}
        if (CcioMgr.IoOverloadRT()) {
            // There is an overload I/O point, handle it
        }
        \endcode

        \return A bitmask indicating which CCIO-8 pins have asserted
        outputs but the subsequent read of the input is FALSE.
    **/
    volatile const uint64_t &IoOverloadRT() {
        return m_ccioOverloaded;
    }

    /**
        \brief Accessor for all the CCIO-8 pins' accumulated overload states.

        \code{.cpp}
        // Save the I/O points that have overloaded since the previous read
        uint64_t accumOverloads = CcioMgr.IoOverloadAccum();
        \endcode

        \return A bitmask indicating which CCIO-8 pins have been overloaded
        since the last time IoOverloadAccum() was called.
    **/
    uint64_t IoOverloadAccum();

    /**
        \brief Accessor for all the CCIO-8 pins' accumulated overload states.

        \code{.cpp}
        // Save the I/O points that have overloaded since program startup
        uint64_t accumOverloads = CcioMgr.IoOverloadSinceStartupAccum();
        \endcode

        \return A bitmask indicating which CCIO-8 pins have been overloaded
        since the ClearCore has been restarted.
    **/
    volatile const uint64_t &IoOverloadSinceStartupAccum() {
        return m_overloadSinceStartupAccum;
    }

    /**
        \brief Clear on read accessor for rising input edges.

        \code{.cpp}
        if (CcioMgr.InputsRisen()) {
            // An input on a CCIO-8 has risen, react to it.
        }
        \endcode

        \param [in] mask (optional) The CCIO-8 bits to check for rising edges.

        \return CCIO-8 bits that have risen since last poll.
    **/
    uint64_t InputsRisen(uint64_t mask = UINT64_MAX);

    /**
        \brief Clear on read accessor for falling input edges.

        \code{.cpp}
        if (CcioMgr.InputsFallen()) {
            // An input on a CCIO-8 has fallen, react to it.
        }
        \endcode

        \param [in] mask (optional) The CCIO-8 bits to check for falling edges.

        \return CCIO-8 bits that have fallen since last poll.
    **/
    uint64_t InputsFallen(uint64_t mask = UINT64_MAX);

    /**
        \brief Accessor for all the CCIO-8 pins' filtered input states.

        \code{.cpp}
        // Save a reference to the state of all CCIO-8 input points.
        uint64_t iState = CcioMgr.InputState();
        \endcode

        \return A bitmask indicating which CCIO-8 inputs are asserted.
    **/
    volatile const uint64_t &InputState() {
        return m_filteredInputs;
    }

    /**
        \brief Accessor for all the CCIO-8 pins' output states.

        \code{.cpp}
        // Save a reference to the state of all CCIO-8 output points.
        uint64_t oState = CcioMgr.OutputState();
        \endcode

        \return A bitmask indicating which CCIO-8 outputs are asserted.
    **/
    volatile const uint64_t &OutputState() {
        return m_lastOutputs;
    }

    /**
        \brief Enable or Disable the automatic rediscover function

        If the CCIO-8 link is broken the CcioBoardManager can attempt to rebuild
        the link.

        \code{.cpp}
        // Turn off the CCIO-8 rediscover feature
        CcioMgr.CcioRediscoverEnable(false);
        \endcode

        \note This feature is enabled by default.

        \param[in] enable Whether to enable or disable rediscover.
    **/
    void CcioRediscoverEnable(bool enable);

    /**
        \brief Accessor for the individual CCIO-8 pin connectors.

        \code{.cpp}
        if (CcioMgr.PinByIndex(CLEARCORE_PIN_CCIOA0)->State()) {
            // The first CCIO-8 board's connector 0 input is high.
        }
        \endcode

        \param[in] connectorIndex The index of the CCIO-8 pin being requested
        \return A pointer to the corresponding CcioPin Connector object
    **/
    CcioPin *PinByIndex(ClearCorePins connectorIndex);

    /**
        \brief Accessor for the CCIO-8 link refresh rate

        Calculates and returns the refresh rate based on the number
        of CCIO-8 boards currently connected.
    **/
    uint8_t RefreshRate() {
        uint8_t cnt = CcioCount();
        return (cnt > 1) ? (cnt >> 1) : 1;
    }

private:
    union CcioBuf {
        struct __attribute__((packed)) {
            uint8_t writeMarker;
            uint64_t inputs;
            uint64_t outputsSwapped;
            uint8_t readMarker;
        } buf64;
        uint8_t buf8[2 * MAX_CCIO_DEVICES + 2];
        CcioBuf() {
            Clear();
        }
        void Clear() {
            buf64.writeMarker = 0;
            buf64.inputs = 0;
            buf64.outputsSwapped = 0;
            buf64.readMarker = 0;
        };
    };

    typedef enum {
        CCIO_SEARCH,
        CCIO_TEST,
        CCIO_FOUND
    } CcioDiscoverState;

    CcioBuf m_writeBuf;
    CcioBuf m_readBuf;

    // Reference for the discovery state of the CCIO-8 link network
    CcioDiscoverState m_discoverState;

    // Reference to serial port
    SerialDriver *m_serPort;
    // CCIO-8 Device Count
    uint8_t m_ccioCnt;
    // Refresh rate
    uint8_t m_ccioRefreshRate;
    // Refresh delay
    uint8_t m_ccioRefreshDelay;
    // Currently overloaded outputs
    uint64_t m_throttledOutputs;

    // Storage for inputs/outputs (max 64 pins to a serial port)
    // LSB corresponds to 1st pin on 1st CCIO-8 in the chain
    uint64_t m_currentInputs;
    uint64_t m_filteredInputs;
    uint64_t m_currentOutputs;
    uint64_t m_outputMask;
    uint64_t m_lastOutputsSwapped;
    // copy of last outputs sent, prior to any swapping or throttling
    uint64_t m_lastOutputs;
    uint64_t m_outputsWithThrottling;
    uint64_t m_ccioMask;    // mask for active CCIOs

    // Pulse out control variables
    uint64_t m_pulseActive;
    uint64_t m_pulseValue;
    uint64_t m_pulseStopPending;

    uint16_t m_consGlitchCnt;   // count of consecutive glitches detected
    bool m_ccioLinkBroken;
    uint64_t m_ccioOverloaded;
    uint64_t m_ccioOverloadAccum;
    uint64_t m_overloadSinceStartupAccum;
    uint64_t m_inputRegRisen;
    uint64_t m_inputRegFallen;
    uint32_t m_faultLed;
    bool m_autoRediscover;
    uint32_t m_lastDiscoverTime;

    CcioPin m_ccioPins[CCIO_PIN_CNT];

    /**
        Constructor
    **/
    CcioBoardManager();

    /**
        Initializes the CCIO-8 manager and the CcioPin objects.
    **/
    void Initialize();

    /**
        Updates filtered internal state.
    **/
    void Refresh();

    /**
        Slow rate refresh that controls the rediscover.
    **/
    void RefreshSlow();

    /**
        Set the current output overload bits
    **/
    void IoOverloadRT(uint64_t overloadState);

    /*
        Fill a buffer with len bytes of the given val
    */
    static void FillBuffer(uint8_t *buf, uint8_t len, uint8_t val) {
        uint8_t i;
        for (i = 0; i < len; i++) {
            *buf++ = val;
        }
    }

    /*
        Return true if all entries are equal to val
    */
    static bool AllEntriesEqual(const uint8_t *buf, uint8_t len, uint8_t val) {
        uint8_t i;
        for (i = 0; i < len; i++) {
            if (buf[i] != val) {
                return false;
            }
        }
        return true;
    }

}; // CcioBoardManager

} // ClearCore namespace

#endif // __CCIOBOARDMANAGER_H__
