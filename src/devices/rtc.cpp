#include "rtc.h"
#include "drivers/clock/rv8803.h"
#include "devices/peripherals.h"

constexpr uint32_t i2c_addr = I2C1_BASE;

static Libp::Rv8803::Rv8803<LibpStm32::I2c::I2cBus<i2c_addr>> rtc_(i2c_bus);

//#define MOCK_DATA_RTC

#ifndef MOCK_DATA_RTC

bool initRtc()
{
    rtc_.setPdTimeUpdInt(Libp::Rv8803::PdTimeUpdIntType::minute_update);
    rtc_.setInterrupts(true, false, false, false);

    return true;
}

bool getTime(TimeData& time_data)
{
    bool success = rtc_.readTime(time_data);
    TimeData::bcd2bin(time_data);
    return success;;
}

bool setTime(TimeData time_data)
{
    TimeData::bin2bcd(time_data);
    return rtc_.writeTime(time_data);
}


#else

static TimeData mock_data = {
        .hundredths{0},
        .seconds{0},
        .minutes{0},
        .hours{0},
        .day_of_week{1},
        .day_of_month{1},
        .month{1},
        .year{20},
};

bool initRtc()
{
    return true;
}

bool getTime(TimeData& time_data)
{
    if (++mock_data.minutes == 60)
        mock_data.minutes = 0;
    time_data = mock_data;
    return true;
}

bool setTime(TimeData time_data)
{
    mock_data = time_data;
    return true;
}

#endif
