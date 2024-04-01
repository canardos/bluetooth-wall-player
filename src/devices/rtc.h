#ifndef SRC_DEVICES_RTC_H_
#define SRC_DEVICES_RTC_H_

#include "drivers/clock/rv8803.h"

using TimeData = Libp::Rv8803::TimeData;

bool initRtc();

bool getTime(TimeData& time_data);

/**
 * @param [in,out] time_data day_of_week not required and will be set by this
 *                           function
 */
bool setTime(TimeData time_data);

#endif /* SRC_DEVICES_RTC_H_ */
