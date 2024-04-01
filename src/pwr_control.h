#ifndef SRC_PWR_CONTROL_H_
#define SRC_PWR_CONTROL_H_

#include <devices/peripherals.h>
#include <devices/rn52.h>
#include <error_handler.h>
#include "devices/oled.h"
#include "libpekin.h"
#include "power_stm32f1xx.h"

class PwrControl {
private:
    static constexpr const char* state_names[3] { "sleep","clock","connected" };
public:
    enum class PwrState : uint8_t {
        sleep,
        clock,
        connected
    };

    static void setState(PwrState state) {

        getErrHndlr().report("PwrState: %s\r\n", state_names[Libp::enumBaseT(state)]);
        static PwrState cur_state = PwrState::connected;
        if (state == cur_state)
            return;

        switch (state) {
        case PwrState::sleep:
            Pins::pwr_en_btn.clear();
            Pins::pwr_en_amp.clear();
            oledSetPower(false);
            Pins::rn52_pwr_en.clear();
            Pins::pwr_en_rn52_disp.clear();
            break;
        case PwrState::clock:
            Pins::pwr_en_amp.clear();
            Pins::pwr_en_rn52_disp.set();
            Pins::rn52_pwr_en.set();
            // Avoid startup interfering with capacitive button init.
            Libp::delayMs(50);
            Pins::pwr_en_btn.set();
            Libp::delayMs(10);
            // If we init on connected -> clock transition, we get a black frame
            if (cur_state == PwrState::sleep)
                oledInit();
            break;
        case PwrState::connected:
            Pins::pwr_en_btn.set();
            Pins::pwr_en_amp.set();
            Pins::pwr_en_rn52_disp.set();
            Pins::rn52_pwr_en.set();
            oledSetPower(true);
            break;
        }
        cur_state = state;
    }

    static void prepareToSleep()
    {
        stopPeripheralsBeforeSleep();
    }
    static void prepareToWake()
    {
        startPeripheralsAfterSleep();
    }

    void sleep()
    {
        setState(PwrState::sleep);
        getErrHndlr().report("Sleeping...");
        prepareToSleep();

        LibpStm32::pwrCtrlStop();

        prepareToWake();
        setState(PwrState::clock);
        getErrHndlr().report("awake\r\n");
        // Must be called post powering the RN52 (discards incoming 'CMD')
        initRn52();
        oledUpdateBrightness( getLightLvl() );
    }

    bool timeToSleep()
    {
        return (Libp::getMillis() - last_activity_time_ms) > inactivity_sleep_time_ms;
    }

    void resetInactivityTimer()
    {
        last_activity_time_ms = Libp::getMillis();
    }

private:
    // Sleep after 3 minutes of inactivity
    static constexpr uint32_t inactivity_sleep_time_ms = 3 * 60 * 1000;
    uint32_t last_activity_time_ms = 0;
};

#endif /* SRC_PWR_CONTROL_H_ */
