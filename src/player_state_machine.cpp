#include <event_queue.h>
#include <devices/peripherals.h>
#include <devices/haptic.h>
#include <devices/rtc.h>
#include <devices/sensors.h>
#include <display.h>
#include <player_state_machine.h>

using namespace Libp;

PlayerStateMachine::PlayerStateMachine(EventQueue& event_queue,
        BtModule& bt_module, Display& display, PwrControl& pwr_ctrl)
            : event_queue_(event_queue), bt_module_(bt_module),
              display_(display), pwr_ctrl_(pwr_ctrl) { }

// Main program loop
void PlayerStateMachine::run()
{
    pwr_ctrl_.resetInactivityTimer();
    display_.notifyNewModuleState(ModuleState::disconnected, ModuleState::disconnected);
    event_queue_.postEvent(EventType::clock_tick); // paint clock
    while (true) {

        while(event_queue_.eventIsPending())
            processEvent(event_queue_.getNextPendingEvent());

        // Flag set by transition to disconnected state.
        // Need to wait on disconnect animation if we were previously connected
        if (draw_clock_when_display_ready_ && display_.disconnectAnimIsDone() ) {
            draw_clock_when_display_ready_ = false;
            // Start clock animation
            display_.notifyNewModuleState(ModuleState::disconnected, ModuleState::disconnected);
            event_queue_.postEvent(EventType::clock_tick);
        }
        // Detect long press of play button
        if (last_play_press_ms) {
            static constexpr uint32_t long_press_duration_ms = 2000;
            uint32_t now_ms = getMillis();
            if ( (now_ms - last_play_press_ms) > long_press_duration_ms) {
                event_queue_.postEvent(EventType::btn_play_long_press);
                last_play_press_ms = 0;
            }
        }

        if (player_state_ == PlayerState::pairing_listening)
            handlePairingModeListening();

        display_.update();
        oledUpdateBrightness( getLightLvl() );
        sleepIfInactive();

        delayMs(5);
    }
}

static uint32_t lastWakeTime = 0;

void PlayerStateMachine::sleepIfInactive()
{
    if (module_state_ == ModuleState::disconnected && pwr_ctrl_.timeToSleep()) {
        player_state_ = PlayerState::normal;
        pwr_ctrl_.sleep();
        lastWakeTime = getMillis();
        event_queue_.postEvent(EventType::clock_tick);
    }
}

void PlayerStateMachine::processEvent(EventQueue::Event event)
{
    getErrHndlr().report("SM: new event (%s)\r\n", event_type_names[enumBaseT(event.event_)]);
    if (event.event_ != EventType::clock_tick)
        pwr_ctrl_.resetInactivityTimer();

    switch (event.event_) {
    case EventType::rn52_gpio2:
        handleGpio2Event();
        break;
    case EventType::rn52_state_chg:
        handleModuleStateChg(event.new_state_);
        break;
    case EventType::track_chg:
        if (module_state_ == ModuleState::connected_streaming)
            handleTrackChange();
        break;
    case EventType::proximity_trigger:
        clearProxInterrupt();
        // nothing else to do, inactivity timer already reset above
        break;
    case EventType::clock_tick:
        handleClockTick();
        break;
    case EventType::btn_play:
    case EventType::btn_next:
    case EventType::btn_prev:
    case EventType::btn_vol_up:
    case EventType::btn_vol_dn:
    case EventType::btn_play_release:
    case EventType::btn_play_long_press:
        handleBtnPress(event);
        break;
    case EventType::do_nothing_event:
        // nothing to do
        break;
    }
}

void PlayerStateMachine::handleClockTick()
{
    if (module_state_ != ModuleState::disconnected)
        return;

    TimeData time_data;
    EnvData env_data;
    const uint32_t secsActive = (getMillis() - lastWakeTime) / 1000;
    bool success = getTime(time_data) && getTemp(env_data, secsActive);
    if (!success)
        getErrHndlr().halt(ErrCode::i2c, "Sensor failure");

    // Don't draw clock if we are still animating disconnect
    if (!draw_clock_when_display_ready_)
        display_.drawClockAndWeather(time_data, env_data);
}

void PlayerStateMachine::handleBtnPress(EventQueue::Event event)
{
    constexpr bool enable_haptic = false;

    if constexpr (enable_haptic) {
        constexpr uint16_t haptic_duration_ms = 80;
        if (event.event_ != EventType::btn_play_release)
            triggerHaptic(haptic_duration_ms);
    }

    // Track long press for play button
    if (event.event_ == EventType::btn_play)
        last_play_press_ms = getMillis();
    else if (event.event_ == EventType::btn_play_release)
        last_play_press_ms = 0;

    // Delegate to handler for the current state
    (this->*btn_handlers_[enumBaseT(player_state_)])(event);
}

void PlayerStateMachine::handleBtnNormal(EventQueue::Event event)
{
    switch (module_state_) {
    case ModuleState::disconnected:
        if (event.event_ == EventType::btn_play_long_press)
            enterMenuMode();
        break;
    case ModuleState::pairing:
        break;
    case ModuleState::connected:
    case ModuleState::connected_streaming:
        switch(event.event_) {
        case EventType::btn_vol_up:
            bt_module_.volUp();
            break;
        case EventType::btn_vol_dn:
            bt_module_.volDown();
            break;
        case EventType::btn_next:
            bt_module_.trackNext();
            break;
        case EventType::btn_prev:
            bt_module_.trackPrev();
            break;
        case EventType::btn_play:
            bt_module_.playPause();
            break;
        case EventType::btn_play_long_press:
        case EventType::btn_play_release:
            break;
        // Illegal states
        case EventType::rn52_gpio2:
        case EventType::rn52_state_chg:
        case EventType::track_chg:
        case EventType::proximity_trigger:
        case EventType::clock_tick:
        case EventType::do_nothing_event:
            break;
        }
        break;
    }
}

void PlayerStateMachine::handleBtnMenu(EventQueue::Event event)
{
    switch(event.event_) {
    case EventType::btn_prev:
         exitMenuMode();
         break;
    case EventType::btn_play:
        bt_module_.enterPairingMode();
        exitMenuMode();
        break;
    case EventType::btn_next:
        enterSetClockMode();
        break;
    case EventType::btn_vol_up:
        bt_module_.resetPairings();
        display_.drawText(Display::TextPos::fullscreen, "All prior", "pairings cleared");
        delayMs(1800);
        exitMenuMode();
        break;
    case EventType::btn_vol_dn:
        NVIC_SystemReset();
        break;
    case EventType::btn_play_release:
    case EventType::btn_play_long_press:
    case EventType::rn52_gpio2:
    case EventType::rn52_state_chg:
    case EventType::track_chg:
    case EventType::proximity_trigger:
    case EventType::clock_tick:
    case EventType::do_nothing_event:
        break;
    }
}

struct ClockFieldWrapper {
    uint8_t* const p_val;
    const uint8_t min_val;
    const uint8_t max_val;
    // Increment value pointed to by p_val and roll back to min_val after max_val
    void inc() const
    {
        *p_val = *p_val == max_val
                ? min_val
                : *p_val + 1;
    }
    // Decrement value pointed to by p_val and roll back to max_val after min_val
    void dec() const
    {
        *p_val = *p_val == min_val
                ? max_val
                : *p_val - 1;
    }
};


void PlayerStateMachine::handleBtnSetClock(EventQueue::Event event)
{
    static ClockField clock_field = ClockField::day_of_month;
    TimeData time_data;
    static const ClockFieldWrapper p_fields[] = {
        // order MUST match ClockField order
        // Unfort. can't popular c style array statically by index
        { reinterpret_cast<uint8_t*>(&time_data) + offsetof(TimeData, day_of_month), 1, 31 },
        { reinterpret_cast<uint8_t*>(&time_data) + offsetof(TimeData, month),        1, 12 },
        { reinterpret_cast<uint8_t*>(&time_data) + offsetof(TimeData, year),         0, 99 },
        { reinterpret_cast<uint8_t*>(&time_data) + offsetof(TimeData, hours),        0, 23 },
        { reinterpret_cast<uint8_t*>(&time_data) + offsetof(TimeData, minutes),      0, 59 }
    };
    // Check that ClockField matches above array element indices
    static_assert(enumBaseT(ClockField::day_of_month) == 0);
    static_assert(enumBaseT(ClockField::month) == 1);
    static_assert(enumBaseT(ClockField::year) == 2);
    static_assert(enumBaseT(ClockField::hours) == 3);
    static_assert(enumBaseT(ClockField::minutes) == 4);

    getTime(time_data);

    // TimeData is guaranteed 8-bytes, ordered

    switch(event.event_) {
    case EventType::btn_prev:
        --clock_field;
         break;
    case EventType::btn_play:
        exitMenuMode();
        return; // avoid clock redraw
    case EventType::btn_next:
        ++clock_field;
        break;
    case EventType::btn_vol_up:
        p_fields[enumBaseT(clock_field)].inc();
        break;
    case EventType::btn_vol_dn:
        p_fields[enumBaseT(clock_field)].dec();
        break;
    case EventType::btn_play_release:
    case EventType::btn_play_long_press:
    case EventType::rn52_gpio2:
    case EventType::rn52_state_chg:
    case EventType::track_chg:
    case EventType::proximity_trigger:
    case EventType::clock_tick:
    case EventType::do_nothing_event:
        return;
    }
    setTime(time_data);
    getTime(time_data); // get correct day_of_week field
    display_.drawAdjustClock(time_data, clock_field);
}

void PlayerStateMachine::handleBtnPairListen(EventQueue::Event event)
{
    // Cancel on any button press
    if (module_state_ == ModuleState::pairing) {
        bt_module_.exitPairingMode();
        player_state_ = PlayerState::normal;
        // RN52 state event will trigger clock redraw
        //handleClockTick();
    }
}

void PlayerStateMachine::handleBtnPairGotCode(EventQueue::Event event)
{
    if (module_state_ != ModuleState::pairing)
        return;

    if (event.event_ == EventType::btn_vol_up)
        bt_module_.acceptPairing();
    else
        // cancel on other button press
        bt_module_.exitPairingMode();
    player_state_ = PlayerState::normal;
}

void PlayerStateMachine::enterMenuMode()
{
    display_.drawMenu();
    player_state_ = PlayerState::menu;
}

void PlayerStateMachine::exitMenuMode()
{
    player_state_ = PlayerState::normal;
    display_.exitMenu();
    // Menu is only supported when disconnected
    handleClockTick();
    // TODO: force animation repaint
}

void PlayerStateMachine::enterSetClockMode()
{
    player_state_ = PlayerState::set_clock;
    TimeData time_data;
    getTime(time_data);
    display_.drawAdjustClock(time_data, ClockField::day_of_month);
}

void PlayerStateMachine::handleTrackChange()
{
    Rn52::MetaData meta = {}; // getMeta wont terminate fields it doesn't get
    bool success = bt_module_.getMetadata(&meta);

    if (!success)
        getErrHndlr().halt(ErrorCode::metadata_failure, "Metadata error\r\n");

    display_.drawMetaText(meta.artist, meta.title);
}

static ModuleState rn52StateToOurs(uint16_t status)
{
    Rn52::StatusState status_state = static_cast<Rn52::StatusState>(status & 0x0f);
    switch (status_state) {
    case Rn52::StatusState::state_limbo:
    case Rn52::StatusState::state_connectable:
        return ModuleState::disconnected;
    case Rn52::StatusState::state_discoverable_connectable:
        return ModuleState::pairing;
    case Rn52::StatusState::state_connected:
        return ModuleState::connected;
    case Rn52::StatusState::state_audio_streaming:
        return ModuleState::connected_streaming;
    case Rn52::StatusState::state_low_battery:
        return ModuleState::connected;
    default:
        getErrHndlr().halt(ErrCode::illegal_state, "Unhandled rn52 state");
        return ModuleState::disconnected; // can't get here
    }
}

static void printStatus(uint16_t status)
{
    getErrHndlr().report("RN52 state: ");
    static const char* labelsStatusState[] = {
        "limbo",
        "connectable",
        "discoverable_connectable",
        "connected",
        "out_call_established",
        "in_call_established",
        "active_call_headset",
        "test_mode",
        "three_way_waiting",
        "three_way_hold",
        "three_way_multi_call",
        "in_call_hold",
        "active_call_handset",
        "audio_streaming",
        "low_battery",
        "" // avoid oob if we get 0xf below
    };
    getErrHndlr().report(labelsStatusState[status & 0x0f]);

    constexpr uint8_t n = 8;
    static Rn52::StatusFlags statuses[n] = {
        Rn52::StatusFlags::active_con_iap,
        Rn52::StatusFlags::active_con_spp,
        Rn52::StatusFlags::active_con_a2dp,
        Rn52::StatusFlags::active_con_hfphsp,
        Rn52::StatusFlags::caller_id_event,
        Rn52::StatusFlags::track_change_event,
        Rn52::StatusFlags::audio_vol_change_event,
        Rn52::StatusFlags::microphone_vol_change_event,
    };
    static const char* labels[] = {
        "active_con_iap",
        "active_con_spp",
        "active_con_a2dp",
        "active_con_hfphsp",
        "caller_id_event",
        "track_change_event",
        "audio_vol_change_event",
        "microphone_vol_change_event",
    };
    getErrHndlr().report(" (flags: ");
    for (uint8_t i = 0; i < n; i++) {
        if ( (status & statuses[i])) {
            getErrHndlr().report(labels[i]);
            getErrHndlr().report(", ");
        }
    }
    getErrHndlr().report(")\r\n");
}

/// Process RN52 GPIO2 event and dispatch appropriate sub events
void PlayerStateMachine::handleGpio2Event()
{
    uint16_t status = bt_module_.queryStatus();
    if (status == Rn52::query_status_error) {
        display_.drawText(Display::TextPos::fullscreen, "RN52", "Status error");
        disableExtIrqs();
        getErrHndlr().halt(ErrCode::illegal_state, "RN52 status error");
    }
    else if (status == 0) {
        display_.drawText(Display::TextPos::fullscreen, "RN52", "in limbo");
        disableExtIrqs();
        getErrHndlr().halt(ErrCode::illegal_state, "RN52 is in limbo");
    }
    else {
        getErrHndlr().report("Status: %02x\r\n", status);
        printStatus(status);
        ModuleState state = rn52StateToOurs(status);
        event_queue_.postEvent(EventType::rn52_state_chg, state);
        if (status & Rn52::StatusFlags::track_change_event) {
            event_queue_.postEvent(EventType::track_chg, state);
        }
    }
}

void PlayerStateMachine::handleModuleStateChg(ModuleState new_state)
{
    const ModuleState old_state = module_state_;
    if (new_state == old_state)
        return;

    switch(new_state) {
    case ModuleState::disconnected:
        pwr_ctrl_.setState(PwrControl::PwrState::clock);
        // Need to wait for display disconnect anim to finish
        draw_clock_when_display_ready_ = true;
        break;
    case ModuleState::connected:
    case ModuleState::connected_streaming:
        pwr_ctrl_.setState(PwrControl::PwrState::connected);
        break;
    case ModuleState::pairing:
        break;
    }

    // Exited pairing mode through connection
    // or button press or connect or timeout
    if (old_state == ModuleState::pairing) {
        player_state_ = PlayerState::normal;
    }

    display_.notifyNewModuleState(new_state, old_state);
    module_state_ = new_state;

    if (new_state == ModuleState::pairing)
        handleEnterPairingMode();
    else if (new_state == ModuleState::connected_streaming)
        handleTrackChange();
}


void PlayerStateMachine::handleEnterPairingMode()
{
    display_.drawText(Display::TextPos::pairing, "Pair your", "device now...");
    player_state_ = PlayerState::pairing_listening;
}

void PlayerStateMachine::handlePairingModeListening()
{
    char passkey[6];
    bool got_key = bt_module_.awaitPairingPasskey(passkey); // 50ms timeout
    if (got_key) {
        constexpr size_t len = sizeof("Code: 123456\0");
        char code[len];
        snprintf(code, len, "Code: %.*s", 6, passkey);
        display_.drawText(Display::TextPos::pairing, code, "[vol+] accept");
        getErrHndlr().report("Got passkey '%.*s'\r\n", (int)6, passkey);
        player_state_ = PlayerState::pairing_got_code;
    }
}
