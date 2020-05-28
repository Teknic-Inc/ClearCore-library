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
    \file SysUtils.h
    \brief ClearCore common utility functions
**/

#ifndef __SYSUTILS_H__
#define __SYSUTILS_H__

#include <stdint.h>

// A    EIC
// B    REF
//      ADC
//      AC
//      DAC
//      PTC
// C    SERCOM
// D    SERCOM_ALT
// E    TC
// F    TCC
// G    TCC/PDEC
// H    USB
// I    SDHC
// J    I2S
// K    PCC
// L    GMAC
// M    GCLK/AC
// N    CCL

/** Peripheral type **/
typedef enum {
    /* The pin is controlled by the assoc. signal of peripheral... */
    PER_EXTINT = 0, /* A. */
    PER_ANALOG,     /* B. */
    PER_SERCOM,     /* C. */
    PER_SERCOM_ALT, /* D. */
    PER_TIMER,      /* E. */
    PER_TIMER_ALT,  /* F. */
    PER_TIMER_PDEC, /* G. */
    PER_USB,        /* H. */
    PER_SDHC,       /* I. */
    PER_I2S,        /* J. */
    PER_PCC,        /* K. */
    PER_GMAC,       /* L. */
    PER_GCLK_AC,    /* M. */
    PER_CCL,        /* N. */
} PerType;

/**
    Set the correct peripheral multiplexer for the specified pin in the
    specified port.
 **/
#define PMUX_SELECTION(GPIO_PORT, GPIO_PIN, PER_TYPE)                          \
if ((GPIO_PIN) & 1) {                                                          \
    PORT->Group[(GPIO_PORT)].PMUX[(GPIO_PIN) >> 1].bit.PMUXO = (PER_TYPE);     \
}                                                                              \
else {                                                                         \
    PORT->Group[(GPIO_PORT)].PMUX[(GPIO_PIN) >> 1].bit.PMUXE = (PER_TYPE);     \
}

/**
    Write the data mask to the Data Output Value register on the specified port.
**/
#define DATA_OUTPUT_STATE(GPIO_PORT, DATA_MASK, STATE)                         \
if ((STATE)) {                                                                 \
    PORT->Group[(GPIO_PORT)].OUTSET.reg = (DATA_MASK);                         \
}                                                                              \
else {                                                                         \
    PORT->Group[(GPIO_PORT)].OUTCLR.reg = (DATA_MASK);                         \
}

/**
    Enable the peripheral multiplexer on the specified pin on the specified port.
**/
#define PMUX_ENABLE(GPIO_PORT, GPIO_PIN)                                       \
PORT->Group[(GPIO_PORT)].PINCFG[(GPIO_PIN)].bit.PMUXEN = 1

/**
    Disable the peripheral multiplexer on the specified pin on the specified port.
**/
#define PMUX_DISABLE(GPIO_PORT, GPIO_PIN)                                      \
PORT->Group[(GPIO_PORT)].PINCFG[(GPIO_PIN)].bit.PMUXEN = 0

/**
    Set the pin configuration for the specified pin on the specified port.
**/
#define PIN_CONFIGURATION(GPIO_PORT, GPIO_PIN, CONFIG)                         \
PORT->Group[(GPIO_PORT)].PINCFG[(GPIO_PIN)].reg = (CONFIG)

/**
    Configure the port data direction as output.
**/
#define DATA_DIRECTION_OUTPUT(GPIO_PORT, DATA_MASK)                            \
PORT->Group[(GPIO_PORT)].DIRSET.reg = (DATA_MASK)

/**
    Configure the port data direction as input.
**/
#define DATA_DIRECTION_INPUT(GPIO_PORT, DATA_MASK)                             \
PORT->Group[(GPIO_PORT)].DIRCLR.reg = (DATA_MASK)

/**
    Wait for the synchronization bits (BITMASK) of the peripheral (PER).
**/
#define SYNCBUSY_WAIT(PER, BITMASK)                                            \
while ((PER)->SYNCBUSY.reg & (BITMASK)) {                                      \
    continue;                                                                  \
}

/**
    Enable the clock specified by the bit on the given Advanced Peripheral Bus.
**/
#define CLOCK_ENABLE(BUS, BIT)                                                 \
MCLK->BUS.bit.BIT = 1

/**
    Set the peripheral's clock source.

    - PER_GLCK_ID is the GCLK ID of a peripheral (e.g. DAC_GCLK_ID).
    - GCLK_INDEX is the numeric index of the GCLK source (i.e. 0-11).

    This will work because GCLK_PCHCTRL_GEN_GCLKx_Val == x for x in [0, 11]
    (see gclk.h).
    Therefore GCLK_PCHCTRL_GEN(x) ==
                GCLK_PCHCTRL_GEN(GCLK_PCHCTRL_GEN_GCLKx_Val)
    and so the correct value will be set in the GEN register of the GCLK.

    The procedure for setting a peripheral's clock source follows from
    section 14.6.3.3 Selecting the Clock Source for a Peripheral (p. 155)
    of the SAMD5xE5x datasheet:
        1. Disable the Peripheral Channel by writing PCHCTRLm.CHEN=0
        2. Assert that PCHCTRLm.CHEN reads '0'
        3. Change the source of the Peripheral Channel by writing PCHCTRLm.GEN
        4. Re-enable the Peripheral Channel by writing PCHCTRLm.CHEN=1

    ...and from section 14.6.3.1 Enabling a Peripheral Clock (p. 155):

    The PCHCTRLm.CHEN bit must be synchronized to the generic clock domain.
    PCHCTRLm.CHEN will continue to read as its previous state until the
    synchronization is complete.

    This necessary synchronization is the reason for the final while-loop.
**/
#define SET_CLOCK_SOURCE(PER_GCLK_ID, GCLK_INDEX)                              \
GCLK->PCHCTRL[(PER_GCLK_ID)].bit.CHEN = 0;                                     \
while (GCLK->PCHCTRL[(PER_GCLK_ID)].bit.CHEN) {                                \
    continue;                                                                  \
}                                                                              \
GCLK->PCHCTRL[(PER_GCLK_ID)].bit.GEN = GCLK_PCHCTRL_GEN((GCLK_INDEX));         \
GCLK->PCHCTRL[(PER_GCLK_ID)].bit.CHEN = 1;                                     \
while (GCLK->SYNCBUSY.reg & GCLK_SYNCBUSY_GENCTRL((GCLK_INDEX))) {             \
    continue;                                                                  \
}
/**
    Return the maximum value of a and b.
**/
#ifndef max
#define max(a, b) (((a) > (b)) ? (a) : (b))
#endif

/**
    Return the minimum value of a and b.
**/
#ifndef min
#define min(a, b) (((a) < (b)) ? (a) : (b))
#endif

#ifdef __cplusplus
extern "C" {
#endif
/**
    \brief Update GCLK frequency

    Updates the divisor on the specified GCLK to generate the requested
    frequency.

    \param[in] gclkIndex The GCLK index.
    \param[in] freqReq The requested frequency, in Hz.
**/
void GClkFreqUpdate(uint8_t gclkIndex, uint32_t freqReq);

#ifdef __cplusplus
}
#endif
#endif // __SYSUTILS_H__
