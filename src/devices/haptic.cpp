#include "haptic.h"

#include <cstdint>
#include "timer_stm32f1xx.h"
#include "devices/peripherals.h"

using namespace LibpStm32::Tim;

// TIM2 CH2 => Pin A1
static BasicTimer<TIM2_BASE> timer;
static constexpr Channel channel = Channel::ch2;

void triggerHaptic(uint16_t ms)
{
    const uint32_t cycles_per_ms = SystemCoreClock / 1'000;

    timer.initBasic(cycles_per_ms * ms, true);
    timer.setupOutputChannel(
            channel,
            OutputCompareMode::force_active);
    timer.setupOutputChannel(
            channel,
            OutputCompareMode::ch1_inactive_on_match);
    timer.enable();
}
