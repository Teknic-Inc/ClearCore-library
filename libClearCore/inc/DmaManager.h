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
    \file DmaManager.h
    \brief DMA Peripheral Manager for the ClearCore Board

    This class manages the DMA transfers on the Teknic ClearCore.
**/

#ifndef __DMAMANAGER_H__
#define __DMAMANAGER_H__

#include <stdint.h>
#include <sam.h>

#ifndef HIDE_FROM_DOXYGEN
namespace ClearCore {

/* DMA channels */
typedef enum {
    DMA_ADC_RESULTS,    ///< ADC result data
    DMA_ADC_SEQUENCE,   ///< ADC conversion info
    DMA_SERCOM0_SPI_RX, ///< COM1 SPI streaming input
    DMA_SERCOM0_SPI_TX, ///< COM1 SPI streaming output
    DMA_SERCOM7_SPI_RX, ///< COM0 SPI streaming input
    DMA_SERCOM7_SPI_TX, ///< COM0 SPI streaming output
    DMA_CHANNEL_COUNT,  // Keep at end
    DMA_INVALID_CHANNEL // Placeholder for unset values
} DmaChannels;

/**
    \brief DMA Peripheral Manager for the ClearCore Board

    This class manages the DMA transfers on the Teknic ClearCore.
**/
class DmaManager {
    friend class SysManager;

public:
    static DmacChannel *Channel(DmaChannels index);
    static DmacDescriptor *BaseDescriptor(DmaChannels index);

    /**
        Public accessor for singleton instance
    **/
    static DmaManager &Instance();
private:

    static DmacDescriptor writeBackDescriptor[DMA_CHANNEL_COUNT] __attribute__((
                aligned(16)));
    static DmacDescriptor descriptorBase[DMA_CHANNEL_COUNT] __attribute__((aligned(
                16)));

    /**
        \brief Constructor for DmaManager

        Initializes member variables, doesn't do any work.

        \note Should not be called by anything other than SysManager
    **/
    DmaManager() {};

    /**
        \brief One-time initialization of the DMAC

        Enables Peripheral clock and configures DMAC (Direct Memory Access
        Controller).
    **/
    static void Initialize();
}; // DmaManager

} // ClearCore namespace

#endif // HIDE_FROM_DOXYGEN
#endif // __DMAMANAGER_H__