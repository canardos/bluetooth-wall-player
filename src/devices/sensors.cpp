#include <algorithm>
#include "sensors.h"
#include "drivers/sensors/bme280.h"
#include "drivers/sensors/vcnl_4200.h"
#include "serial/i2c.h"
#include "serial/i2c_bus_stm32f1xx.h"
#include "devices/peripherals.h"
#include "error_handler.h"

using namespace Libp;

constexpr uint32_t i2c_addr = I2C1_BASE;

static I2cWrapper i2c_wrap_bme_(i2c_bus, Bme280::i2c_addr_sdo_gnd);
static I2cWrapper i2c_wrap_prox_(i2c_bus, Vcnl4200::i2c_address);

static Bme280::Bme280 bme280_(i2c_wrap_bme_);
static Vcnl4200::Vcnl4200 vcnl4200_(i2c_wrap_prox_);

#ifndef MOCK_DATA

bool initTempSensor()
{
    return bme280_.initialize()
        && bme280_.reset()
        && bme280_.setConfig(Bme280::Mode::forced,
                // Don't change without updating measure_time_ms below
                Bme280::OSample::osample_x1, Bme280::OSample::osample_x1, Bme280::OSample::osample_x1,
                Bme280::TStandby::ts_10ms,
                Bme280::Filter::coef_16, false);
}
static constexpr uint16_t measure_time_ms = bme280_.maxMeasureTime(true, true, true, 1, 1, 1);


static constexpr Vcnl4200::Vcnl4200Cfg vcnl_cfg = []() {
    using namespace Vcnl4200;
    return Vcnl4200CfgBuilder()
            .alsEnable()
            .psEnable()
            .psLedSetDuty(PsLedDutyCycle::ratio_1_1280)
            .psLedSetDuration(PsLedDuration::it_4t)
            .psLedSetPulses(PsLedNPulses::x8)
            .psIntEnable(PsIntTrigger::closing, 0, 0, PsIntPersistence::x1)
            .build();
}();

uint16_t initProxSensor()
{
    using namespace Vcnl4200;
    bool success = vcnl4200_.configure(vcnl_cfg);

    constexpr uint16_t threshold_margin = 20;
    // Delay until sample is ready
    delayMs(250);
    const uint16_t count = vcnl4200_.psRead();
    const uint16_t threshold = count + threshold_margin;

    getErrHndlr().report("\r\nProx count %d. Threshold = %d\r\n", count, threshold);
    success = success && vcnl4200_.psSetThresholds(threshold, threshold);
    return success
            ? count
            : 0;
}

#ifdef TEST_PROX_MODE
uint16_t getProxCount()
{
    return vcnl4200_.psRead();
}
#endif

uint8_t clearProxInterrupt()
{
    return vcnl4200_.intFlags();
}


// Adjust for heating from proximity sensor/self.
// Using straight line at present until we have more data.
// Curve is specific to the prox sensor params above.
// y = mx + c
static constexpr float m = 1.1187;
static constexpr float c = -464.81;

bool getTemp(EnvData& env_data, uint32_t secsActive)
{
    // Trigger measure
    bool success = bme280_.setMode(Bme280::Mode::forced);
    if (!success)
        return false;

    delayMs(measure_time_ms);
    success = bme280_.getMeasurements(env_data);

    getErrHndlr().report("measured: %d\n", env_data.temperature);

    // Adjust for proximity sensor heating
//    env_data.temperature = 248;
    env_data.temperature = m * env_data.temperature + c;
    getErrHndlr().report("adjusted1: %d\n", env_data.temperature);

    // Adjust for general heating (~0.1deg/min, max 4min)
    env_data.temperature = env_data.temperature - std::min(secsActive, static_cast<uint32_t>(240)) * 0.15;
    getErrHndlr().report("adjusted2: %d\n", env_data.temperature);

    return success;
}

uint32_t getLightLvl()
{
    int32_t als = vcnl4200_.alsRead();
    if (als == -1)
        getErrHndlr().halt(ErrCode::i2c, "ALS read fail");

    return Vcnl4200::alsToMilliLux(als, Vcnl4200::AlsIntegTime::t50ms);
}

#else

bool initTempSensor()
{
    return true;
}

uint16_t initProxSensor()
{
    return 100;
}

uint8_t clearProxInterrupt()
{
    return 0;
}

bool getTemp(EnvData& env_data)
{
    static EnvData mock_data = {
        .pressure{1234},
        .temperature{2000},
        .humidity{47445}
    };
    if ( (mock_data.temperature += 50) == 3000)
        mock_data.temperature = 2000;
    env_data = mock_data;
    return true;
}

uint32_t getLightLvl()
{
    return 1500;
}

#endif
