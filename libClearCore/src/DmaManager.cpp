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
    DMA Peripheral Manager for the ClearCore Board

    This class manages the DMA transfers on the Teknic ClearCore.
**/

#include "DmaManager.h"
#include <stddef.h>
#include <sam.h>
#include "SysUtils.h"

namespace ClearCore {

// Interrupt priority 0(High) - 7(Low)
#define DMA_COMPLETE_PRIORITY 2

#if (DMA_CHANNEL_COUNT > DMAC_CH_NUM)
#error "Attempting to use more DMA channels than available on the device"
#endif

#ifndef HIDE_FROM_DOXYGEN
/* Active DMA descriptors */
DmacDescriptor DmaManager::writeBackDescriptor[DMA_CHANNEL_COUNT]
__attribute__((aligned(16)));
/* Starting DMA descriptors */
DmacDescriptor DmaManager::descriptorBase[DMA_CHANNEL_COUNT] __attribute__((
            aligned(16)));
#endif

DmaManager &DmaMgr = DmaManager::Instance();

DmaManager &DmaManager::Instance() {
    static DmaManager *instance = new DmaManager();
    return *instance;
}

void DmaManager::Initialize() {
    /***********************************************************
     * DMA peripheral initialization
     ***********************************************************/
    // Enables the peripheral clock to the DMAC
    CLOCK_ENABLE(AHBMASK, DMAC_);

    // Reset the DMAC to start fresh
    DMAC->CTRL.reg = DMAC_CTRL_SWRST;
    // Wait for the reset to finish
    while (DMAC->CTRL.reg == DMAC_CHCTRLA_SWRST) {
        continue;
    }

    /* Disable GPDMA interrupt */
    NVIC_DisableIRQ(DMAC_0_IRQn);
    /* Initialize DMA interrupt priority  */
    NVIC_SetPriority(DMAC_0_IRQn, DMA_COMPLETE_PRIORITY);

    // Tell the DMAC where the descriptors are (must be located in SRAM)
    DMAC->BASEADDR.reg = (uint32_t)descriptorBase;
    DMAC->WRBADDR.reg = (uint32_t)writeBackDescriptor;

    // Enable the DMAC and set the priority
    DMAC->CTRL.reg = DMAC_CTRL_DMAENABLE | DMAC_CTRL_LVLEN(0xf);

    DMAC->DBGCTRL.bit.DBGRUN = 1;

    NVIC_EnableIRQ(DMAC_0_IRQn);

    /***************************************************************
     * DMA channels that will be automatically triggered
     ***************************************************************/
    DMAC->SWTRIGCTRL.reg &=
        ~((1UL << DMA_ADC_SEQUENCE) | (1UL << DMA_ADC_RESULTS) |
          (1UL << DMA_SERCOM0_SPI_TX) | (1UL << DMA_SERCOM0_SPI_RX) |
          (1UL << DMA_SERCOM7_SPI_TX) | (1UL << DMA_SERCOM7_SPI_RX));
}

DmacChannel *DmaManager::Channel(DmaChannels index) {
    if (index >= DMA_CHANNEL_COUNT) {
        return NULL;
    }
    return &DMAC->Channel[index];
}

DmacDescriptor *DmaManager::BaseDescriptor(DmaChannels index) {
    if (index >= DMA_CHANNEL_COUNT) {
        return NULL;
    }
    return &descriptorBase[index];
}

} // ClearCore namespace