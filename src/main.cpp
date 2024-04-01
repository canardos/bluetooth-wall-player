#include "libpekin.h"
#include "string_util.h"
#include "libpekin_stm32_hal.h"
#include "clock_stm32f1xx.h"

#include <bt_module.h>
#include <error_handler.h>
#include <event_queue.h>
#include <player_state_machine.h>

#include <devices/peripherals.h>
#include <devices/rn52.h>
#include "devices/oled.h"
#include "devices/sensors.h"
#include "devices/rtc.h"

/*
3.3V / GND / sink
blue /white/brown, left to right front of faceplate
brown/black/blue cable
*/

using namespace Libp;
using namespace LibpStm32;

static BtModule bt_module_(getRn52());
static Display display(getOled());
static PwrControl pwr_ctrl;
static EventQueue queue_;
static PlayerStateMachine player(queue_, bt_module_, display, pwr_ctrl);

void initSysClock()
{
    Clk::setSysClk(Clk::SysClkSrc::ext_high_speed_osc, 2, Clk::PllSrc::hse, 2);
    // 24 MHz
    Clk::setSysClk(Clk::SysClkSrc::pll, 2, Clk::PllSrc::hse, 3);
    // APB1 can't exceed 36 MHz
    Clk::setPeripheralClk(Clk::ApbPrescaler::div1, Clk::ApbPrescaler::div1);
}

void initSensors()
{
    if (!initTempSensor()) {
        getErrHndlr().halt(ErrorCode::module_init_fail, "BME280 init failed");
    }
    if (!initRtc()) {
        getErrHndlr().halt(ErrorCode::module_init_fail, "RTC init failed");
    }

    display.drawText(Display::TextPos::fullscreen, "Calibrating proximity", "in 3");
    delayMs(1000);
    display.drawText(Display::TextPos::fullscreen, "Calibrating proximity", "in 2");
    delayMs(1000);
    display.drawText(Display::TextPos::fullscreen, "Calibrating proximity", "in 1");
    delayMs(1000);

    uint16_t prox_count = initProxSensor();
    if (prox_count == 0) {
        getErrHndlr().halt(ErrorCode::module_init_fail, "Prox init failed");
    }

    constexpr uint8_t max_len = strlen("Done ()") + Libp::maxStrLen<uint16_t>();
    char buf[max_len];
    snprintf(buf, max_len, "Done (%d)", prox_count);
    display.drawText(Display::TextPos::fullscreen, buf, "Initializing Bluetooth...");

    TimeData time_data = {0};
    time_data.year = 20;
    time_data.month = 1;
    time_data.day_of_month = 1;
    setTime(time_data);
}

//#define TEST_PROX_MODE

#ifdef TEST_PROX_MODE
void drawProx()
{
    char buf[50];
    while (true) {
        uint16_t count = getProxCount();
        snprintf(buf, 50, "Count: %d", count);
        display.drawText(Display::TextPos::fullscreen, buf, "");
        delayMs(300);
    }
}
#endif

void initPeripherals()
{
    initGpio();  // enable GPIO clocks before peripherals init
    initSerial();
    initSpi();
    initI2c();
    initTimers();
    initErrHndlr();
}

int main(void)
{
    initSysClock();
    libpekinInitTimers();
    initPeripherals();
    pwr_ctrl.setState(PwrControl::PwrState::clock); // needs errHndlr
    oledInit();                                     // needs SPI and timers
    initSensors();                                  // needs I2C, timers, and display
#ifdef TEST_PROX_MODE
    drawProx();
#endif
    initRn52();
    if (!bt_module_.init()) {
        getErrHndlr().halt(ErrorCode::module_init_fail, "BT init failed");
    }
    enableExtIrqs();
    clearProxInterrupt();
    player.run();
}

#ifdef __cplusplus
extern "C" {
#endif

/// Triggered by RN52 event on GPIO2 and button presses
__attribute__ ((interrupt("IRQ")))
void EXTI15_10_IRQHandler(void)
{
    disableExtIrqs();
    getErrHndlr().report("\r\nEXTI15_10_IRQ\r\n");

    // --- RN52 event ---

    if (Pins::rn52_gpio2.irqIsPending()) {
        queue_.postEvent(EventType::rn52_gpio2);
        Pins::rn52_gpio2.clearPendingIrqBit();
    }

    // --- Button events ---

    if (Pins::btn_play.irqIsPending()) {
        bool pressed = Pins::btn_play.read();
        getErrHndlr().report("Play: %d\r\n", (int)pressed);
        queue_.postEvent(pressed ? EventType::btn_play : EventType::btn_play_release);
        Pins::btn_play.clearPendingIrqBit();
    }
    if (Pins::btn_vol_up.irqIsPending()) {
        queue_.postEvent(EventType::btn_vol_up);
        Pins::btn_vol_up.clearPendingIrqBit();
    }
    if (Pins::btn_vol_dn.irqIsPending()) {
        queue_.postEvent(EventType::btn_vol_dn);
        Pins::btn_vol_dn.clearPendingIrqBit();
    }
    if (Pins::btn_next.irqIsPending()) {
        queue_.postEvent(EventType::btn_next);
        Pins::btn_next.clearPendingIrqBit();
    }
    if (Pins::btn_prev.irqIsPending()) {
        queue_.postEvent(EventType::btn_prev);
        Pins::btn_prev.clearPendingIrqBit();
    }
    Pins::btn_prev.clearPendingIrqLine();
    //__DSB();
    enableExtIrqs();
}

/// Triggered by RTC minute tick
__attribute__ ((interrupt("IRQ")))
void EXTI9_5_IRQHandler(void)
{
    getErrHndlr().report("\r\nEXTI9_5_IRQ\r\n");
    if (Pins::rtc_int.irqIsPending()) {
        queue_.postEvent(EventType::clock_tick);
        Pins::rtc_int.clearPendingIrqBit();
    }
    Pins::rtc_int.clearPendingIrqLine();
    __DSB();
}

/// Triggered by proximity sensor
__attribute__ ((interrupt("IRQ")))
void EXTI4_IRQHandler(void)
{
    getErrHndlr().report("\r\nEXTI4_IRQ\r\n");
    if (Pins::proximity_int.irqIsPending()) {
        queue_.postEvent(EventType::proximity_trigger);
        Pins::proximity_int.clearPendingIrqBit();
    }
    Pins::proximity_int.clearPendingIrqLine();
    __DSB();
}

#ifdef __cplusplus
}
#endif
