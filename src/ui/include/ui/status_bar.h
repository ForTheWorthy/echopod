#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "gfx/canvas.h"
#include "ui/theme.h"

typedef enum StatusPlayback {
    STATUS_PLAYBACK_NONE,
    STATUS_PLAYBACK_PLAYING,
    STATUS_PLAYBACK_PAUSED
} StatusPlayback;

typedef enum StatusBluetooth {
    STATUS_BLUETOOTH_OFF,
    STATUS_BLUETOOTH_ON,
    STATUS_BLUETOOTH_CONNECTED
} StatusBluetooth;

enum {
    STATUS_BAR_LOW_BATTERY_PERCENT = 15,
    STATUS_BAR_PERCENT_CAPACITY = 8,
    STATUS_BAR_SLEEP_CAPACITY = 8,
    STATUS_BAR_LAST_MINUTE_MS = 60000
};

typedef struct StatusBarState {
    StatusPlayback playback;
    StatusBluetooth bluetooth;
    bool battery_known;
    uint8_t battery_percent;
    bool charging;
    bool hide_percent;
    bool sleep_active;
    uint32_t sleep_remaining_ms;
} StatusBarState;

Rect status_bar_playback_area(const Theme *theme);
Rect status_bar_bluetooth_area(const Theme *theme);
Rect status_bar_battery_area(const Theme *theme);
int status_bar_battery_fill_width(uint8_t percent, int inner_width);
void status_bar_percent_text(uint8_t percent, char *text, size_t capacity);
void status_bar_sleep_text(uint32_t remaining_ms, char *text, size_t capacity);
Color status_bar_sleep_color(const Theme *theme, uint32_t remaining_ms);
Rect status_bar_bolt_area(const Theme *theme);
Rect status_bar_percent_area(const Theme *theme, const char *text, bool charging);
Rect status_bar_sleep_area(const Theme *theme, const char *text, bool battery_known, bool charging,
                           bool show_percent, const char *percent);
void status_bar_draw(Canvas *canvas, const Theme *theme, StatusBarState state);
