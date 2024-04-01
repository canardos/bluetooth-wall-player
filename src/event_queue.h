#ifndef SRC_EVENT_QUEUE_H_
#define SRC_EVENT_QUEUE_H_

#include <cstdint>
#include <atomic>
#include "app.h"
#include "error_handler.h"
#include "devices/peripherals.h"

enum class EventType {
    do_nothing_event,
    proximity_trigger,
    clock_tick,
    rn52_gpio2,
    rn52_state_chg,
    track_chg,
    btn_play, btn_next, btn_prev, btn_vol_up, btn_vol_dn,
    btn_play_release,
    btn_play_long_press
};

/// labels for serial debugging
inline const char* event_type_names[] = {
    "do_nothing_event",
    "proximity_trigger",
    "clock_tick",
    "rn52_gpio2",
    "rn52_state_chg",
    "track_chg",
    "btn_play", "btn_next", "btn_prev", "btn_vol_up", "btn_vol_dn",
    "btn_play_release",
    "btn_play_long_press"
};

/// Message queue to receive irq events.
/// e.g. RN52 events, button presses, proximity etc.
class EventQueue {
public:
    struct Event {
        EventType event_;
        ModuleState new_state_;
    };

    // NOTE: reentrant

    /**
     * Place a new event in the queue.
     *
     * @param type
     * @param new_state only used if type == EventType::rn52_state_chg
     */
    void postEvent(EventType type, ModuleState new_state = ModuleState::disconnected)
    {
        getErrHndlr().report("EQ: postEvent (%s)\r\n", event_type_names[Libp::enumBaseT(type)]);
        // This function is called by the irq handlers and the main program
        // loop. Interrupts are all the same priority and so can't interrupt
        // each other, but an interrupt could interrupt calls from the main
        // loop so we need to disable interrupts as it's not safe re-entry.
        //
        // Interrupt flags will still be set and so should be serviced as soon
        // as interrupts are re-enabled below (edit: not so because they are
        // cleared before enabling).
        //
        disableExtIrqs();
        if (n_pending_events_ == queue_length)
            // queue full - should we halt here instaed of dropping events?
            return;
        pending_events_[write_idx_++] = { type, new_state };
        if (write_idx_ == queue_length)
            write_idx_ = 0;
        n_pending_events_++;
        enableExtIrqs();
    }

    bool eventIsPending()
    {
        return n_pending_events_ > 0;
    }

    Event getNextPendingEvent()
    {
        disableExtIrqs();
        if (n_pending_events_ == 0)
            return { EventType::do_nothing_event, ModuleState::disconnected };
        // Copy with interrupts disabled to ensure
        // no partial overwrite while returning a copy
        Event event_copy = pending_events_[read_idx_++];
        if (read_idx_ == queue_length)
            read_idx_ = 0;
        n_pending_events_--;
        enableExtIrqs();
        return event_copy;
    }

private:
    static constexpr uint8_t queue_length = 5;
    Event pending_events_[queue_length];
    std::atomic<uint8_t> write_idx_ = 0;
    uint8_t read_idx_ = 0;
    std::atomic<uint8_t> n_pending_events_ = 0;
};

#endif /* SRC_EVENT_QUEUE_H_ */
