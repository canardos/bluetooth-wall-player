#include "libpekin.h"
#include "libpekin_stm32_hal.h"

#include "serial/uart_stm32f1xx.h"
#include "serial/i_serial_io.h"

#include "peripherals.h"
#include <drivers/wireless/rn_52.h>

#include <error_handler.h>

using namespace Libp;
using namespace LibpStm32;

static constexpr uint32_t uart_baud = 115200;
/// Needs to be large enough for unsolicited metadata
static constexpr uint16_t uart_buffer_len = 512;

static Uart::UartIo<USART1_BASE, Uart::IrqMode::interrupt, uart_buffer_len> uart;
static Rn52 rn52(uart);

static void powerUpRn52()
{
    Pins::rn52_cmd_mode.clear();
    Pins::rn52_pwr_en.set();
    Pins::pwr_en_rn52_disp.set();
    // Read past "CMD\r\n" or anything else on boot
    char buf;
    uart.read(&buf, 1, 2000);
    while (uart.read(&buf, 1, 10))
            ;
}

void initRn52()
{
    uart.start(Uart::Mode::rx_tx, Uart::Parity::none, Uart::StopBits::bits_1, uart_baud);
    uart.configIrq<Uart::IrqType::rx_data_ready>();
    uart.enableIrq();
    powerUpRn52();
}

Rn52& getRn52()
{
    return rn52;
}


#ifdef __cplusplus
extern "C" {
#endif

__attribute__ ((interrupt("IRQ")))
void USART1_IRQHandler(void)
{
    uart.clearPendingIrq();
    if (uart.isOverrun())
        getErrHndlr().halt(ErrCode::uart_overrun, "Usart overrun");
    uart.serviceIrq();
}

#ifdef __cplusplus
}
#endif
