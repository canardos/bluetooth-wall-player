#ifndef SRC_ANIMATION_H_
#define SRC_ANIMATION_H_

#include <cstdint>
#include <libpekin.h>
#include <graphics/primitives_render.h>
#include "app.h"

/**
 * Manages animation state and drawing.
 *
 * Set animation via `startAnimations` and draw via regular calls to `update`.
 */
class AnimationRender {
public:
    using time_ms_t = uint32_t;
    using frame_count_t = uint8_t;

    AnimationRender(Libp::PrimitivesRender<uint8_t>& painter) : painter_(painter) { }

    void startAnimations(ModuleState new_state, ModuleState old_state)
    {
        switch(new_state) {
        case ModuleState::connected:
            p_anim1_ = &anim_waiting;
            p_anim2_ = nullptr;
            break;
        case ModuleState::connected_streaming:
            p_anim1_ = &anim_listen;
            p_anim2_ = nullptr;
            break;
        case ModuleState::pairing:
            p_anim1_ = &anim_waiting;
            p_anim2_ = &anim_pairing;
            break;
        case ModuleState::disconnected:
            p_anim1_ = (old_state == ModuleState::connected || old_state == ModuleState::connected_streaming)
                    ? &anim_disconnect
                    : &anim_clock;
            p_anim2_ = nullptr;
            break;
        }
        if (p_anim1_)
            p_anim1_->reset();
        if (p_anim2_)
            p_anim2_->reset();
    }

    /**
     * Draw animation frames as required. Must be called regularly if any
     * animations are active.
     *
     * @return true if any drawing operations were performed
     */
    bool update()
    {
        time_ms_t now_ms = Libp::getMillis();
        bool updated = false;

        uint32_t ms_since_last_frame = now_ms - p_anim1_->lastFrameTime();
        if( p_anim1_ && (ms_since_last_frame > p_anim1_->msToNextFrame()) ) {
            if (!p_anim1_->update(painter_, now_ms))
                p_anim1_ = nullptr;
            updated = true;
        }

        ms_since_last_frame = now_ms - p_anim2_->lastFrameTime();
        if( p_anim2_ && (ms_since_last_frame > p_anim2_->msToNextFrame()) ) {
            p_anim2_->update(painter_, now_ms);
            if (!p_anim2_->update(painter_, now_ms))
                p_anim2_ = nullptr;
            updated = true;
        }
        return updated;
    }

    /**
     * @return true if disconnect animation is still in progress
     */
    bool disconnectAnimIsActive()
    {
        return p_anim1_ == &anim_disconnect;
    }

private:
    /// Encapsulates single running animation
    /// including data and state.
    class Animation {
    public:
        /**
         * Draw next frame
         *
         * @param painter
         * @param ms_to_next_frame [out]
         * @param frame [out]
         * @return true if animation still going, false if animation is finished
         */
        using AnimFunction = bool (*)(Libp::PrimitivesRender<uint8_t>& painter, time_ms_t& ms_to_next_frame, frame_count_t& frame);

        Animation(AnimFunction fn) : function_(fn) {}
        void reset() { frame_ = 0; last_frame_time_ms_ = 0; ms_to_next_frame_ = 0; }
        time_ms_t lastFrameTime() { return last_frame_time_ms_; }
        time_ms_t msToNextFrame() { return ms_to_next_frame_; }
        bool update(Libp::PrimitivesRender<uint8_t>& painter, time_ms_t now_ms) {
            last_frame_time_ms_ = now_ms;
            return function_(painter, ms_to_next_frame_, frame_);
        }
    private:
        const AnimFunction function_;
        frame_count_t frame_ = 0;
        time_ms_t last_frame_time_ms_ = 0;
        time_ms_t ms_to_next_frame_ = 0;
    };

    // Max 2 concurrent animations
    Animation* p_anim1_ = nullptr;
    Animation* p_anim2_ = nullptr;
    Libp::PrimitivesRender<uint8_t>& painter_;

    static Animation anim_listen;
    static Animation anim_waiting;
    static Animation anim_disconnect;
    static Animation anim_pairing;
    static Animation anim_clock;
};

#endif /* SRC_ANIMATION_H_ */
