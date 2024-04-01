#include "animation_render.h"
#include "data/image_data.h"
#include "display.h"

using namespace Libp;

static bool doListen(
        PrimitivesRender<uint8_t>& painter,
        AnimationRender::time_ms_t& ms_to_next_frame,
        AnimationRender::frame_count_t& frame_)
{
    painter.drawBitmap(Display::mgn_left, Display::center_y, bt_connected_img, Align::middle_left);
    if (frame_) {
        painter.drawBitmap(192, 1, cat_left_img, Align::top_left);
        frame_ = 0;
    }
    else {
        painter.drawBitmap(192, 1, cat_right_img, Align::top_left);
        frame_ = 1;
    }
    ms_to_next_frame = 800;
    return true;
}

static bool doWaitingActual(
        PrimitivesRender<uint8_t>& painter,
        AnimationRender::time_ms_t& ms_to_next_frame,
        AnimationRender::frame_count_t& frame_,
        bool clock)
{
    enum class Face : uint8_t {
        look_straight = 0, look_left = 1, look_right = 2, tongue_out = 3, blink = 4
    };

    if (!clock)
        painter.drawBitmap(Display::mgn_left, Display::center_y, bt_connected_img, Align::middle_left);

    // number of ms until hiding cat
    static int32_t hide_in_ms;
    static Face face = Face::look_straight;

    constexpr uint16_t slide_frames = Display::height - Display::cat_pos_y_clock - 1;
    const uint16_t cat_pos_x = clock ? Display::cat_pos_x_clock : Display::cat_pos_x;
    const uint16_t cat_pos_y = clock
            ? frame_ <= slide_frames ? Display::cat_pos_y_clock + slide_frames - frame_: Display::cat_pos_y_clock - slide_frames + frame_ - 1
            : Display::cat_pos_y;

    switch (face) {
    case Face::look_straight:
        painter.drawBitmap(cat_pos_x, cat_pos_y, cat_wait_img, Align::top_left);
        ms_to_next_frame = randInRng(2000, 5000);;
        switch (randInRng(0, 7)) {
        case 0:
        case 1:
        case 2:
        case 3:
            face = Face::blink;
            break;
        case 4:
            face = Face::look_left;
            break;
        case 5:
            face = Face::look_right;
            break;
        case 6:
            face = Face::tongue_out;
            break;
        default:
            face = Face::look_straight;
        }
        break;
    case Face::look_left:
        painter.drawBitmap(cat_pos_x, cat_pos_y, cat_wait_img, Align::top_left);
        painter.drawBitmap(cat_pos_x + 6, cat_pos_y + 19, cat_eyes_left_img, Align::top_left);
        face = (randInRng(0, 5) == 0) ? Face::look_right : Face::look_straight;
        ms_to_next_frame = randInRng(400, 1500);
        break;
    case Face::look_right:
        painter.drawBitmap(cat_pos_x, cat_pos_y, cat_wait_img, Align::top_left);
        painter.drawBitmap(cat_pos_x + 6, cat_pos_y + 19, cat_eyes_right_img, Align::top_left);
        face = (randInRng(0, 5) == 0) ? Face::look_left :  Face::look_straight;
        ms_to_next_frame = randInRng(400, 1500);
        break;
    case Face::tongue_out:
        painter.drawBitmap(cat_pos_x, cat_pos_y, cat_tongue_out_img, Align::top_left);
        face = Face::look_straight;
        ms_to_next_frame = randInRng(600, 1000);
        break;
    case Face::blink:
        painter.drawBitmap(cat_pos_x, cat_pos_y, cat_wait_img, Align::top_left);
        painter.drawBitmap(cat_pos_x + 4, cat_pos_y + 19, cat_eyes_close_img, Align::top_left);
        face = Face::look_straight;
        ms_to_next_frame = randInRng(150, 400);
        break;
    }

//#define TESTING_CAT_ANIM
#ifdef TESTING_CAT_ANIM
    constexpr uint32_t cat_visible_min_time_ms =  5'000;
    constexpr uint32_t cat_visible_max_time_ms =  6'000;
    constexpr uint32_t cat_hidden_min_time_ms =  5'000;
    constexpr uint32_t cat_hidden_max_time_ms =  6'000;
#else
    constexpr uint32_t cat_visible_min_time_ms =  10'000;
    constexpr uint32_t cat_visible_max_time_ms =  90'000;
    constexpr uint32_t cat_hidden_min_time_ms =  200'000;
    constexpr uint32_t cat_hidden_max_time_ms =  900'000;
#endif

    if (clock) {
        // Frames:
        // 0->47 : sliding up
        // 48    : waiting
        // 49-96 : sliding down
        if (frame_ == 0) {
            hide_in_ms = randInRng(cat_visible_min_time_ms, cat_visible_max_time_ms);
            //hide_time_ms = now_ms + randInRng(cat_visible_min_time_ms, cat_visible_max_time_ms);
         }
        // Move to slide down mode if it's time

        //if (frame_ == slide_frames && now_ms >= hide_time_ms) {
        if (frame_ == slide_frames && hide_in_ms <= 0) {
            frame_++;
        }
        // Advance frame if we're still sliding up/down
        if (frame_ != slide_frames) {
            frame_++;
            ms_to_next_frame = 5;
            face = frame_ == slide_frames ? Face::tongue_out : Face::look_straight;
        }
        // Stay hidden until next appearance time
        if (frame_ == (slide_frames * 2 + 3) ) {
            frame_ = 0;
            ms_to_next_frame = randInRng(cat_hidden_min_time_ms, cat_hidden_max_time_ms);
            painter.drawLineHoriz(cat_pos_x, cat_pos_y - 1, cat_wait_img.width, 0);
        }
        // Delete prior image if we are sliding down
        if (frame_ > slide_frames) {
            painter.drawLineHoriz(cat_pos_x, cat_pos_y - 1, cat_wait_img.width, 0);
        }
        hide_in_ms -= ms_to_next_frame;
    }
    return true;
}

static bool doWaiting(
        PrimitivesRender<uint8_t>& painter,
        AnimationRender::time_ms_t& ms_to_next_frame,
        AnimationRender::frame_count_t& frame_)
{
    return doWaitingActual(painter, ms_to_next_frame, frame_, false);
}

static bool doClock(
        PrimitivesRender<uint8_t>& painter,
        AnimationRender::time_ms_t& ms_to_next_frame,
        AnimationRender::frame_count_t& frame_)
{
    return doWaitingActual(painter, ms_to_next_frame, frame_, true);
}

static bool doDisconnect(
        PrimitivesRender<uint8_t>& painter,
        AnimationRender::time_ms_t& ms_to_next_frame,
        AnimationRender::frame_count_t& frame_)
{
    painter.drawRectSolid(Display::cat_pos_x - 6, 0, Display::width - Display::cat_pos_x + 6, Display::height, 0x0);
    switch (frame_) {
    case 0:
        painter.drawBitmap(Display::cat_pos_x, 9, cat_shocked_img, Align::top_left);
        painter.drawRectSolid(0, 0, 50, 64, 0x0);
        painter.drawBitmap(2, 20, bt_connected_explode1_img, Align::top_left);
        ms_to_next_frame = 120;
        frame_++;
        break;
    case 1:
        painter.drawBitmap(Display::cat_pos_x, 9, cat_shocked_img, Align::top_left);
        painter.drawRectSolid(0, 0, 50, 64, 0x0);
        painter.drawBitmap(0, 16, bt_connected_explode2_img, Align::top_left);
        ms_to_next_frame = 120;
        frame_++;
        break;
    case 2:
        painter.drawRectSolid(0, 0, 50, 64, 0x0);
        painter.drawBitmap(Display::cat_pos_x, 9, cat_shocked_img, Align::top_left);
        ms_to_next_frame = 1000;
        frame_++;
        break;
    default:
        static constexpr uint8_t show_paws_frame = 15;
        if (frame_ < show_paws_frame) {
            painter.drawBitmap(Display::cat_pos_x, 9 + frame_ - 2, cat_shocked_img, Align::top_left);
            ms_to_next_frame = 40;
        }
        else {
            painter.drawBitmap(Display::cat_pos_x - 6, 9 + frame_ - 14, cat_with_paws_img, Align::top_left);

            uint8_t line_len = frame_ - show_paws_frame;
            painter.drawLineVert(Display::cat_pos_x - 5, 16, line_len, 0x8);
            painter.drawLineVert(Display::cat_pos_x, 12, line_len, 0x8);
            painter.drawLineVert(Display::cat_pos_x + 7, 12, line_len, 0x8);
            painter.drawLineVert(Display::cat_pos_x + 12, 17, line_len, 0x8);

            painter.drawLineVert(37 + Display::cat_pos_x - 5, 16, line_len, 0x8);
            painter.drawLineVert(37 + Display::cat_pos_x, 12, line_len, 0x8);
            painter.drawLineVert(37 + Display::cat_pos_x + 7, 12, line_len, 0x8);
            painter.drawLineVert(37 + Display::cat_pos_x + 12, 17, line_len, 0x8);

            static constexpr uint8_t accel = 25;
            ms_to_next_frame = std::max(40, 200 - (frame_ - show_paws_frame) * accel);
        }
        frame_++;
        if (frame_ == 70) {
            ms_to_next_frame = 3000;
        }
        else if (frame_ == 71) {
            painter.drawRectSolid(Display::cat_pos_x - 6, 0, Display::width - Display::cat_pos_x + 6, Display::height, 0x0);
            frame_ = 0;
            return false;
        }
        break;
    }
    return true;
}

static void drawCircle(PrimitivesRender<uint8_t>& painter, uint16_t x, uint16_t y, uint8_t& frame)
{
    constexpr uint8_t start_radius = 6;
    uint8_t color = (frame >= 15) ? 30 - frame : frame;
    if (frame > 0) {
        painter.drawCircleThick(x, y, start_radius + frame - 1, 2, 0x0);
    }
    painter.drawCircleThick(x, y, start_radius + frame, 2, color);
    frame++;
    if (frame == 31) {
        frame = 0;
    }
}

static constexpr uint32_t pairing_frame_rate_ms = 80;
static bool doPairing(
        PrimitivesRender<uint8_t>& painter,
        AnimationRender::time_ms_t& ms_to_next_frame,
        AnimationRender::frame_count_t& frame)
{
    static constexpr uint16_t x = Display::mgn_left + bt_connected_img.width / 2;
    static constexpr uint16_t y = Display::center_y;
    static uint8_t frame2;
    frame2 = (frame >= 21) ? frame - 21 : frame + 10;

    drawCircle(painter, x, y, frame2);
    drawCircle(painter, x, y, frame);
    // Draw in the waiting animation, but circles will override so redraw
    painter.drawBitmap(x, y, bt_connected_img, Align::middle_center);
    ms_to_next_frame = pairing_frame_rate_ms;
    return true;
}

AnimationRender::Animation AnimationRender::anim_listen{doListen};
AnimationRender::Animation AnimationRender::anim_waiting{doWaiting};
AnimationRender::Animation AnimationRender::anim_disconnect{doDisconnect};
AnimationRender::Animation AnimationRender::anim_pairing{doPairing};
AnimationRender::Animation AnimationRender::anim_clock{doClock};
