#include "ui/status_bar.h"

#include "core/format.h"
#include "core/string_builder.h"
#include "gfx/color.h"
#include "gfx/text.h"
#include "ui/icons.h"
#include "ui/label.h"

enum {
    BORDER = 1,
    FILL_INSET = 2,
    NUB_DIVISOR = 5,
    BOLT_WIDTH_TENTHS = 6,
    BLUETOOTH_WIDTH_TENTHS = 7,
    TENTHS = 10,
    FULL_PERCENT = 100,
    MILLISECONDS_PER_SECOND = 1000,
    SECONDS_PER_MINUTE = 60,
    MINUTES_PER_HOUR = 60,
    MINUTE_TEXT_LIMIT = 10,
    HOUR_TEXT_LIMIT = 100,
    MILLISECONDS_PER_MINUTE = MILLISECONDS_PER_SECOND * SECONDS_PER_MINUTE
};

static int icon_top(const Theme *theme)
{
    int band = theme->metrics.status_height > 0 ? theme->metrics.status_height
                                                : theme->metrics.header_height;
    return (band - theme->metrics.status_icon_size) / 2;
}

static int nub_width(int size)
{
    return size / NUB_DIVISOR > 0 ? size / NUB_DIVISOR : 1;
}

Rect status_bar_playback_area(const Theme *theme)
{
    int size = theme->metrics.status_icon_size;
    return (Rect){theme->metrics.padding, icon_top(theme), size, size};
}

Rect status_bar_bluetooth_area(const Theme *theme)
{
    Rect playback = status_bar_playback_area(theme);
    int width = playback.width * BLUETOOTH_WIDTH_TENTHS / TENTHS;
    return (Rect){playback.x + playback.width + theme->metrics.padding / 2, playback.y,
                  width > 0 ? width : 1, playback.height};
}

Rect status_bar_battery_area(const Theme *theme)
{
    int size = theme->metrics.status_icon_size;
    int width = 2 * size + nub_width(size);
    return (Rect){theme->metrics.screen_width - theme->metrics.padding - width, icon_top(theme),
                  width, size};
}

int status_bar_battery_fill_width(uint8_t percent, int inner_width)
{
    int clamped = percent < FULL_PERCENT ? percent : FULL_PERCENT;
    int width = clamped * inner_width / FULL_PERCENT;
    return clamped > 0 && width == 0 ? 1 : width;
}

void status_bar_percent_text(uint8_t percent, char *text, size_t capacity)
{
    StringBuilder builder = string_builder_make(text, capacity);
    string_builder_append_unsigned(&builder, percent < FULL_PERCENT ? percent : FULL_PERCENT);
    string_builder_append_char(&builder, '%');
}

void status_bar_sleep_text(uint32_t remaining_ms, char *text, size_t capacity)
{
    uint32_t minutes = remaining_ms / MILLISECONDS_PER_MINUTE;
    if (minutes < MINUTE_TEXT_LIMIT) {
        format_duration(remaining_ms, text, capacity);
        return;
    }
    if (remaining_ms % MILLISECONDS_PER_MINUTE != 0) {
        minutes++;
    }
    StringBuilder builder = string_builder_make(text, capacity);
    if (minutes < HOUR_TEXT_LIMIT) {
        string_builder_append_unsigned(&builder, minutes);
        string_builder_append_char(&builder, 'm');
        return;
    }
    string_builder_append_unsigned(&builder, minutes / MINUTES_PER_HOUR);
    string_builder_append_char(&builder, 'h');
    uint32_t rest = minutes % MINUTES_PER_HOUR;
    if (rest > 0) {
        string_builder_append_unsigned(&builder, rest);
    }
}

Color status_bar_sleep_color(const Theme *theme, uint32_t remaining_ms)
{
    if (remaining_ms >= STATUS_BAR_LAST_MINUTE_MS) {
        return theme->colors.header_text;
    }
    if (theme->colors.selection_shine != theme->colors.selection_background) {
        return theme->colors.selection_shine;
    }
    return color_between(theme->colors.selection_background, theme->colors.header_text, 1, 2);
}

static int percent_gap(const Theme *theme)
{
    return theme->metrics.padding / 2 > 0 ? theme->metrics.padding / 2 : 1;
}

Rect status_bar_bolt_area(const Theme *theme)
{
    Rect battery = status_bar_battery_area(theme);
    int width = theme->metrics.status_icon_size * BOLT_WIDTH_TENTHS / TENTHS;
    int bolt_width = width > 0 ? width : 1;
    return (Rect){battery.x - percent_gap(theme) - bolt_width, battery.y, bolt_width,
                  battery.height};
}

Rect status_bar_percent_area(const Theme *theme, const char *text, bool charging)
{
    Rect beside = charging ? status_bar_bolt_area(theme) : status_bar_battery_area(theme);
    int width = text_width(theme->fonts.small, text);
    return (Rect){beside.x - percent_gap(theme) - width, beside.y, width, beside.height};
}

Rect status_bar_sleep_area(const Theme *theme, const char *text, bool battery_known, bool charging,
                           bool show_percent, const char *percent)
{
    Rect battery = status_bar_battery_area(theme);
    int width = text_width(theme->fonts.small, text);
    if (!battery_known) {
        return (Rect){theme->metrics.screen_width - theme->metrics.padding - width, battery.y,
                      width, battery.height};
    }
    Rect anchor = show_percent ? status_bar_percent_area(theme, percent, charging)
                               : (charging ? status_bar_bolt_area(theme) : battery);
    return (Rect){anchor.x - percent_gap(theme) - width, anchor.y, width, anchor.height};
}

static void draw_percent(Canvas *canvas, const Theme *theme, uint8_t percent, bool charging)
{
    char text[STATUS_BAR_PERCENT_CAPACITY];
    status_bar_percent_text(percent, text, sizeof text);
    Rect area = status_bar_percent_area(theme, text, charging);
    TextStyle style = {.font = theme->fonts.small, .color = theme->colors.header_text};
    label_draw(canvas, style,
               (Label){.area = label_centered_area(style.font, area, text), .text = text});
}

static void draw_sleep(Canvas *canvas, const Theme *theme, StatusBarState state)
{
    char sleep[STATUS_BAR_SLEEP_CAPACITY];
    char percent[STATUS_BAR_PERCENT_CAPACITY];
    status_bar_sleep_text(state.sleep_remaining_ms, sleep, sizeof sleep);
    status_bar_percent_text(state.battery_percent, percent, sizeof percent);
    bool show_percent = state.battery_known && !state.hide_percent;
    Rect area = status_bar_sleep_area(theme, sleep, state.battery_known, state.charging,
                                      show_percent, percent);
    Color color = status_bar_sleep_color(theme, state.sleep_remaining_ms);
    TextStyle style = {.font = theme->fonts.small, .color = color};
    label_draw(canvas, style,
               (Label){.area = label_centered_area(style.font, area, sleep), .text = sleep});
}

static Color fill_color(const Theme *theme, StatusBarState state)
{
    if (state.charging) {
        return theme->colors.success;
    }
    return state.battery_percent <= STATUS_BAR_LOW_BATTERY_PERCENT ? theme->colors.danger
                                                                   : theme->colors.header_text;
}

static void draw_outline(Canvas *canvas, Rect body, Color color)
{
    canvas_fill_rect(canvas, (Rect){body.x, body.y, body.width, BORDER}, color);
    canvas_fill_rect(canvas, (Rect){body.x, body.y + body.height - BORDER, body.width, BORDER},
                     color);
    canvas_fill_rect(canvas, (Rect){body.x, body.y, BORDER, body.height}, color);
    canvas_fill_rect(canvas, (Rect){body.x + body.width - BORDER, body.y, BORDER, body.height},
                     color);
}

static void draw_battery(Canvas *canvas, const Theme *theme, StatusBarState state)
{
    Rect area = status_bar_battery_area(theme);
    int size = theme->metrics.status_icon_size;
    Rect body = {area.x, area.y, 2 * size, size};
    int nub = nub_width(size);
    Rect tip = {body.x + body.width, area.y + size / 3, nub, size - 2 * (size / 3)};
    Rect inner = {body.x + FILL_INSET, body.y + FILL_INSET, body.width - 2 * FILL_INSET,
                  body.height - 2 * FILL_INSET};
    Color color = theme->colors.header_text;
    draw_outline(canvas, body, color);
    canvas_fill_rect(canvas, tip, color);
    inner.width = status_bar_battery_fill_width(state.battery_percent, inner.width);
    canvas_fill_rect(canvas, inner, fill_color(theme, state));
}

void status_bar_draw(Canvas *canvas, const Theme *theme, StatusBarState state)
{
    Rect icon = status_bar_playback_area(theme);
    Color color = theme->colors.header_text;
    if (state.playback == STATUS_PLAYBACK_PLAYING) {
        icon_draw_play(canvas, icon, color);
    } else if (state.playback == STATUS_PLAYBACK_PAUSED) {
        icon_draw_pause(canvas, icon, color);
    }
    if (state.bluetooth != STATUS_BLUETOOTH_OFF) {
        Color tint =
            state.bluetooth == STATUS_BLUETOOTH_CONNECTED ? color : theme->colors.text_tertiary;
        icon_draw_bluetooth(canvas, status_bar_bluetooth_area(theme), tint);
    }
    if (state.battery_known) {
        draw_battery(canvas, theme, state);
        if (state.charging) {
            icon_draw_bolt(canvas, status_bar_bolt_area(theme), theme->colors.success);
        }
        if (!state.hide_percent) {
            draw_percent(canvas, theme, state.battery_percent, state.charging);
        }
    }
    if (state.sleep_active) {
        draw_sleep(canvas, theme, state);
    }
}
