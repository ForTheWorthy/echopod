#include "check.h"
#include "default_theme.h"
#include "gfx/canvas.h"
#include "gfx/text.h"
#include "hal/display.h"
#include "ui/header.h"
#include "ui/status_bar.h"

static Color before_pixels[HAL_DISPLAY_LONG_SIDE * HAL_DISPLAY_LONG_SIDE];
static Color after_pixels[HAL_DISPLAY_LONG_SIDE * HAL_DISPLAY_LONG_SIDE];

static Rect title_rect(const Theme *theme, const char *title)
{
    int inset = theme->metrics.header_inset > theme->metrics.padding ? theme->metrics.header_inset
                                                                     : theme->metrics.padding;
    Rect row = theme_title_area(theme);
    int width = text_width(theme->fonts.title, title);
    int box = row.width - 2 * inset;
    if (width > box) {
        width = box;
    }
    return (Rect){row.x + inset + (box - width) / 2, row.y, width, row.height - 1};
}

static Canvas make_canvas(Color *pixels, const Theme *theme)
{
    return canvas_make(pixels,
                       (Rect){0, 0, theme->metrics.screen_width, theme->metrics.screen_height});
}

static void draw_header(Canvas *canvas, const Theme *theme, const char *title)
{
    canvas_clear(canvas, theme->colors.background);
    header_draw(canvas, theme, title);
}

static bool same_pixels(const Canvas *before, const Canvas *after, Rect area)
{
    for (int y = area.y; y < area.y + area.height; y++) {
        for (int x = area.x; x < area.x + area.width; x++) {
            if (canvas_pixel_at(before, x, y) != canvas_pixel_at(after, x, y)) {
                return false;
            }
        }
    }
    return true;
}

static bool contains_color(const Canvas *canvas, Rect area, Color color)
{
    for (int y = area.y; y < area.y + area.height; y++) {
        for (int x = area.x; x < area.x + area.width; x++) {
            if (canvas_pixel_at(canvas, x, y) == color) {
                return true;
            }
        }
    }
    return false;
}

static StatusBarState countdown(uint32_t remaining_ms, uint8_t percent, bool charging)
{
    return (StatusBarState){
        .battery_known = true,
        .battery_percent = percent,
        .charging = charging,
        .sleep_active = true,
        .sleep_remaining_ms = remaining_ms,
    };
}

static void the_countdown_leaves_the_centered_title_untouched(const Theme *theme, const char *title,
                                                              StatusBarState state)
{
    Canvas before = make_canvas(before_pixels, theme);
    Canvas after = make_canvas(after_pixels, theme);
    draw_header(&before, theme, title);
    draw_header(&after, theme, title);
    Rect title_area = title_rect(theme, title);
    int center = title_area.x + title_area.width / 2;
    int screen_center = theme->metrics.screen_width / 2;
    CHECK(center == screen_center || center + 1 == screen_center);
    CHECK(contains_color(&before, title_area, theme->colors.header_text));
    status_bar_draw(&after, theme, state);
    CHECK(same_pixels(&before, &after, title_area));
    char sleep[STATUS_BAR_SLEEP_CAPACITY];
    char percent[STATUS_BAR_PERCENT_CAPACITY];
    status_bar_sleep_text(state.sleep_remaining_ms, sleep, sizeof sleep);
    status_bar_percent_text(state.battery_percent, percent, sizeof percent);
    Rect sleep_area = status_bar_sleep_area(theme, sleep, true, state.charging, true, percent);
    if (theme->metrics.status_height == 0) {
        CHECK(sleep_area.x >= title_area.x + title_area.width);
    } else {
        CHECK(sleep_area.y + sleep_area.height <= theme->metrics.status_height);
    }
    CHECK(!same_pixels(&before, &after, sleep_area));
}

static void now_playing_stays_centered_beside_every_countdown(void)
{
    Theme theme = default_theme_for(false);
    the_countdown_leaves_the_centered_title_untouched(&theme, "Now Playing",
                                                      countdown(15 * 60 * 1000, 80, false));
    the_countdown_leaves_the_centered_title_untouched(&theme, "Now Playing",
                                                      countdown(40 * 60 * 1000, 100, true));
    the_countdown_leaves_the_centered_title_untouched(&theme, "Now Playing",
                                                      countdown(120 * 60 * 1000, 100, true));
    the_countdown_leaves_the_centered_title_untouched(&theme, "Now Playing",
                                                      countdown(42000, 80, false));
}

static void a_portrait_title_stays_centered_above_the_countdown(void)
{
    Theme theme = default_theme_for(true);
    the_countdown_leaves_the_centered_title_untouched(&theme, "Now Playing",
                                                      countdown(40 * 60 * 1000, 100, true));
}

int main(void)
{
    static const TestCase cases[] = {
        TEST_CASE(now_playing_stays_centered_beside_every_countdown),
        TEST_CASE(a_portrait_title_stays_centered_above_the_countdown),
    };
    return test_run_all(cases, TEST_CASE_COUNT(cases));
}
