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

#ifndef __STATUSMANAGER_H__
#define __STATUSMANAGER_H__

#include "ShiftRegister.h"
#include "BlinkCodeDriver.h"
#include "SysConnectors.h"

namespace ClearCore {

/**
    \brief ClearCore Status Register Manager class

    This class manages access to ClearCore status information.
**/
class StatusManager {
    friend class CcioBoardManager;
    friend class DigitalInOut;

public:
    /**
        \union StatusRegister

        The ClearCore status register.
    **/
    union StatusRegister {
        /**
            Broad access to the whole register
        **/
        uint32_t reg;

        /**
            Field access to the status register
        **/
        struct {
            /**
                Supply voltage has exceeded 29V, outside of the range of
                normal operating conditions (nominally 24 V).
            **/
            uint32_t VSupplyOverVoltage    : 1;
            /**
                Supply voltage has gone below 10V, outside of the range of
                normal operating conditions (nominally 24V).
            **/
            uint32_t VSupplyUnderVoltage   : 1;
            /**
                The H-Bridge chip on connectors IO-4 and IO-5 has experienced
                an overload condition.
            **/
            uint32_t HBridgeOverloaded     : 1;
            /**
                The H-Bridge chip on connectors IO-4 and IO-5 is currently
                undergoing a hardware reset.
            **/
            uint32_t HBridgeReset          : 1;
            /**
                The 5V off-board supply has gone below 4V, outside of the range
                of normal operating conditions (nominally 5V).
            **/
            uint32_t Overloaded5V          : 1;
            /**
                An output is currently overloaded on the ClearCore board
                (driven TRUE but being pulled FALSE).
            **/
            uint32_t OutputOverloaded      : 1;
            /**
                An output is currently overloaded on an attached CCIO-8 board
                (driven TRUE but being pulled FALSE).
            **/
            uint32_t CcioOverloaded        : 1;
            /**
                An established CCIO-8 link has gone offline.
            **/
            uint32_t CcioLinkBroken        : 1;
            /**
                A conversion in the analog-to-digital converter has timed out.
            **/
            uint32_t AdcTimeout            : 1;
            /**
                The Ethernet cable is unplugged.
            **/
            uint32_t EthernetDisconnect    : 1;
            /**
                A remote Ethernet error has occurred.
            **/
            uint32_t EthernetRemoteFault   : 1;
            /**
                Ethernet initialization was attempted but failed.
            **/
            uint32_t EthernetPhyInitFailed : 1;
            /**
                The SD card is currently in a hardware fault state.
            **/
            uint32_t SdCardError           : 1;
            /**
                The last NVM write has not yet synchronized or was unable to
                synchronize.
            **/
            uint32_t NvmDesync      : 1;
        } bit;

        /**
            Status Register default constructor
        **/
        StatusRegister() {
            reg = 0;
        }

        /**
            Status Register constructor with initial value
        **/
        StatusRegister(uint32_t val) {
            reg = val;
        }

        /**
            Interpret the StatusRegister as a boolean by reporting whether any
            bits are set.
        **/
        operator bool() const {
            return reg > 0;
        }
    };

#ifndef HIDE_FROM_DOXYGEN
    /**
        Public accessor for singleton instance
    **/
    static StatusManager &Instance();
#endif

    /**
        \brief The real time status register.

        The bits that are asserted in the status register that's returned
        indicate events that are occurring now.

        \note This register shows a real-time view of the ClearCore's status.
        Some of the status bits contained within are of a transient nature and
        will not persist for multiple samples. This means that your polling of
        this register may miss certain status events. To catch these events poll
        the StatusRisen and StatusFallen registers.

        \code{.cpp}
        // Save the current ADC timeout status as a bool.
        bool adcTimedOut = StatusMgr.StatusRT().bit.AdcTimeout;
        if (adcTimedOut) {
            // The ADC has timed out.
        }
        \endcode

        \param [in] mask (optional) A StatusRegister whose asserted bits
        indicate which of the ClearCore status bits to check for an asserted
        state. If one of the \a bit members of this mask are deasserted, that
        bit will be ignored when checking for asserted status bits. If no
        \a mask is provided, it's equivalent to passing a StatusRegister with
        all bits asserted, in which case this function would report any status
        bits that are currently asserted.

        \return StatusRegister whose asserted bits indicate currently active
        ClearCore status events.
    **/
    StatusRegister StatusRT(StatusRegister mask = UINT32_MAX);

    /**
        \brief Clear on read accessor for status bits that have risen
        (transitioned from deasserted to asserted) sometime since the previous
        invocation of this function.

        \code{.cpp}
        // Check if any power supply problems have occurred since the last call
        StatusManager::StatusRegister mask;
        mask.bit.VSupplyOverVoltage = 1;
        mask.bit.VSupplyUnderVoltage = 1;
        StatusManager::StatusRegister risen = StatusMgr.StatusRisen(mask);
        if (risen.reg) {
            // Power supply problems have occurred since last call.
        }
        \endcode

        \param [in] mask (optional) A StatusRegister whose asserted bits
        indicate which of the ClearCore status bits to check for rising edges.
        If one of the \a bit members of this mask are deasserted, that bit will
        be ignored when checking for rising edges. If no \a mask is provided,
        it's equivalent to passing a StatusRegister with all bits asserted, in
        which case this function would report rising edges on any of the status
        bits.

        \return StatusRegister whose asserted bits indicate which ClearCore
        status bits have risen since the last poll.
    **/
    StatusRegister StatusRisen(StatusRegister mask = UINT32_MAX);

    /**
        \brief Clear on read accessor for status bits that have fallen
        (transitioned from asserted to deasserted) sometime since the previous
        invocation of this function.

        \code{.cpp}
        // Check if any power supply problems have gone away since the last call
        StatusManager::StatusRegister mask;
        mask.bit.VSupplyOverVoltage = 1;
        mask.bit.VSupplyUnderVoltage = 1;
        StatusManager::StatusRegister fallen = StatusMgr.StatusFallen(mask);
        if (fallen.reg) {
            // Power supply problems have gone away since last call
        }
        \endcode

        \param [in] mask (optional) A StatusRegister whose asserted bits
        indicate which of the ClearCore status bits to check for falling edges.
        If one of the \a bit members of this mask are deasserted, that bit will
        be ignored when checking for falling edges. If no \a mask is provided,
        it's equivalent to passing a StatusRegister with all bits asserted, in
        which case this function would report falling edges on any of the status
        bits.

        \return StatusRegister whose asserted bits indicate which ClearCore
        status bits have fallen since the last poll.
    **/
    StatusRegister StatusFallen(StatusRegister mask = UINT32_MAX);

    /**
        \brief Accumulating Clear on read accessor for any status bits that were
        asserted sometime since the previous invocation of this function.

        \note This is similar to #StatusRisen() except that it tracks asserted
        status bits rather than status bits that have transitioned from
        deasserted to asserted.
        Therefore in the case that the supply voltage has been low since
        startup, i.e. its status bit has been set since startup, calling this
        function repeatedly will show that bit asserted each time, while calling
        #StatusRisen() repeatedly will only show the bit asserted on the first
        call, since it transitioned from deasserted to asserted only once at
        startup.

        \code{.cpp}
        // See if any CCIO-8 link errors have occurred since the last call
        bool ccioConnectionProb = StatusMgr.StatusAccum().bit.CcioLinkBroken;
        if (ccioConnectionProb) {
            // The CCIO-8 link is broken; do something about it here.
        }
        \endcode

        \param [in] mask (optional) A StatusRegister whose asserted bits
        indicate which of the ClearCore status bits to check for an asserted
        state. If one of the \a bit members of this mask are deasserted, that
        bit will be ignored when checking for asserted status bits. If no
        \a mask is provided, it's equivalent to passing a StatusRegister with
        all bits asserted, in which case this function would report any
        accumulated asserted status bits.

        \return StatusRegister whose asserted bits indicate which ClearCore
        status bits have been asserted since the last poll.
    **/
    StatusRegister StatusAccum(StatusRegister mask = UINT32_MAX);

    /**
        \brief Access to all accumulated status bits that have asserted since
        board startup (or since the last board reset).

        \note This is not a clear on read operation, so reading this register
        does not automatically clear out the bits that have been raised,
        unlike #StatusRisen() and #StatusFallen().

        \code{.cpp}
        // See if any CCIO-8 link errors have occurred since startup.
        bool ccioConnectionProb =
            StatusMgr.SinceStartupAccum().bit.CcioLinkBroken;
        if (ccioConnectionProb) {
            // The CCIO-8 link is broken; do something about it here.
        }
        \endcode

        \param [in] mask (optional) A StatusRegister whose asserted bits
        indicate which of the ClearCore status bits to check for an asserted
        state. If one of the \a bit members of this mask are deasserted, that
        bit will be ignored when checking for asserted status bits. If no
        \a mask is provided, it's equivalent to passing a StatusRegister with
        all bits asserted, in which case this function would report any
        accumulated asserted status bits.

        \return StatusRegister whose asserted bits indicate which ClearCore
        status bits have risen since startup.
    **/
    StatusRegister SinceStartupAccum(StatusRegister mask = UINT32_MAX);

    /**
        Deactivate a blink code.

        Clear out a currently active blink code. This is useful for when an
        error can be fixed on the fly, while the board is still operating,
        to avoid being continually alerted to the error even after it was
        resolved.

        \param[in] group The group number the blink code belongs to.
        \param[in] code The blink code number to clear.

        \code{.cpp}
        // Check for an historical overload on connector IO-0.
        if (StatusMgr.IoOverloadAccum().bit.CLEARCORE_PIN_IO0) {
            // Clear out the reporting of the overload (blink code 1-1)
            StatusMgr.BlinkCodeClear(1, 1);
        }
        \endcode

        See \ref BlinkCodes for group and code numbers.
    **/
    void BlinkCodeClear(uint8_t group, uint8_t code) {
        m_blinkMgr.BlinkCodeClear(group, code);
    }

    /**
        \brief Starts a reset pulse to the DigitalInOutHBridge connectors.

        Resetting the HBridge will temporarily disable the DigitalInOutHBridge
        and MotorDriver connectors.

        \code{.cpp}
        // Reset the HBridge connectors.
        StatusMgr.HBridgeReset();
        \endcode

        \note Any active step and direction moves on the MotorDriver
        connectors will be terminated.
    **/
    void HBridgeReset();

#ifndef HIDE_FROM_DOXYGEN
    /**
        Initializes the StatusManager.
    **/
    bool Initialize(ShiftRegister::Masks faultLed);

    /**
        Refreshes the StatusManager.
    **/
    void Refresh();

    /**
        \brief Helper to set the state of the DigitalInOutHBridge connectors
        during reset.
    **/
    void HBridgeState(bool reset);
#endif

    /**
        \brief Read accessor for whether the ADC has timed out while attempting
        a conversion.

        \code{.cpp}
        if (StatusMgr.AdcIsInTimeout()) {
            // ADC has timed out.
        }
        \endcode

        \return True if the ADC is currently timed out, false otherwise.
    **/
    bool AdcIsInTimeout();

    /**
        \brief Accessor for the real time overload status of the I/O connectors.

        \code{.cpp}
        if (StatusMgr.IoOverloadRT().bit.CLEARCORE_PIN_IO3) {
            // IO-3 is currently overloaded.
        }
        \endcode

        \param [in] mask (optional) A SysConnectorState whose asserted bits
        indicate which of the ClearCore connector overload status bits to
        check for an asserted state. If one of the \a bit members of this mask
        are deasserted, that bit will be ignored when checking for connector
        overloads. If no \a mask is provided, it's equivalent to passing a
        SysConnectorState with all bits asserted, in which case this function
        would report any connectors currently in an overloaded state.

        \return SysConnectorState register whose asserted bits indicate which
        connectors are currently overloaded.
    **/
    SysConnectorState IoOverloadRT(SysConnectorState mask = UINT32_MAX);

    /**
        \brief Clear on read accessor for connector overload status since the
        last invocation of this function.

        \code{.cpp}
        if (StatusMgr.IoOverloadAccum().reg) {
            // There has been a connector overload since the last call to
            // IoOverloadAccum().
        }
        \endcode

        \param [in] mask (optional) A SysConnectorState whose asserted bits
        indicate which of the ClearCore connector overload status bits to check
        for an asserted state. If one of the \a bit members of this mask are
        deasserted, that bit will be ignored when checking for connector
        overloads. If no \a mask is provided, it's equivalent to passing a
        SysConnectorState with all bits asserted, in which case this function
        would report any connectors in an overload state since the last
        invocation of this function.

        \return SysConnectorState register whose asserted bits indicate which
        connectors have been overloaded since the last poll.
    **/
    SysConnectorState IoOverloadAccum(SysConnectorState mask = UINT32_MAX);

    /**
        \brief Accessor for connector overload status since startup (or board
        reset).

        \code{.cpp}
        if (StatusMgr.IoOverloadSinceStartupAccum().bit.CLEARCORE_PIN_IO3) {
            // Connector IO-3 has overloaded since startup.
        }
        \endcode

        \code{.cpp}
        if (StatusMgr.IoOverloadSinceStartupAccum().bit.CLEARCORE_PIN_IO2) {
            // Connector IO-2 was overloaded since startup.
        }
        \endcode

        \param [in] mask (optional) A SysConnectorState whose asserted bits
        indicate which of the ClearCore connector overload status bits to check
        for an asserted state. If one of the \a bit members of this mask are
        deasserted, that bit will be ignored when checking for connector
        overloads. If no \a mask is provided, it's equivalent to passing a
        SysConnectorState with all bits asserted, in which case this function
        would report any connectors that have been in an overload state since
        board startup.

        \return SysConnectorState register whose asserted bits indicate which
        connectors have been overloaded since startup.
    **/
    SysConnectorState IoOverloadSinceStartupAccum(SysConnectorState mask =
                UINT32_MAX);

    /**
        Activate an application driven blink code.

        This function allows the application code to display a blink code
        in the #BLINK_GROUP_APPLICATION code group.

        \code{.cpp}
        // If an error code is present, display the corresponding blink code.
        uint8_t errorCode = 3;
        if (errorCode) {
            StatusMgr.UserBlinkCode(1 << (errorCode-1));
        }
        \endcode

        \param[in] mask A mask representing an aggregate of blink codes to
        display.
    **/
    void UserBlinkCode(uint8_t mask) {
        m_blinkMgr.CodeGroupAdd(
            BlinkCodeDriver::BLINK_GROUP_APPLICATION, mask);
    }

private:
    StatusRegister m_statusRegSinceStartup;
    StatusRegister m_statusRegRT;
    StatusRegister m_statusRegAccum;
    StatusRegister m_statusRegRisen;
    StatusRegister m_statusRegFallen;

    SysConnectorState m_overloadSinceStartup;
    SysConnectorState m_overloadAccum;
    SysConnectorState m_overloadRT;

    ShiftRegister::Masks m_faultLed;
    BlinkCodeDriver m_blinkMgr;

    bool m_disableMotors;
    volatile bool m_hbridgeResetting;

    StatusManager()
        : m_statusRegSinceStartup(),
          m_statusRegRT(),
          m_statusRegAccum(),
          m_statusRegRisen(),
          m_statusRegFallen(),
          m_faultLed(ShiftRegister::SR_NO_FEEDBACK_MASK),
          m_disableMotors(false),
          m_hbridgeResetting(false) {}

    /**
        Activate a blink code.

        \param[in] group The group the blink code belongs to.
        See BlinkCodeDriver#BlinkCodeGroups.
        \param[in] mask A mask representing an aggregate of blink codes to
        display. See BlinkCodeDriver#SupplyErrorCodes,
        BlinkCodeDriver#DeviceErrors, and BlinkCodeDriver#CcioOverload.
    **/
    void BlinkCode(BlinkCodeDriver::BlinkCodeGroups group, uint8_t mask);

    /**
        Update the board's blink codes with the supplied status
        information.

        \param[in] status A status register with error status information
        to determine which blink codes to display.
    **/
    void UpdateBlinkCodes(StatusRegister status);

    /**
        Set or clear a connector's overload state.
    **/
    void OverloadUpdate(uint32_t mask, bool inFault);

}; // StatusManager

} // ClearCore namespace
#endif // __STATUSMANAGER_H__
