#include "display.h"

const Libp::RasterFont<32, 126>& Display::font_ = roboto_condensed_regular_14_2;

/**
 * Update artist/track name text.
 *
 * @param artist [in,out] Null terminated artist name. length will be
 *                        reduced and "..." suffix added if required.
 * @param track  [in,out] Null terminated track name. length will be
 *                        reduced and "..." suffix added if required.
 */
void Display::drawMetaText(char* artist, char* track)
{
    const uint16_t available_width = end_text_x_pos - Libp::enumBaseT(TextPos::now_playing);
    text_painter_.trimText(font_, artist, available_width);
    text_painter_.trimText(font_, track, available_width);
    drawText(TextPos::now_playing, artist, track);
}


void Display::drawText(TextPos pos, const char* line1, const char* line2)
{
    const uint16_t x_pos = Libp::enumBaseT(pos);
    const uint16_t available_width = (pos == TextPos::fullscreen ? width : end_text_x_pos) - x_pos;

    if (pos == TextPos::fullscreen)
        disp_buffer_.fillScreen(0x0);
    else
        painter_.drawRectSolid(x_pos, 0, available_width, height, 0x0);

    Libp::Align align = (pos == TextPos::fullscreen) ? Libp::Align::middle_center : Libp::Align::middle_left;

    getErrHndlr().report("display_text: '%s', '%s'\r\n", line1, line2);

    text_painter_.drawText(x_pos, text_pos_y_row1, font_, line1, align, text_colors);
    text_painter_.drawText(x_pos, text_pos_y_row2, font_, line2, align, text_colors);
    flush();
}

void Display::drawClock(TimeData& time_data, bool incl_year)
{
    static const char* day_names[] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
    static const char* month_names[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };

    constexpr uint8_t max_len = std::max(sizeof("99-Mmm-99 (Ddd)"), sizeof("12.3  / 12%"));
    char buf[max_len];

    uint8_t day_of_week_idx = Libp::log2_8(time_data.day_of_week);
    if (incl_year) {
        snprintf(buf, max_len, "%d-%s-%02d (%s)",
                time_data.day_of_month,
                month_names[time_data.month - 1],
                time_data.year,
                day_names[day_of_week_idx]);
    }
    else {
        snprintf(buf, max_len, "%d-%s (%s)",
                time_data.day_of_month,
                month_names[time_data.month - 1],
                day_names[day_of_week_idx]);
    }
    text_painter_.drawText(mgn_left, mgn_top, font_, buf, Libp::Align::top_left, text_colors);

    const uint8_t display_hours = time_data.hours == 0
            ? 12
            : time_data.hours > 12 ? time_data.hours - 12 : time_data.hours;

    snprintf(buf, max_len, "%d:%02d", display_hours, time_data.minutes);
    uint16_t offs = text_painter_.drawText(
            mgn_left,
            height - mgn_bottom - text_height - 1,
            font_, buf, Libp::Align::top_left, text_colors);
    text_painter_.drawText(
            offs,
            height - mgn_bottom - text_height - 1,
            font_, time_data.hours < 12 ? " am" : " pm", Libp::Align::top_left, text_colors);
}


void Display::drawWeather(EnvData& env_data)
{
    constexpr uint8_t max_len = std::max(sizeof("123.4d / 99%"), sizeof("1234 hPa"));
    char buf[max_len];

    snprintf(buf, max_len, "%d.%d  / %d%%",
            static_cast<int>(env_data.temperature / 100),
            static_cast<int>(env_data.temperature % 100 / 10),
            static_cast<int>(env_data.humidity / 1000));
    text_painter_.drawText(
            width - mgn_right,
            mgn_top,
            font_, buf, Libp::Align::top_right, text_colors);

    constexpr uint16_t deg_sym_x_pos = width - mgn_right - 44;
    painter_.drawCircleThick(
            env_data.humidity >= 10000 ? deg_sym_x_pos : deg_sym_x_pos + 8,
            mgn_top + 5, 3, 2, 0xf);

    snprintf(buf, max_len, "%d hPa",
            static_cast<int>(env_data.pressure / 100));
    text_painter_.drawText(
            width - mgn_right,
            height - mgn_bottom - text_height - 1,
            font_, buf, Libp::Align::top_right, text_colors);
}


/**
 * Draw the time/date and highlight a specific field
 *
 * @param date_time
 * @param highlight
 */
void Display::drawAdjustClock(TimeData& time_data, ClockField highlight)
{
    disp_buffer_.fillScreen(0x0);

    drawClock(time_data, true);

    // Button images

    constexpr uint16_t btn_mgn_x = 4;
    constexpr uint16_t btn_mgn_y = 10;

    uint16_t x = 200;
    uint16_t y = 5;

    painter_.drawBitmap(x, y, btn_vol_up_img, Libp::Align::top_left);
    y += btn_mgn_y + btn_play_img.height;
    painter_.drawBitmap(x, y, btn_vol_dn_img, Libp::Align::top_left);
    y -= (btn_play_img.height + btn_mgn_y)/2;
    x = x - btn_play_img.width - btn_mgn_x;
    painter_.drawBitmap(x, y, btn_prev_img, Libp::Align::top_left);
    x += btn_play_img.width + btn_play_img.width + btn_mgn_x + btn_mgn_x;
    painter_.drawBitmap(x, y, btn_next_img, Libp::Align::top_left);

    // Highlight field

    struct LinParams {
        const uint8_t x_pos;
        const uint8_t y_pos;
        const uint8_t len;
    };
    /// Top/bottom lines to highlight field being edited
    static LinParams lines[] = {
            {  5,  3, 10 }, // day of month
            { 30,  3, 10 }, // month
            { 56,  3, 10 }, // year
            {  4, 36, 10 }, // hour
            { 23, 36, 10 }, // minute
    };
    const uint8_t field_idx =  Libp::enumBaseT(highlight);
    painter_.drawLineHoriz(
            lines[field_idx].x_pos,
            lines[field_idx].y_pos,
            lines[field_idx].len, 0xf);
    painter_.drawLineHoriz(
            lines[field_idx].x_pos,
            lines[field_idx].y_pos + 24,
            lines[field_idx].len, 0xf);

    flush();
}


void Display::drawMenu()
{
    menu_mode_ = true;

    disp_buffer_.fillScreen(0x0);
    const uint16_t text_left_mgn = 6;
    const uint16_t text_offset_x = btn_prev_img.width + text_left_mgn;

    uint16_t x = 4;
    uint16_t y = 5;
    painter_.drawBitmap(x, y, btn_prev_img, Libp::Align::top_left);
    text_painter_.drawText(x + text_offset_x, 4, font_,  "Back", Libp::Align::top_left, text_colors);
    x = 84;
    painter_.drawBitmap(x, y, btn_play_img, Libp::Align::top_left);
    text_painter_.drawText(x + text_offset_x, 4, font_,  "Pair", Libp::Align::top_left, text_colors);
    x = 156;
    painter_.drawBitmap(x, y, btn_next_img, Libp::Align::top_left);
    text_painter_.drawText(x + text_offset_x, 4, font_, "Set clock", Libp::Align::top_left, text_colors);
    y = 38;
    x = 28;
    painter_.drawBitmap(x, y, btn_vol_dn_img, Libp::Align::top_left);
    text_painter_.drawText(x + text_offset_x, 37, font_, "Reboot", Libp::Align::top_left, text_colors);
    x = 132;
    painter_.drawBitmap(x, y, btn_vol_up_img, Libp::Align::top_left);
    text_painter_.drawText(x + text_offset_x, 37, font_, "Clear pairs", Libp::Align::top_left, text_colors);

    flush();
}
