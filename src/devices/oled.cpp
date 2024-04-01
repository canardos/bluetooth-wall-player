#include "serial/spi_bus_stm32f1xx.h"
#include "drivers/display/sh1122_driver.h"
#include "drivers/display/ssd1362_driver.h"
#include "devices/peripherals.h"
#include "misc_math.h"
#include "display.h"


static LibpStm32::Spi::SpiBus<SPI1_BASE> spi;

// Use SSD1362 or SH1122 driver
#define USE_SSD1362
#ifdef USE_SSD1362
static Libp::Ssd1362::Ssd1362Driver display(spi,
        Pins::out_disp_rst, Pins::out_disp_cs, Pins::out_disp_cmd,
        Display::width, Display::height);
#else
static Libp::Sh1122::Sh1122Driver display(spi,
        Pins::out_disp_rst, Pins::out_disp_cs, Pins::out_disp_cmd,
        Display::width, Display::height);
#endif

Libp::IDrawingSurface<uint8_t>& getOled()
{
    return display;
}

static uint32_t rollingAvg(uint32_t avg, uint32_t new_val)
{
    constexpr uint16_t n = 1000;
    static uint16_t remainder = 0;
    uint32_t sum = (avg * (n-1) + new_val + remainder);
    uint32_t new_avg = sum / n;
    remainder = sum  - new_avg * n;
    return new_avg;
}

// 0 -> 1,573,000
static constexpr uint8_t milliLuxToBrightness(uint32_t millilux)
{
    constexpr uint8_t min_brightness = 10;
    constexpr uint8_t max_brightness = 80;

    return Libp::constrain(millilux / 128, min_brightness, max_brightness);
}

static constexpr uint32_t starting_millilux = 25000;

void oledInit()
{
    spi.start(
            LibpStm32::Spi::MasterSlave::master,
            LibpStm32::Spi::CpolCpha::cpha0cpol0,
            LibpStm32::Spi::BaudRate::pclk_div_2,
            LibpStm32::Spi::DataFrameFormat::bits_8,
            LibpStm32::Spi::BitEndianess::msb_first);
    display.initDisplay();
    display.setBrightness(milliLuxToBrightness(starting_millilux));
}


void oledUpdateBrightness(uint32_t ambient_millilux)
{
    static uint32_t avg_millilux = starting_millilux;
    static uint8_t current_brightness = milliLuxToBrightness(starting_millilux);

    avg_millilux = rollingAvg(avg_millilux, ambient_millilux);
    uint8_t tgt_brightness = milliLuxToBrightness(avg_millilux);
    if (tgt_brightness > current_brightness)
        current_brightness++;
    else if (tgt_brightness < current_brightness)
        current_brightness--;
    display.setBrightness(current_brightness);
}

void oledSetPower(bool on)
{
    display.setPower(on);
}
