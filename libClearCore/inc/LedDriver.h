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
    \file LedDriver.h
    \brief Connector LED shift register access.

    This class implements the Connector interface to conveniently drive the on-
    board LED located next to the USB port.
**/

#ifndef __LEDDRIVER_H__
#define __LEDDRIVER_H__

#include <stdint.h>
#include "Connector.h"
#include "ShiftRegister.h"

namespace ClearCore {

/**
    \brief ClearCore LED control class

    This class manages access to the LED shift register so LEDs may be
    controlled at the connector level.
**/
class LedDriver : public Connector {
    friend class SysManager;

public:
#ifndef HIDE_FROM_DOXYGEN
    /**
        \brief Default constructor so this connector can be a global and
        constructed by SysManager.

        \note Should not be called by anything other than SysManager.
    **/
    LedDriver() {};

    /**
        \brief Get the connector's operational mode.
        \note The only valid operational mode for this connector
        type is #OUTPUT_DIGITAL.

        \return The connector's current operational mode.
    **/
    virtual ConnectorModes Mode() override {
        return Connector::Mode();
    }

    /**
        \brief Set the connector's operational mode.

        \param[in] newMode The new mode to be set.
        The only valid mode for this connector type is: #OUTPUT_DIGITAL.
        \return Returns false if the mode is invalid or setup fails.
    **/
    bool Mode(ConnectorModes newMode) override {
        return newMode == ConnectorModes::OUTPUT_DIGITAL;
    }
#endif

    /**
        \brief Get connector type.

        \code{.cpp}
        if (ConnectorAlias.Type() == Connector::SHIFT_REG_TYPE) {
            // This generic connector variable is the LedDriver
        }
        \endcode

        \return The type of the current connector.
    **/
    Connector::ConnectorTypes Type() override {
        return Connector::SHIFT_REG_TYPE;
    }

    /**
        \brief Get R/W status of the connector.

        \code{.cpp}
        if (ConnectorAlias.IsWritable()) {
            // This generic connector variable is writable
        }
        \endcode

        \return True because this connector is always writable.
    **/
    bool IsWritable() override {
        return true;
    }

    /**
        \brief Get LED's last sampled state.

        \code{.cpp}
        if (ConnectorLed.State()) {
            // The built-in LED is on
        }
        \endcode

        \return The latest LED state.
    **/
    int16_t State() override;

    /**
        \brief Set the state of the LED.

        \code{.cpp}
        // Turn on the built-in LED
        ConnectorLed.State(1);
        \endcode

        \param[in] newState New state to set for the LED.
    **/
    bool State(int16_t newState) override;

#ifndef HIDE_FROM_DOXYGEN
    bool IsInHwFault() override {
        return false;
    }
#endif

private:
    ShiftRegister::Masks m_ledMask;
    /**
        \brief Update connector's state.

        Poll the underlying connector for new state update.

        This is typically called from a timer or main loop to update the
        underlying value.
    **/
    void Refresh() override {}

    /**
        Initialize hardware and/or internal state.

        \note This class requires no initialization, but requires an
        implementation of Initialize() to be a concrete class.
    **/
    void Initialize(ClearCorePins clearCorePin) override {
        m_clearCorePin = clearCorePin;
        m_mode = OUTPUT_DIGITAL;
    }

    /**
        Construct and wire in LED bit number.
    **/
    explicit LedDriver(ShiftRegister::Masks ledMask);
}; // LedDriver

} // ClearCore namespace

#endif // __LEDDRIVER_H__