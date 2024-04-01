#ifndef SRC_PLAYER_STATE_HPP_
#define SRC_PLAYER_STATE_HPP_

#include <bt_module.h>
#include <display.h>
#include <event_queue.h>
#include <pwr_control.h>
#include <app.h>

/**
 * Core player state machine.
 *
 * Contains main program loop handling device state and event dispatch.
 */
class PlayerStateMachine {
public:
    PlayerStateMachine(
            EventQueue& event_queue, BtModule& bt_module, Display& display, PwrControl& pwr_ctrl);

    /// Main program loop - never returns.
    /// Monitors the event queue, dispatches events
    /// and updates the display.
    [[noreturn]]
    void run();

private:
    enum class PlayerState : uint8_t {
        // DO NOT REORDER or assign values
        normal,            ///< Following module mode
        menu,              ///< menu
        set_clock,         ///< set clock
        pairing_listening, ///< pairing, waiting for connection
        pairing_got_code,  ///< pairing, waiting for user confirmation
    };
    using HandleBtnPressFunc = void (PlayerStateMachine::*)(EventQueue::Event);
    HandleBtnPressFunc btn_handlers_[5] = {
            &PlayerStateMachine::handleBtnNormal,
            &PlayerStateMachine::handleBtnMenu,
            &PlayerStateMachine::handleBtnSetClock,
            &PlayerStateMachine::handleBtnPairListen,
            &PlayerStateMachine::handleBtnPairGotCode
    } ;
    void handleBtnNormal(EventQueue::Event event);
    void handleBtnMenu(EventQueue::Event event);
    void handleBtnSetClock(EventQueue::Event event);
    void handleBtnPairListen(EventQueue::Event event);
    void handleBtnPairGotCode(EventQueue::Event event);
    // make it harder to accidently make a breaking change above
    static_assert(Libp::enumBaseT(PlayerState::normal) == 0);
    static_assert(Libp::enumBaseT(PlayerState::menu) == 1);
    static_assert(Libp::enumBaseT(PlayerState::set_clock) == 2);
    static_assert(Libp::enumBaseT(PlayerState::pairing_listening) == 3);
    static_assert(Libp::enumBaseT(PlayerState::pairing_got_code) == 4);

    /// Our state
    PlayerState player_state_ = PlayerState::normal;

    /// Last reported state of the Bluetooth module
    ModuleState module_state_ = ModuleState::disconnected;

    EventQueue& event_queue_;
    BtModule& bt_module_;
    Display& display_;
    PwrControl& pwr_ctrl_;

    /// Monitor long press of play button
    uint32_t last_play_press_ms = 0;

    /// Triggers shutdown once animations are finished
    bool draw_clock_when_display_ready_ = false;

    /// Shutdown and sleep if inactive
    void sleepIfInactive();

    /// Top level event processor
    void processEvent(EventQueue::Event event);
    /// Delegates button press to appropriate handler
    void handleBtnPress(EventQueue::Event event);

    void handleClockTick();
    void handleProximityEvent();
    void handleGpio2Event();
    void handleModuleStateChg(ModuleState new_state);
    void handleTrackChange();

    void enterMenuMode();
    void exitMenuMode();
    void enterSetClockMode();
    void handleEnterPairingMode();
    void handlePairingModeListening();
};

#endif /* SRC_PLAYER_STATE_HPP_ */
