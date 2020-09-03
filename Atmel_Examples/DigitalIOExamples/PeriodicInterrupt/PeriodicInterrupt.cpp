/*
 * PeriodicInterrupt
 * Configure a user-defined periodic interrupt.
 */
 /*
 * Title: PeriodicInterrupt
 *
 * Objective:
 *    This example demonstrates how to generate a user defined periodic
 * interrupt.
 *
 * Description:
 *    This example configures a periodic interrupt handler that turns the
 * user LED on and off during each call to the interrupt. Once configured,
 * The interrupt will execute at the requested frequency without having to
 * be called from the main program.
 *
 * Requirements:
 * ** None
 *
 * Links:
 * ** ClearCore Documentation: https://teknic-inc.github.io/ClearCore-library/
 * ** ClearCore Manual: https://www.teknic.com/files/downloads/clearcore_user_manual.pdf
 *
 * Last Modified: 6/11/2020
 * Copyright (c) 2020 Teknic Inc. This work is free to use, copy and distribute under the terms of
 * the standard MIT permissive software license which can be found at https://opensource.org/licenses/MIT
 */

#include "ClearCore.h"

/**
    \brief Start a periodic interrupt

    Configures the TCC2 clock prescaler and period to generate an interrupt
    at the requested frequency. If the requested frequency is zero, the
    interrupt will not be configured. TCC2 is not used by any ClearCore
    hardware or core libraries.

    \param [in] frequencyHz The rate which the interrupt should occur.
**/
void ConfigurePeriodicInterrupt(uint32_t frequencyHz);

/// Make an alias for the ISR handler so that the user code does not need
/// to know which IRQ it is being fired from
extern "C" void TCC2_0_Handler(void) __attribute__((
            alias("PeriodicInterrupt")));

// Periodic interrupt priority
// 0 is highest priority, 7 is lowest priority
// Recommended priority is >= 4 to not interfere with other processing
#define PERIODIC_INTERRUPT_PRIORITY     4
#define ACK_PERIODIC_INTERRUPT  TCC2->INTFLAG.reg = TCC_INTFLAG_MASK

// State currently written to LED_BUILTIN
bool ledState = false;
uint32_t interruptFreqHz = 4;

/**
    \brief The periodic interrupt handler

    This is the function where your code to periodically execute should live.

    \note ACK_PERIODIC_INTERRUPT must be called to clear the interrupt.
**/
extern "C" void PeriodicInterrupt(void) {
    // Perform periodic processing here.
    ledState = !ledState;
    ConnectorLed.State(ledState);

    // Acknowledge the interrupt to clear the flag and wait for the next interrupt.
    ACK_PERIODIC_INTERRUPT;
}

int main() {
    ConfigurePeriodicInterrupt(interruptFreqHz);
}

void ConfigurePeriodicInterrupt(uint32_t frequencyHz) {
    // Enable the TCC2 peripheral.
    // TCC2 and TCC3 share their clock configuration and they
    // are already configured to be clocked at 120 MHz from GCLK0.
    CLOCK_ENABLE(APBCMASK, TCC2_);

    // Disable TCC2.
    TCC2->CTRLA.bit.ENABLE = 0;
    SYNCBUSY_WAIT(TCC2, TCC_SYNCBUSY_ENABLE);

    // Reset the TCC module so we know we are starting from a clean state.
    TCC2->CTRLA.bit.SWRST = 1;
    while (TCC2->CTRLA.bit.SWRST) {
        continue;
    }

    // If the frequency requested is zero, disable the interrupt and bail out.
    if (!frequencyHz) {
        NVIC_DisableIRQ(TCC2_0_IRQn);
        return;
    }

    // Determine the clock prescaler and period value needed to achieve the
    // requested frequency.
    uint32_t period = (CPU_CLK + frequencyHz / 2) / frequencyHz;
    uint8_t prescale;
    // Make sure period is >= 1.
    period = max(period, 1U);

    // Prescale values 0-4 map to prescale divisors of 1-16,
    // dividing by 2 each increment.
    for (prescale = TCC_CTRLA_PRESCALER_DIV1_Val;
            prescale < TCC_CTRLA_PRESCALER_DIV16_Val && (period - 1) > UINT16_MAX;
            prescale++) {
        period = period >> 1;
    }
    // Prescale values 5-7 map to prescale divisors of 64-1024,
    // dividing by 4 each increment.
    for (; prescale < TCC_CTRLA_PRESCALER_DIV1024_Val && (period - 1) > UINT16_MAX;
            prescale++) {
        period = period >> 2;
    }
    // If we have maxed out the prescaler and the period is still too big,
    // use the maximum period. This results in a ~1.788 Hz interrupt.
    if (period > UINT16_MAX) {
        TCC2->PER.reg = UINT16_MAX;
    }
    else {
        TCC2->PER.reg = period - 1;
    }
    TCC2->CTRLA.bit.PRESCALER = prescale;

    // Interrupt every period on counter overflow.
    TCC2->INTENSET.bit.OVF = 1;
    // Enable TCC2.
    TCC2->CTRLA.bit.ENABLE = 1;

    // Set the interrupt priority and enable it.
    NVIC_SetPriority(TCC2_0_IRQn, PERIODIC_INTERRUPT_PRIORITY);
    NVIC_EnableIRQ(TCC2_0_IRQn);
}
