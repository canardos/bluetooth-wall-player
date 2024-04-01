#include "devices/peripherals.h"

#include "app.h"
#include "error.h"
#include "clock_stm32f1xx.h"
#include "pins_stm32f1xx.h"
#include "serial/uart_stm32f1xx.h"
#include "libpekin_stm32_hal.h"

using namespace Libp;

static constexpr uint32_t baud_rate = 115200;

static LibpStm32::Uart::UartIo<USART2_BASE> uart_out;
static Error::SetLedFunc my_set_led_func = [](bool on) {
    Pins::led_pcb.set(!on);
};

static Error error(my_set_led_func, uart_out);

/// UART clocks/GPIO must be already configured.
void initErrHndlr()
{
    uart_out.start(baud_rate);
}

Error& getErrHndlr()
{
    return error;
}
