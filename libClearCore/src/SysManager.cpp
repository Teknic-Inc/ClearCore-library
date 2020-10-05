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
    Supervisory Manager for the ClearCore Board

    This class implements the high level access to the Teknic ClearCore
    features.
**/

#include "SysManager.h"
#include <stddef.h>
#include <stdio.h>
#include "AdcManager.h"
#include "CcioBoardManager.h"
#include "DigitalIn.h"
#include "DigitalInAnalogIn.h"
#include "DigitalInOut.h"
#include "DigitalInOutAnalogOut.h"
#include "DigitalInOutHBridge.h"
#include "DmaManager.h"
#include "EncoderInput.h"
#include "EthernetManager.h"
#include "HardwareMapping.h"
#include "InputManager.h"
#include "LedDriver.h"
#include "MotorDriver.h"
#include "MotorManager.h"
#include "NvmManager.h"
#include "SdCardDriver.h"
#include "SerialDriver.h"
#include "SerialUsb.h"
#include "ShiftRegister.h"
#include "StatusManager.h"
#include "SysConnectors.h"
#include "SysTiming.h"
#include "SysUtils.h"
#include "UsbManager.h"
#include "XBeeDriver.h"

// Variable from linker script
extern uint32_t __text_start__;

namespace ClearCore {

bool FastSysTick = false;

// Interrupt priority 0(High) - 7(Low)
#define TONE_INTERRUPT_PRIORITY 2
#define MAIN_INTERRUPT_PRIORITY 3
#define SYSTICK_INTERRUPT_PRIORITY 6
#define EIC_INTERRUPT_PRIORITY 7

// These must match the bootloader!
#define DOUBLE_TAP_MAGIC            0xf01669efUL
#define BOOT_DOUBLE_TAP_ADDRESS     (HSRAM_ADDR + HSRAM_SIZE - 4)

// EVSYS channel assignments
enum _evSysCh {
    // Motor HLFB event generators for period/pulse-width TC mode
    EVSYS_M0,
    EVSYS_M1,
    EVSYS_M2,
    EVSYS_M3
};

extern volatile uint32_t tickCnt;
extern uint32_t NvmMgrUnlock;

// Create our core system objects
extern AdcManager &AdcMgr;
extern DmaManager &DmaMgr;
extern EthernetManager &EthernetMgr;
extern CcioBoardManager &CcioMgr;
EncoderInput EncoderIn;
extern InputManager &InputMgr;
extern MotorManager &MotorMgr;
extern NvmManager &NvmMgr;
extern StatusManager &StatusMgr;
extern UsbManager &UsbMgr;
extern SysTiming &TimingMgr;
SdCardDriver SdCard;
ShiftRegister ShiftReg;
XBeeDriver XBee;

// Special accessor to user-controlled LED
LedDriver ConnectorLed;

// I/O Connectors
DigitalInOutAnalogOut ConnectorIO0;

DigitalInOut ConnectorIO1;
DigitalInOut ConnectorIO2;
DigitalInOut ConnectorIO3;

// H-Bridge type connectors
DigitalInOutHBridge ConnectorIO4;
DigitalInOutHBridge ConnectorIO5;

extern DigitalInOutHBridge *const hBridgeCon[HBRIDGE_CON_CNT] = {
    &ConnectorIO4, &ConnectorIO5
};

// Digital input only connectors
DigitalIn ConnectorDI6;
DigitalIn ConnectorDI7;
DigitalIn ConnectorDI8;

DigitalInAnalogIn ConnectorA9;
DigitalInAnalogIn ConnectorA10;
DigitalInAnalogIn ConnectorA11;
DigitalInAnalogIn ConnectorA12;

// ClearPath Motor Connectors
MotorDriver ConnectorM0;
MotorDriver ConnectorM1;
MotorDriver ConnectorM2;
MotorDriver ConnectorM3;

extern MotorDriver *const MotorConnectors[MOTOR_CON_CNT] = {
    &ConnectorM0, &ConnectorM1, &ConnectorM2, &ConnectorM3
};

SerialUsb    ConnectorUsb;
SerialDriver ConnectorCOM0;
SerialDriver ConnectorCOM1;

SysManager SysMgr;

// Create an array of "pin" handlers for each of the ClearCore connectors.
Connector *const Connectors[CLEARCORE_PIN_MAX] = {
    &ConnectorIO0, &ConnectorIO1, &ConnectorIO2, &ConnectorIO3,
    &ConnectorIO4, &ConnectorIO5,
    &ConnectorDI6, &ConnectorDI7, &ConnectorDI8,
    &ConnectorA9, &ConnectorA10, &ConnectorA11, &ConnectorA12,
    &ConnectorLed,
    &ConnectorM0, &ConnectorM1, &ConnectorM2, &ConnectorM3,
    &ConnectorCOM0, &ConnectorCOM1,
    &ConnectorUsb
};

/**
    Constructor
**/
SysManager::SysManager() : m_readyForOperations(false) {
    XBee = XBeeDriver(&XBee_CTS_IN, &XBee_RTS_OUT, &XBee_Rx_IN, &XBee_Tx_OUT,
                      PER_SERCOM_ALT);
    SdCard = SdCardDriver(&MicroSD_MISO, &MicroSD_SS, &MicroSD_SCK,
                          &MicroSD_MOSI, PER_SERCOM_ALT);
    ConnectorLed = LedDriver(ShiftRegister::SR_LED_USB_MASK);

    ConnectorIO0 = DigitalInOutAnalogOut(ShiftRegister::SR_LED_IO_0_MASK,
                                         &IN00n_Aout00n, &OUT00, &Aout00, true);
    ConnectorIO1 = DigitalInOut(ShiftRegister::SR_LED_IO_1_MASK, &IN01n,
                                &OUT01, true);
    ConnectorIO2 = DigitalInOut(ShiftRegister::SR_LED_IO_2_MASK, &IN02n,
                                &OUT02, true);
    ConnectorIO3 = DigitalInOut(ShiftRegister::SR_LED_IO_3_MASK, &IN03n,
                                &OUT03, true);

    ConnectorIO4 = DigitalInOutHBridge(ShiftRegister::SR_LED_IO_4_MASK, &IN04n,
                                       &OUT04_ENABLE04, &Polarity04_PWM04A,
                                       &Polarity04S_PWM04B, TCC4_0_IRQn, false);
    ConnectorIO5 = DigitalInOutHBridge(ShiftRegister::SR_LED_IO_5_MASK, &IN05n,
                                       &OUT05_ENABLE05, &Polarity05_PWM05A,
                                       &Polarity05S_PWM05B, TCC3_0_IRQn, false);

    ConnectorDI6 = DigitalIn(ShiftRegister::SR_LED_DI_6_MASK, &IN06n_QuadA);
    ConnectorDI7 = DigitalIn(ShiftRegister::SR_LED_DI_7_MASK, &IN07n_QuadB);
    ConnectorDI8 = DigitalIn(ShiftRegister::SR_LED_DI_8_MASK, &IN08n_QuadI);

    ConnectorA9 = DigitalInAnalogIn(ShiftRegister::SR_LED_ADI_09_MASK,
                                    ShiftRegister::SR_ANAIN_DIGITAL_09_MASK,
                                    &IN09n_AIN09, AdcManager::ADC_AIN09);
    ConnectorA10 = DigitalInAnalogIn(ShiftRegister::SR_LED_ADI_10_MASK,
                                     ShiftRegister::SR_ANAIN_DIGITAL_10_MASK,
                                     &IN10n_AIN10, AdcManager::ADC_AIN10);
    ConnectorA11 = DigitalInAnalogIn(ShiftRegister::SR_LED_ADI_11_MASK,
                                     ShiftRegister::SR_ANAIN_DIGITAL_11_MASK,
                                     &IN11n_AIN11, AdcManager::ADC_AIN11);
    ConnectorA12 = DigitalInAnalogIn(ShiftRegister::SR_LED_ADI_12_MASK,
                                     ShiftRegister::SR_ANAIN_DIGITAL_12_MASK,
                                     &IN12n_AIN12, AdcManager::ADC_AIN12);

    ConnectorM0 = MotorDriver(ShiftRegister::SR_EN_OUT_0_MASK, &Mtr0_An_SCTx,
                              &Mtr0_B, &Mtr0_HLFB_SCRx, 4, EVSYS_M0);
    ConnectorM1 = MotorDriver(ShiftRegister::SR_EN_OUT_1_MASK, &Mtr1_An,
                              &Mtr1_B, &Mtr1_HLFB, 5, EVSYS_M1);
    ConnectorM2 = MotorDriver(ShiftRegister::SR_EN_OUT_2_MASK,
                              &Mtr2_An_Sdrvr2_PWMA, &Mtr2_B_Sdrvr2_PWMB,
                              &Mtr2_HLFB_Sdrvr2_Trig, 3, EVSYS_M2);
    ConnectorM3 = MotorDriver(ShiftRegister::SR_EN_OUT_3_MASK,
                              &Mtr3_An_Sdrvr3_PWMA, &Mtr3_B_Sdrvr3_PWMB,
                              &Mtr3_HLFB_Sdrvr3_Trig, 0, EVSYS_M3);

    ConnectorCOM0 = SerialDriver(0, ShiftRegister::SR_LED_COM_0_MASK,
                                 ShiftRegister::SR_UART_SPI_SEL_0_MASK,
                                 ShiftRegister::SR_UART_TTL_0_MASK,
                                 &Com0_CTS_MISO, &Com0_RTS_SS, &Com0_RX_SCK,
                                 &Com0_TX_MOSI, PER_SERCOM_ALT);
    ConnectorCOM1 = SerialDriver(1, ShiftRegister::SR_LED_COM_1_MASK,
                                 ShiftRegister::SR_UART_SPI_SEL_1_MASK,
                                 ShiftRegister::SR_UART_TTL_1_MASK,
                                 &Com1_CTS_MISO, &Com1_RTS_SS, &Com1_RX_SCK,
                                 &Com1_TX_MOSI, PER_SERCOM);
}

/**
    Initialize the board to power-up state.
**/
void SysManager::Initialize() {
    // Clear and enable the cycle counter
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0;
    DWT->CTRL = DWT_CTRL_CYCCNTENA_Msk;

    InitClocks();

    // Enable brownout detection on the 3.3V rail. The default fuse value is 1.7
    // Set brownout detection to ~2.5V. Default from factory is 1.7V,
    // It appears that NVM can work to as low as 1.7V
    SUPC->BOD33.bit.ENABLE = 0;
    SUPC->BOD33.bit.LEVEL = 167;  // Brown out voltage = 1.5V + LEVEL * 6mV.
    // Reset
    // If desired, an interrupt can be triggered instead of reset. Useful if
    // sensitive actions need to be completed at the last moment.
    SUPC->BOD33.bit.ACTION = SUPC_BOD33_ACTION_RESET_Val;//SUPC_BOD33_ACTION_NONE_Val;
    // Hysteresis voltage (4 bits). HYST*6mV
    SUPC->BOD33.bit.HYST = 0x7;
    SUPC->BOD33.bit.ENABLE = 1; // enable brown-out detection

    // Reset and initialize the HBridge
    StatusMgr.HBridgeState(true);
    Delay_ms(1);
    StatusMgr.HBridgeState(false);

    // Set the output direction for the OutFault_04or05
    DATA_DIRECTION_INPUT(OutFault_04or05.gpioPort,
                         1UL << OutFault_04or05.gpioPin);

    PIN_CONFIGURATION(OutFault_04or05.gpioPort, OutFault_04or05.gpioPin,
                      PORT_PINCFG_INEN);

    InputMgr.Initialize();

    for (int32_t i = 0; i < CLEARCORE_PIN_MAX; i++) {
        Connectors[i]->Initialize(static_cast<ClearCorePins>(i));
    }

    DmaMgr.Initialize();
    MotorMgr.Initialize();
    ShiftReg.Initialize();
    AdcMgr.Initialize();
    CcioMgr.Initialize();
    UsbMgr.Initialize();
    EncoderIn.Initialize();

    // Configure external interrupt controller
    SET_CLOCK_SOURCE(EIC_GCLK_ID, 0);

    ShiftReg.LedPattern(ShiftRegister::SR_UNDERGLOW_MASK,
                        ShiftRegister::LED_BLINK_BREATHING, true);

    // Enable clock EIC for I/O interrupts
    CLOCK_ENABLE(APBAMASK, EIC_);

    // Enable External Interrupt Controllers 0-15
    // EIC_0_IRQn = 12, ..., EIC_15_IRQn = 27
    for (IRQn_Type irq = EIC_0_IRQn; irq <= EIC_15_IRQn;
            irq = static_cast<IRQn_Type>(static_cast<uint8_t>(irq) + 1)) {
        NVIC_EnableIRQ(irq);
        NVIC_SetPriority(irq, EIC_INTERRUPT_PRIORITY);
    }

    NVIC_EnableIRQ(TCC0_0_IRQn); // Enable sample rate interrupt
    NVIC_SetPriority(TCC0_0_IRQn, MAIN_INTERRUPT_PRIORITY); // Set priority

    NVIC_EnableIRQ(GMAC_IRQn);
    NVIC_SetPriority(GMAC_IRQn, MAIN_INTERRUPT_PRIORITY);

    NVIC_EnableIRQ(TCC4_0_IRQn); // Enable IO4 tone interrupt
    NVIC_EnableIRQ(TCC3_0_IRQn); // Enable IO5 tone interrupt
    NVIC_SetPriority(TCC4_0_IRQn, TONE_INTERRUPT_PRIORITY); // Set priority
    NVIC_SetPriority(TCC3_0_IRQn, TONE_INTERRUPT_PRIORITY); // Set priority

    // Set SysTick to 1ms interval
    if (TimingMgr.SysTickPeriodMicroSec(1000)) {
        // Capture error
        while (1) {
            continue;
        }
    }
    // Set priority for SysTick interrupt (2nd lowest).
    NVIC_SetPriority(SysTick_IRQn, SYSTICK_INTERRUPT_PRIORITY);

    // Run power-on tests and detect faults if any.
    StatusMgr.Initialize(ShiftRegister::SR_UNDERGLOW_MASK);

    // Ethernet PHY requires 300us + 10ms minimum for a cold startup.
    while (Microseconds() < 10300) {
        continue;
    }

    EthernetMgr.Initialize();

    m_readyForOperations = true;
}

/**
    Update systems at the sample rate
**/
void SysManager::UpdateFastImpl() {
    CcioMgr.Refresh();
    AdcMgr.Update();
    StatusMgr.Refresh();
    UsbMgr.Refresh();
    InputMgr.UpdateBegin();

    if (SysMgr.Ready()) {
        for (uint8_t i = 0; i < CLEARCORE_PIN_MAX; i++) {
            Connectors[i]->Refresh();
        }
    }

    InputMgr.UpdateEnd();
    EncoderIn.Update();

    // Update subsystems in the background
    ShiftReg.Update();
    TimingMgr.Update();

    tickCnt++;
}

/**
    Update systems at SysTick rate.
**/
void SysManager::UpdateSlowImpl() {
    if (!m_readyForOperations) {
        return;
    }

    // CCIO-8 Auto-Rediscover
    CcioMgr.RefreshSlow();

    for (uint8_t iMotor = 0; iMotor < MOTOR_CON_CNT; iMotor++) {
        MotorConnectors[iMotor]->RefreshSlow();
    }
}

Connector *SysManager::ConnectorByIndex(ClearCorePins theConnector) {
    if (theConnector < CLEARCORE_PIN_MAX) {
        return Connectors[theConnector];
    }
    else {
        return CcioMgr.PinByIndex(theConnector);
    }
}

void SysManager::InitClocks() {
    // Set up TCC0 which will be used to generate the sample time interrupt
    // and by the motors in S&D/PWM mode to send bursts of steps or PWM duty.
    SET_CLOCK_SOURCE(TCC0_GCLK_ID, 1);
    CLOCK_ENABLE(APBBMASK, TCC0_);

    // Disable TCC0
    TCC0->CTRLA.bit.ENABLE = 0;
    SYNCBUSY_WAIT(TCC0, TCC_SYNCBUSY_ENABLE);

    // Initialize counter value to zero
    TCC0->COUNT.reg = 0;

    // Use double buffering
    TCC0->CTRLBCLR.bit.LUPD = 1;

    // Set TCC0 as normal PWM, invert the polarity of the outputs
    TCC0->WAVE.reg |= TCC_WAVE_WAVEGEN_NPWM | TCC_WAVE_POL_Msk;
    // Set the initial value
    for (int8_t iChannel = 0; iChannel < 6; iChannel++) {
        TCC0->CC[iChannel].reg = 0;
    }
    // Interrupt every period
    TCC0->INTENSET.bit.OVF = 1;

    // Setup TCC1 which will be used by motors using PWM input on InA
    SET_CLOCK_SOURCE(TCC1_GCLK_ID, 1);
    CLOCK_ENABLE(APBBMASK, TCC1_);

    // Disable TCC1
    TCC1->CTRLA.bit.ENABLE = 0;
    SYNCBUSY_WAIT(TCC1, TCC_SYNCBUSY_ENABLE);

    // Initialize counter value to zero
    TCC1->COUNT.reg = 0;

    // Use double buffering
    TCC1->CTRLBCLR.bit.LUPD = 1;

    // Set TCC1 as normal PWM, invert the polarity of the outputs
    TCC1->WAVE.reg |= TCC_WAVE_WAVEGEN_NPWM | TCC_WAVE_POL_Msk;

    // Set the initial value
    for (int8_t iChannel = 0; iChannel < 6; iChannel++) {
        TCC1->CC[iChannel].reg = 0;
    }

    // Initialize Timer/Counters
    SET_CLOCK_SOURCE(TC1_GCLK_ID, 6);
    CLOCK_ENABLE(APBAMASK, TC1_); // Enable TC1 bus clock

    SET_CLOCK_SOURCE(TC2_GCLK_ID, 6);
    CLOCK_ENABLE(APBBMASK, TC2_); // Enable TC2 bus clock

    SET_CLOCK_SOURCE(TC5_GCLK_ID, 6);
    CLOCK_ENABLE(APBCMASK, TC5_); // Enable TC5 bus clock

    GCLK->PCHCTRL[TC6_GCLK_ID].reg =
        GCLK_PCHCTRL_GEN_GCLK6 | GCLK_PCHCTRL_CHEN;
    CLOCK_ENABLE(APBDMASK, TC6_); // Enable TC6 bus clock

    // TCC3 used by IO5 for H-bridge PWM generation
    SET_CLOCK_SOURCE(TCC3_GCLK_ID, 0);
    CLOCK_ENABLE(APBCMASK, TCC3_);

    // TCC4 used by IO4 for H-bridge PWM generation
    SET_CLOCK_SOURCE(TCC4_GCLK_ID, 0);
    CLOCK_ENABLE(APBDMASK, TCC4_);

    Tc *TCs[] = {TC1, TC2, TC6};
    for (int8_t tcIndex = 0; tcIndex < 3; tcIndex++) {
        Tc *tc = TCs[tcIndex];
        TcCount8 *tcCount = &tc->COUNT8;

        // Disable TCx
        tcCount->CTRLA.bit.ENABLE = 0;
        SYNCBUSY_WAIT(tcCount, TC_SYNCBUSY_ENABLE);

        tcCount->CTRLBCLR.bit.LUPD = 1; // Double buffering
        tcCount->CTRLA.bit.MODE = TC_CTRLA_MODE_COUNT8_Val;
        // Make 500Hz carrier from GCLK
        tcCount->CTRLA.bit.PRESCALER = TC_CTRLA_PRESCALER_DIV16_Val;
        tcCount->WAVE.reg = TC_WAVE_WAVEGEN_NPWM;
        tcCount->DRVCTRL.reg = TC_DRVCTRL_INVEN_Msk;

        tcCount->PERBUF.reg = 254;
        tcCount->CCBUF[0].reg = 0;
        tcCount->CCBUF[1].reg = 0;

        tcCount->COUNT.reg = 0;

        // Enable TCx
        tcCount->CTRLA.bit.ENABLE = 1;
        SYNCBUSY_WAIT(tcCount, TC_SYNCBUSY_ENABLE);
    }
}

void SysManager::ResetBoard(ResetModes mode) {
    __disable_irq();

    uint32_t *addr = (uint32_t *)BOOT_DOUBLE_TAP_ADDRESS;
    if (mode == RESET_TO_BOOTLOADER) {
        *addr = DOUBLE_TAP_MAGIC;
    }
    else {
        *addr = 0;
    }

    // Reset the device
    NVIC_SystemReset();

    while (true) {
        continue;
    }
}

void SysManager::SysTickUpdate() {
    if (!FastSysTick) {
        SysMgr.UpdateSlowImpl();
    }
}

#define ACK_FAST_UPDATE_INT TCC0->INTFLAG.reg = TCC_INTFLAG_MASK

void SysManager::FastUpdate() {
    ACK_FAST_UPDATE_INT;
    TimingMgr.IsrStart();
    SysMgr.UpdateFastImpl();
    if (FastSysTick) {
        SysMgr.UpdateSlowImpl();
    }

    TimingMgr.IsrEnd();
}

} // ClearCore namespace


// =============================================================================
// =========================== Connector ISR Handlers ==========================
// =============================================================================

extern "C" void GMAC_Handler(void) {
    ClearCore::EthernetMgr.IrqHandlerGmac();
}

extern "C" void SERCOM0_0_Handler(void) {
    ClearCore::ConnectorCOM1.IrqHandlerTx();
}
extern "C" void SERCOM0_2_Handler(void) {
    ClearCore::ConnectorCOM1.IrqHandlerRx();
}
extern "C" void SERCOM0_3_Handler(void) {
    ClearCore::ConnectorCOM1.IrqHandlerException();
}

extern "C" void SERCOM2_0_Handler(void) {
    ClearCore::XBee.IrqHandlerTx();
}
extern "C" void SERCOM2_2_Handler(void) {
    ClearCore::XBee.IrqHandlerRx();
}
extern "C" void SERCOM2_3_Handler(void) {
    ClearCore::XBee.IrqHandlerException();
}

extern "C" void SERCOM7_0_Handler(void) {
    ClearCore::ConnectorCOM0.IrqHandlerTx();
}
extern "C" void SERCOM7_2_Handler(void) {
    ClearCore::ConnectorCOM0.IrqHandlerRx();
}
extern "C" void SERCOM7_3_Handler(void) {
    ClearCore::ConnectorCOM0.IrqHandlerException();
}

extern "C" void EIC_0_Handler(void) {
    ClearCore::InputMgr.EIC_Handler(0);
}

extern "C" void EIC_1_Handler(void) {
    ClearCore::InputMgr.EIC_Handler(1);
}

extern "C" void EIC_2_Handler(void) {
    ClearCore::InputMgr.EIC_Handler(2);
}

extern "C" void EIC_3_Handler(void) {
    ClearCore::InputMgr.EIC_Handler(3);
}

extern "C" void EIC_4_Handler(void) {
    ClearCore::InputMgr.EIC_Handler(4);
}

extern "C" void EIC_5_Handler(void) {
    ClearCore::InputMgr.EIC_Handler(5);
}

extern "C" void EIC_6_Handler(void) {
    ClearCore::InputMgr.EIC_Handler(6);
}

extern "C" void EIC_7_Handler(void) {
    ClearCore::InputMgr.EIC_Handler(7);
}

extern "C" void EIC_8_Handler(void) {
    ClearCore::InputMgr.EIC_Handler(8);
}

extern "C" void EIC_9_Handler(void) {
    ClearCore::InputMgr.EIC_Handler(9);
}

extern "C" void EIC_10_Handler(void) {
    ClearCore::InputMgr.EIC_Handler(10);
}

extern "C" void EIC_11_Handler(void) {
    ClearCore::InputMgr.EIC_Handler(11);
}

extern "C" void EIC_12_Handler(void) {
    ClearCore::EthernetMgr.IrqHandlerPhy();
}

extern "C" void EIC_13_Handler(void) {
    ClearCore::InputMgr.EIC_Handler(13);
}

extern "C" void EIC_14_Handler(void) {
    ClearCore::InputMgr.EIC_Handler(14);
}

extern "C" void EIC_15_Handler(void) {
    ClearCore::InputMgr.EIC_Handler(15);
}

extern "C" void TCC3_0_Handler(void) {
    TCC3->INTFLAG.reg = TCC_INTFLAG_MASK;
    ClearCore::ConnectorIO5.ToneUpdate();
}
extern "C" void TCC4_0_Handler(void) {
    TCC4->INTFLAG.reg = TCC_INTFLAG_MASK;
    ClearCore::ConnectorIO4.ToneUpdate();
}

extern "C" void SysTick_Handler(void) {
    ClearCore::SysMgr.SysTickUpdate();
}
/**
    Interrupt to handle ClearCore background tasks
**/
extern "C" void TCC0_0_Handler(void) {
    ClearCore::SysMgr.FastUpdate();
}

extern "C" void InitSysManager() {
    // Start the ClearCore board manager
    ClearCore::SysMgr.Initialize();
}

///////////////////////////////////////////////////////////////////
//                    Startup Code                              //
/////////////////////////////////////////////////////////////////

extern uint32_t __etext;
extern uint32_t __data_start__;
extern uint32_t __data_end__;
extern uint32_t __bss_start__;
extern uint32_t __bss_end__;
extern uint32_t __StackTop;
extern "C" void __libc_init_array(void);

extern int main(void);

/**
    This is called on processor reset to initialize the device and call main().
*/
void Reset_Handler(void) {
    uint32_t *pSrc, *pDest;

    // Initialize the initialized data section
    pSrc = &__etext;
    pDest = &__data_start__;

    if ((&__data_start__ != &__data_end__) && (pSrc != pDest)) {
        for (; pDest < &__data_end__; pDest++, pSrc++) {
            *pDest = *pSrc;
        }
    }

    // Clear the zero section
    if (&__bss_start__ != &__bss_end__) {
        for (pDest = &__bss_start__; pDest < &__bss_end__; pDest++) {
            *pDest = 0ul;
        }
    }

    SystemInit();

    /* Initialize the C library */
    __libc_init_array();

    ClearCore::SysMgr.Initialize();

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
    // ISO C++ forbids hijacking main(), but we can't avoid it
    // in this application.
    main();
#pragma GCC diagnostic pop

    while (1) {
        continue;
    }
}
