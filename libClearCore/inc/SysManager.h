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
    \file SysManager.h
    Supervisory System Manager for the ClearCore Board

    This class implements the high level access to the Teknic ClearCore
    features.
**/

#ifndef __SYSMANAGER_H__
#define __SYSMANAGER_H__

#include <stddef.h>
#include <stdint.h>
#include "Connector.h"
#include "PeripheralRoute.h"
#include "SysConnectors.h"

#ifdef __cplusplus

/**
    \namespace ClearCore

    \brief Namespace to encompass the ClearCore board API
**/
namespace ClearCore {

#ifndef HIDE_FROM_DOXYGEN
typedef void (*voidFuncPtr)(void);
#endif

/**
    \brief ClearCore Board Supervisory System Manager

    This class manages the features of the Teknic ClearCore board.
**/
class SysManager {

public:
    /**
        \enum ResetModes
        \brief Reset modes for the ClearCore board.
    **/
    typedef enum {
        /**
            Reset and restart normal program execution.
        **/
        RESET_NORMAL,
        /**
            Reset and start up in the bootloader.
        **/
        RESET_TO_BOOTLOADER,
    } ResetModes;

#ifndef HIDE_FROM_DOXYGEN
    /**
        Constructor
    **/
    SysManager();

    /**
        Initialize the board to power-up state
    **/
    void Initialize();

    /**
        \brief Accessor for SysManager's readiness

        \return True if the board is initialized and ready for
        operations.
    **/
    bool Ready() {
        return m_readyForOperations;
    }
#endif

    /**
        \brief Pin-determined accessor for ClearCore Connector objects.

        \code{.cpp}
        // Save and cast a pointer to the IO-3 connector object.
        // Be sure to cast the base Connector pointer to the correct type!
        DigitalInOut *io3 =
            static_cast<DigitalInOut*>(SysMgr.ConnectorByIndex(CLEARCORE_PIN_IO3));
        \endcode

        \param[in] connectorIndex The index in the connector array
        \return A pointer to a connector. If the connector is out of
        range, a NULL pointer is returned.
    **/
    Connector *ConnectorByIndex(ClearCorePins connectorIndex);

    /**
        \brief Resets the ClearCore.

        \code{.cpp}
        // Reset the ClearCore and restart the application from the beginning.
        SysMgr.ResetBoard();
        \endcode
        \param[in] mode The reset mode. Valid reset modes are:
        - #RESET_NORMAL
        - #RESET_TO_BOOTLOADER.
    **/
    void ResetBoard(ResetModes mode = RESET_NORMAL);

#ifndef HIDE_FROM_DOXYGEN
    // Ideally these would be private, but they need to be called from C
    // interrupt handler functions that can't be friends without putting them
    // in a namespace.
    void SysTickUpdate();
    void FastUpdate();
#endif

private:
    /// Flag to defer operations until initialized.
    bool m_readyForOperations;

    /**
        Initialize the clock rates and interrupts.
    **/
    void InitClocks();

    /**
        Update systems at #SampleRateHz.
    **/
    void UpdateFastImpl();

    /**
        Update systems at SysTick rate.
    **/
    void UpdateSlowImpl();

};     // SysManager

} // ClearCore namespace

#endif // defined(__cplusplus)
#endif // __SYSMANAGER_H__
