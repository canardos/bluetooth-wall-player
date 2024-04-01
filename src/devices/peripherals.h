#ifndef PERIPHERALS_H_
#define PERIPHERALS_H_

#include <cstdint>

#include "clock_stm32f1xx.h"
#include "pins_stm32f1xx.h"
#include "bits.h"

#include "serial/spi_bus_stm32f1xx.h"
#include "serial/i2c_bus_stm32f1xx.h"

/// GPIO pins
namespace Pins {

inline constexpr LibpStm32::PinC<13> led_pcb;

inline constexpr LibpStm32::PinB<10> btn_vol_up;
inline constexpr LibpStm32::PinB<11> btn_vol_dn;
inline constexpr LibpStm32::PinB<12> btn_next;
inline constexpr LibpStm32::PinB<13> btn_play;
inline constexpr LibpStm32::PinB<14> btn_prev;

inline constexpr LibpStm32::PinB<15> rn52_gpio2;    ///< RN52 event notification pin
inline constexpr LibpStm32::PinA<11> rn52_cmd_mode; ///< RN52 command mode pin
inline constexpr LibpStm32::PinA<12> rn52_pwr_en;   ///< RN52 power enable pin

inline constexpr LibpStm32::PinB<4> proximity_int;
inline constexpr LibpStm32::PinB<9> rtc_int;

inline constexpr LibpStm32::PinB<0> out_disp_cmd;
inline constexpr LibpStm32::PinA<6> out_disp_rst;
inline constexpr LibpStm32::PinB<1> out_disp_cs;
inline constexpr LibpStm32::PinA<1> out_haptic; // TIM2

inline constexpr LibpStm32::PinB<2> pwr_en_btn;
inline constexpr LibpStm32::PinB<8> pwr_en_amp;
inline constexpr LibpStm32::PinB<3> pwr_en_rn52_disp;
}

// Global peripherals
inline constexpr uint32_t i2c_freq = 100'000;
// I2cBus class is fully static. Post optimization, static results
// in less SRAM and Flash usage vs inline for some reason.
static LibpStm32::I2c::I2cBus<I2C1_BASE, i2c_freq> i2c_bus;

/// Must call before peripherals init
inline __attribute__((always_inline))
void initGpio()
{
    // Must be lower than SysTick so we can use ms timers in the ISRs and
    // not mess up other timing.
    //
    // RN52 and all buttons must have the same priority to ensure they don't
    // interrupt each other as the event posting handlers do not support
    // reentrancy.
    static constexpr uint32_t btn_and_rn52_irq_priority = 1;

    LibpStm32::Clk::enable<
            LibpStm32::Clk::Apb2::afio, // for external interrupts
            LibpStm32::Clk::Apb2::iopa,
            LibpStm32::Clk::Apb2::iopb,
            LibpStm32::Clk::Apb2::iopc>();

    // Set unused pins to analog in.
    /*LibpStm32::GpioA::setAllAnalogInput();
    LibpStm32::GpioB::setAllAnalogInput();
    LibpStm32::GpioC::setAllAnalogInput();*/

    // Use B3/B4 as GPIO instead of JTAG
    // Must call after clock enabled
    //__HAL_AFIO_REMAP_SWJ_NOJTAG();
    Libp::Bits::setBits(AFIO->MAPR, AFIO_MAPR_SWJ_CFG_Msk, AFIO_MAPR_SWJ_CFG_JTAGDISABLE);

    Pins::led_pcb.set();
    Pins::pwr_en_rn52_disp.clear();
    Pins::rn52_cmd_mode.clear();

    Pins::pwr_en_btn.setAsOutput(LibpStm32::OutputMode::pushpull, LibpStm32::OutputSpeed::low);
    Pins::pwr_en_amp.setAsOutput(LibpStm32::OutputMode::pushpull, LibpStm32::OutputSpeed::low);

    Pins::led_pcb.setAsOutput(LibpStm32::OutputMode::pushpull, LibpStm32::OutputSpeed::low);
    Pins::pwr_en_rn52_disp.setAsOutput(LibpStm32::OutputMode::pushpull, LibpStm32::OutputSpeed::low);
    Pins::rn52_cmd_mode.setAsOutput(LibpStm32::OutputMode::pushpull, LibpStm32::OutputSpeed::low);
    Pins::rn52_pwr_en.setAsOutput(LibpStm32::OutputMode::pushpull, LibpStm32::OutputSpeed::low);

    Pins::out_disp_cs.setAsOutput(LibpStm32::OutputMode::pushpull, LibpStm32::OutputSpeed::high);
    Pins::out_disp_rst.setAsOutput(LibpStm32::OutputMode::pushpull, LibpStm32::OutputSpeed::low);
    Pins::out_disp_cmd.setAsOutput(LibpStm32::OutputMode::pushpull, LibpStm32::OutputSpeed::high);

    // GPIO with interrupts

    Pins::btn_vol_up.setAsInputIrq(LibpStm32::InputMode::pulldown, true, false);
    Pins::btn_vol_dn.setAsInputIrq(LibpStm32::InputMode::pulldown, true, false);
    Pins::btn_prev.setAsInputIrq(LibpStm32::InputMode::pulldown, true, false);
    Pins::btn_next.setAsInputIrq(LibpStm32::InputMode::pulldown, true, false);
    Pins::btn_play.setAsInputIrq(LibpStm32::InputMode::pulldown, true, true);    // Press and release - need to detect long press

    Pins::rn52_gpio2.setAsInputIrq(LibpStm32::InputMode::pullup, false, true);   // RN52 GPIO2 pulls low for 100ms on event

    Pins::proximity_int.setAsInputIrq(LibpStm32::InputMode::pullup, false, true);// VCNL4200 pulls low, open drain
    Pins::rtc_int.setAsInputIrq(LibpStm32::InputMode::pullup, false, true);      // RV-8803 pulls low, open drain

    NVIC_SetPriority(Pins::btn_vol_up.irqn(), btn_and_rn52_irq_priority);
    NVIC_SetPriority(Pins::btn_vol_dn.irqn(), btn_and_rn52_irq_priority);
    NVIC_SetPriority(Pins::btn_play.irqn(), btn_and_rn52_irq_priority);
    NVIC_SetPriority(Pins::btn_prev.irqn(), btn_and_rn52_irq_priority);
    NVIC_SetPriority(Pins::btn_next.irqn(), btn_and_rn52_irq_priority);

    NVIC_SetPriority(Pins::rn52_gpio2.irqn(), btn_and_rn52_irq_priority);
    NVIC_SetPriority(Pins::proximity_int.irqn(), btn_and_rn52_irq_priority);
    NVIC_SetPriority(Pins::rtc_int.irqn(), btn_and_rn52_irq_priority);
}

/// Assumes relevant GPIO clocks are already enabled
inline __attribute__((always_inline))
void initTimers()
{
    // TIM2 CH2
    LibpStm32::Clk::enable<LibpStm32::Clk::Apb1::tim2>();
    Pins::out_haptic.setAsOutput(LibpStm32::OutputMode::alt_pushpull, LibpStm32::OutputSpeed::low);
}

/// Assumes relevant GPIO clocks are already enabled
inline __attribute__((always_inline))
void initSerial()
{
    // USART1 (used by RN52)
    LibpStm32::Clk::enable<LibpStm32::Clk::Apb2::usart1>();
    LibpStm32::DefPin::usart1_tx.setAsOutput(LibpStm32::OutputMode::alt_pushpull, LibpStm32::OutputSpeed::low);
    LibpStm32::DefPin::usart1_rx.setAsInput(LibpStm32::InputMode::floating);

    // USART2 (used by Error object)
    LibpStm32::Clk::enable<LibpStm32::Clk::Apb1::usart2>();
    LibpStm32::DefPin::usart2_tx.setAsOutput(LibpStm32::OutputMode::alt_pushpull, LibpStm32::OutputSpeed::low);
    LibpStm32::DefPin::usart2_rx.setAsInput(LibpStm32::InputMode::floating);
}

/// Assumes relevant GPIO clocks are already enabled
inline __attribute__((always_inline))
void initSpi()
{
    // SPI1 (used by display, output only)
    LibpStm32::Clk::enable<LibpStm32::Clk::Apb2::spi1>();
    LibpStm32::DefPin::spi1_mosi.setAsOutput(LibpStm32::OutputMode::alt_pushpull, LibpStm32::OutputSpeed::high);
    LibpStm32::DefPin::spi1_sck .setAsOutput(LibpStm32::OutputMode::alt_pushpull, LibpStm32::OutputSpeed::high);
}

/// Assumes relevant GPIO clocks are already enabled
inline __attribute__((always_inline))
void initI2c()
{
    LibpStm32::DefPin::i2c1_scl.setAsOutput(LibpStm32::OutputMode::alt_opendrain, LibpStm32::OutputSpeed::high);
    LibpStm32::DefPin::i2c1_sda.setAsOutput(LibpStm32::OutputMode::alt_opendrain, LibpStm32::OutputSpeed::high);
    LibpStm32::Clk::enable<LibpStm32::Clk::Apb1::i2c1>();
    LibpStm32::Clk::reset<LibpStm32::Clk::Apb1::i2c1>(); // TODO: necessary?
    i2c_bus.initialize();
    i2c_bus.enable();
}


/// Disable RN52 and button interrupts
inline
void disableExtIrqs()
{
    Pins::rn52_gpio2.disableIrq();
    Pins::btn_vol_up.disableIrq();
    Pins::btn_vol_dn.disableIrq();
    Pins::btn_play.disableIrq();
    Pins::btn_prev.disableIrq();
    Pins::btn_next.disableIrq();
    Pins::proximity_int.disableIrq();
    Pins::rtc_int.disableIrq();
    // Ensure no interrupts after we return
    __DSB();
    __ISB();
}

inline
void clearExtIrqs()
{
    Pins::rn52_gpio2.clearPendingIrqBit();
    Pins::btn_vol_up.clearPendingIrqBit();
    Pins::btn_vol_dn.clearPendingIrqBit();
    Pins::btn_play.clearPendingIrqBit();
    Pins::btn_prev.clearPendingIrqBit();
    Pins::btn_next.clearPendingIrqBit();
    Pins::proximity_int.clearPendingIrqBit();
    Pins::rtc_int.clearPendingIrqBit();
    Pins::rn52_gpio2.clearPendingIrqLine();
    Pins::btn_vol_up.clearPendingIrqLine();
    Pins::btn_vol_dn.clearPendingIrqLine();
    Pins::btn_play.clearPendingIrqLine();
    Pins::btn_prev.clearPendingIrqLine();
    Pins::btn_next.clearPendingIrqLine();
    Pins::proximity_int.clearPendingIrqLine();
    Pins::rtc_int.clearPendingIrqLine();
}

/**
 * Enable RN52 and button interrupts
 *
 * @param clear_first clear int pending flags prior to enabling.
 */
inline
void enableExtIrqs(bool clear_first = true)
{
    if (clear_first) {
        clearExtIrqs();
        __DSB();
        __ISB();
    }
    Pins::rn52_gpio2.enableIrq();
    Pins::btn_vol_up.enableIrq();
    Pins::btn_vol_dn.enableIrq();
    Pins::btn_play.enableIrq();
    Pins::btn_prev.enableIrq();
    Pins::btn_next.enableIrq();
    Pins::proximity_int.enableIrq();
    Pins::rtc_int.enableIrq();
}

/// Enable interrupts to wakeup from sleep
inline
void enableWakeupIrqs()
{
    Pins::proximity_int.enableIrq();
}

/// Configure interrupts/peripherals for sleep
inline
void stopPeripheralsBeforeSleep()
{
    // TODO: disable ALS
    disableExtIrqs();
    enableWakeupIrqs();
}

/// Enable interrupts/peripherals that were disabled for sleep
inline
void startPeripheralsAfterSleep()
{
    // TODO: enable ALS
    enableExtIrqs();
    clearExtIrqs();
}

#endif /* PERIPHERALS_H_ */
