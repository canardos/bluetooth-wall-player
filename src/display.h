#ifndef SRC_DISPLAY_H_
#define SRC_DISPLAY_H_

#include "graphics/idrawing_surface.h"
#include "graphics/text_render.h"
#include "graphics/primitives_render.h"
#include "graphics/display_buffer.h"

#include "animation_render.h"
#include <error_handler.h>
#include "app.h"

#include "data/font_dat.h"
#include "data/image_data.h"

#include "devices/rtc.h"
#include "devices/sensors.h"

enum class ClockField : uint8_t {
    // DO NOT REORDER or assign values
    day_of_month,
    month,
    year,
    hours,
    minutes,
};
// ++/-- operators with wraparound
inline ClockField& operator ++(ClockField& field)
{
    return field = (field == ClockField::minutes)
            ? ClockField::day_of_month
            : static_cast<ClockField>(Libp::enumBaseT(field) + 1);
}
inline ClockField& operator --(ClockField& field)
{
    return field = (field == ClockField::day_of_month)
            ? ClockField::minutes
            : static_cast<ClockField>(Libp::enumBaseT(field) - 1);
}

/**
 * Display output class.
 *
 * Provides display output functions and manages animations.
 *
 * `update` must be called regularly when animations are active.
 */
class Display {
public:
    static constexpr uint16_t width = 256;
    static constexpr uint16_t height = 64;

    static constexpr uint16_t mgn_left = 2;
    static constexpr uint16_t mgn_right = 4;
    static constexpr uint16_t mgn_top = 4;
    static constexpr uint16_t mgn_bottom = 4;
    static constexpr uint16_t text_height = 22;

    static constexpr uint16_t mgn_bt_icon_to_text = 8;
    static constexpr uint16_t center_y = height / 2;
    static constexpr uint16_t text_row_offset_y = 14;

    static constexpr uint16_t text_pos_x_playing = mgn_left + bt_connected_img.width + mgn_bt_icon_to_text;
    static constexpr uint16_t text_pos_x_pairing = 50;

    static constexpr uint16_t text_pos_y_row1 = center_y - text_row_offset_y;
    static constexpr uint16_t text_pos_y_row2 = center_y + text_row_offset_y;

    static constexpr uint16_t cat_pos_x = 194;
    static constexpr uint16_t cat_pos_y = 9;
    static constexpr uint16_t cat_pos_x_clock = 110;
    static constexpr uint16_t cat_pos_y_clock = 10;
    static constexpr uint16_t cat_text_mgn = 5;
    static constexpr uint16_t end_text_x_pos = cat_pos_x - cat_text_mgn;
    static constexpr uint16_t main_text_width = cat_pos_x - text_pos_x_playing;

    enum class TextPos : uint16_t {
        fullscreen = width / 2,           ///< fullscreen centered
        now_playing = text_pos_x_playing, ///
        pairing = text_pos_x_pairing
    };

    using PixelType = uint8_t;

    Display(Libp::IDrawingSurface<PixelType>& oled)
            : oled_(oled), painter_(disp_buffer_), text_painter_(disp_buffer_), anim_render_(painter_) { }

     ///
    void notifyNewModuleState(ModuleState new_state, ModuleState old_state)
    {
        if (!menu_mode_) {
            disp_buffer_.fillScreen(0x0);
            anim_render_.startAnimations(new_state, old_state);
            if (anim_render_.update())
                flush();
        }
    }

    /**
     * Must be called regularly when animations are active
     */
    void update()
    {
        if (!menu_mode_) {
            if (anim_render_.update())
                flush();
        }
    }

    /**
     * @return true if the disconnect animation has completed
     */
    bool disconnectAnimIsDone()
    {
        return !anim_render_.disconnectAnimIsActive();
    }


    /**
     * Update artist/track name text.
     *
     * @param artist [in,out] Null terminated artist name. length will be
     *                        reduced and "..." suffix added if required.
     * @param track  [in,out] Null terminated track name. length will be
     *                        reduced and "..." suffix added if required.
     */
    void drawMetaText(char* artist, char* track);
    void drawText(TextPos pos, const char* line1, const char* line2);
    void drawAdjustClock(TimeData& time_data, ClockField highlight);

    /**
     * Draw the time/date and temp/humidity/pressure information.
     *
     * @param date_time
     * @param env_data
     */
    void drawClockAndWeather(TimeData& date_time, EnvData& env_data)
    {
        if (menu_mode_)
            return;

        // Don't clear entire screen or animation will be overwritten
        constexpr uint16_t cat_start_x = cat_pos_x_clock;
        constexpr uint16_t cat_stop_x  = cat_pos_x_clock + cat_wait_img.width;
        disp_buffer_.fillRect(0, 0, cat_start_x, height, 0);
        disp_buffer_.fillRect(cat_stop_x, 0, width - cat_stop_x, height, 0);

        drawClock(date_time, false);
        drawWeather(env_data);
        flush();
    }

    void drawMenu();

    void exitMenu()
    {
        disp_buffer_.fillScreen(0x0);
        flush();
        menu_mode_ = false;
    }


private:
    Libp::DisplayBuf4bpp<width, height> disp_buffer_;
    Libp::IDrawingSurface<PixelType>& oled_;
    Libp::PrimitivesRender<PixelType> painter_;
    Libp::TextRender<PixelType> text_painter_;
    AnimationRender anim_render_;

    // Binary ends up with 2 copies of roboto_condensed_regular_14_2 if we do this...:O
    //static inline constexpr const Libp::RasterFont<32, 126>& font_ = roboto_condensed_regular_14_2;
    static const Libp::RasterFont<32, 126>& font_;

    /// 4-bit colors for 2bpp font
    static constexpr PixelType text_colors[] = {0x0c, 0x0d, 0x0e, 0x0f};

    void drawClock(TimeData& time_data, bool incl_year);
    void drawWeather(EnvData& env_data);

    /// Normal animations don't draw in menu mode
    bool menu_mode_ = false;

    /// Flush display buffer to OLED
    void flush()
    {
        oled_.copyRect(0, 0, width, height, disp_buffer_.buffer());
    }
};

#endif /* SRC_DISPLAY_H_ */
