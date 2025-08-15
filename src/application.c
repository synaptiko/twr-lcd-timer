#include <application.h>
#include <time.h>
#include <string.h>

// TODO: build & flash non-debug version to the device
// TODO: investigate and maybe fix clearing of alarm state when dismissed by click

#define COLOR_BLACK true
#define COLOR_WHITE false

#define FONT (&twr_font_ubuntu_33)
#define TEXT_HEIGHT 33
#define MAX_MINUTES 10
#define SECONDS_INCREMENT 10
#define COUNTDOWN_UPDATE_INTERVAL_MS 100
#define ALARM_UPDATE_INTERVAL_MS 250
#define ALARM_DURATION_MS 10000

typedef enum {
    STATE_SLEEP,
    STATE_TIME_SETTING,
    STATE_COUNTDOWN,
    STATE_ALARM
} app_state_t;

static struct {
    app_state_t state;
    uint64_t total_ms;
    uint16_t alarm_blink_tick;
    uint8_t minutes;
    uint8_t seconds;
    uint8_t progress;
    struct timespec start_time;
} app_ctx = {
    .state = STATE_SLEEP,
    .total_ms = 0,
    .alarm_blink_tick = 0,
    .minutes = 0,
    .seconds = 0,
    .progress = 0,
};

static twr_lis2dh12_alarm_t accelerometer_alarm = {
    .threshold = 2.0f,
    .duration = 5,
    .x_high = true,
    .y_high = true,
    .z_high = true
};
static twr_lis2dh12_t accelerometer;

static twr_led_t led_lcd_blue;

static twr_scheduler_task_id_t draw_task_id;
static twr_scheduler_task_id_t countdown_task_id;
static twr_scheduler_task_id_t alarm_task_id;

void draw_task(void *param);
void countdown_task(void *param);
void alarm_task(void *param);

int64_t get_remaining_ms();
void dismiss_alarm();
void dismiss_countdown();

void increment_minutes();
void increment_seconds();
void reset_minutes();
void reset_seconds();
void set_time(uint8_t minutes, uint8_t seconds);
void check_and_draw_time();

void draw_time_with_bg(char *time_str, uint8_t progress, bool inverted);

void draw_time_setting_state();
void draw_sleep_state();
void draw_countdown_state();
void draw_alarm_state();

void transition_to_time_setting_state();
void transition_to_sleep_state();
void transition_to_countdown_state();
void transition_to_alarm_state();

void draw_task(void *param) {
   (void) param;

    switch (app_ctx.state) {
        case STATE_SLEEP: {
            draw_sleep_state();
            break;
        }
        case STATE_TIME_SETTING: {
            draw_time_setting_state();
            break;
        }
        case STATE_COUNTDOWN: {
            draw_countdown_state();
            break;
        }
        case STATE_ALARM: {
            draw_alarm_state();
            break;
        }
        default:
            break;
    }
}

void countdown_task(void *param) {
    (void) param;

    int64_t remaining_ms = get_remaining_ms();

    if (remaining_ms <= 0) {
        app_ctx.minutes = 0;
        app_ctx.seconds = 0;
        app_ctx.progress = 0;

        transition_to_alarm_state();
    } else {
        uint8_t prev_minutes = app_ctx.minutes;
        uint8_t prev_seconds = app_ctx.seconds;
        uint8_t prev_progress = app_ctx.progress;

        int remaining_seconds = (int)(remaining_ms / 1000);

        uint8_t minutes = remaining_seconds / 60;
        uint8_t seconds = remaining_seconds % 60;
        uint8_t progress = 128 - (int)(((float)remaining_ms / (float)app_ctx.total_ms) * 128.0f);

        app_ctx.minutes = minutes;
        app_ctx.seconds = seconds;
        app_ctx.progress = progress;

        if (minutes != prev_minutes || seconds != prev_seconds || progress != prev_progress) {
            twr_scheduler_plan_now(draw_task_id);
        }
        twr_scheduler_plan_current_from_now(COUNTDOWN_UPDATE_INTERVAL_MS);
    }
}

void alarm_task(void *param) {
    (void) param;

    app_ctx.alarm_blink_tick++;

    if (get_remaining_ms() <= 0) {
        dismiss_alarm();
    } else {
        twr_scheduler_plan_now(draw_task_id);
        twr_scheduler_plan_current_from_now(ALARM_UPDATE_INTERVAL_MS);
    }
}

int64_t get_remaining_ms() {
    struct timespec current_time;

    twr_rtc_get_timestamp(&current_time);

    int64_t elapsed_ms = ((int64_t)(current_time.tv_sec - app_ctx.start_time.tv_sec) * 1000) +
                        ((current_time.tv_nsec - app_ctx.start_time.tv_nsec) / 1000000);

    return app_ctx.total_ms - elapsed_ms;
}

void dismiss_alarm() {
    twr_led_set_mode(&led_lcd_blue, TWR_LED_MODE_OFF);

    twr_scheduler_plan_absolute(alarm_task_id, TWR_TICK_INFINITY);

    transition_to_sleep_state();

    twr_scheduler_plan_now(draw_task_id);
}

void dismiss_countdown() {
    twr_scheduler_plan_absolute(countdown_task_id, TWR_TICK_INFINITY);

    transition_to_time_setting_state();

    twr_scheduler_plan_now(draw_task_id);
}

void increment_minutes(void) {
    app_ctx.minutes += 1;

    if (app_ctx.minutes > MAX_MINUTES) {
        app_ctx.minutes = 0;
    }

    check_and_draw_time();
}

void increment_seconds() {
    app_ctx.seconds += SECONDS_INCREMENT;

    if (app_ctx.seconds >= 60) {
        app_ctx.seconds = 0;
        app_ctx.minutes++;

        if (app_ctx.minutes > MAX_MINUTES) {
            app_ctx.minutes = 0;
        }
    }

    check_and_draw_time();
}

void reset_minutes() {
    app_ctx.minutes = 0;
    check_and_draw_time();
}

void reset_seconds() {
    app_ctx.seconds = 0;
    check_and_draw_time();
}

void set_time(uint8_t minutes, uint8_t seconds) {
    app_ctx.minutes = minutes;
    app_ctx.seconds = seconds;

    if (app_ctx.minutes >= MAX_MINUTES) {
        app_ctx.minutes = MAX_MINUTES;
        app_ctx.seconds = 0;
    }

    if (app_ctx.seconds >= 60) {
        app_ctx.seconds = 0;
    }

    check_and_draw_time();
}

void check_and_draw_time() {
    if (app_ctx.minutes == 0 && app_ctx.seconds == 0) {
        transition_to_sleep_state();
    }

    twr_scheduler_plan_now(draw_task_id);
}

void draw_time_with_bg(char *time_str, uint8_t progress, bool inverted) {
    if (!twr_module_lcd_is_ready()) {
    	return;
    }

    twr_system_pll_enable();

    twr_gfx_t *pgfx = twr_module_lcd_get_gfx();

    twr_gfx_clear(pgfx);

    int text_width = twr_gfx_calc_string_width(pgfx, time_str);

    int center_x = (128 - text_width) / 2;
    int center_y = (128 - TEXT_HEIGHT) / 2;

    bool fg_color = !inverted ? COLOR_BLACK : COLOR_WHITE;
    bool bg_color = !inverted ? COLOR_WHITE : COLOR_BLACK;

    if (fg_color == COLOR_BLACK) {
        twr_gfx_draw_fill_rectangle(pgfx, 0, 128 - progress, 127, 127, fg_color);
    }

    if (progress > 0) {
        twr_gfx_draw_string(pgfx, center_x - 1, center_y, time_str, bg_color);
        twr_gfx_draw_string(pgfx, center_x + 1, center_y, time_str, bg_color);
        twr_gfx_draw_string(pgfx, center_x, center_y - 1, time_str, bg_color);
        twr_gfx_draw_string(pgfx, center_x, center_y + 1, time_str, bg_color);
    }
    twr_gfx_draw_string(pgfx, center_x, center_y, time_str, fg_color);

    twr_gfx_update(pgfx);

    twr_system_pll_disable();
}

void draw_sleep_state() {
    if (!twr_module_lcd_is_ready()) {
    	return;
    }

    twr_system_pll_enable();

    twr_gfx_t *pgfx = twr_module_lcd_get_gfx();

    twr_gfx_clear(pgfx);
    twr_gfx_update(pgfx);

    twr_system_pll_disable();

    twr_module_lcd_off();
}

void draw_time_setting_state() {
    char time_str[8];

    snprintf(time_str, sizeof(time_str), "%02d:%02d", app_ctx.minutes, app_ctx.seconds);

    draw_time_with_bg(time_str, 0, false);

    twr_module_lcd_on();
}

void draw_countdown_state() {
    char time_str[8];

    snprintf(time_str, sizeof(time_str), "%02d:%02d", app_ctx.minutes, app_ctx.seconds);

    draw_time_with_bg(time_str, app_ctx.progress, false);

    twr_module_lcd_on();
}

void draw_alarm_state() {
    if (app_ctx.alarm_blink_tick % 2 == 0) {
        twr_led_set_mode(&led_lcd_blue, TWR_LED_MODE_OFF);
    } else {
        twr_led_set_mode(&led_lcd_blue, TWR_LED_MODE_ON);
    }

    draw_time_with_bg("00:00", 128, app_ctx.alarm_blink_tick % 2 == 1);

    twr_module_lcd_on();
}

void transition_to_time_setting_state() {
    app_ctx.progress = 0;
    app_ctx.state = STATE_TIME_SETTING;

    twr_lis2dh12_set_alarm(&accelerometer, &accelerometer_alarm);
    twr_lis2dh12_set_update_interval(&accelerometer, 100);
}

void transition_to_sleep_state() {
    app_ctx.minutes = 0;
    app_ctx.seconds = 0;
    app_ctx.state = STATE_SLEEP;

    twr_lis2dh12_set_alarm(&accelerometer, NULL);
    twr_lis2dh12_set_update_interval(&accelerometer, TWR_TICK_INFINITY);
}

void transition_to_countdown_state() {
    app_ctx.total_ms = (app_ctx.minutes * 60 + app_ctx.seconds) * 1000;
    app_ctx.progress = 0;
    app_ctx.state = STATE_COUNTDOWN;

    twr_rtc_get_timestamp(&app_ctx.start_time);

    twr_lis2dh12_set_alarm(&accelerometer, NULL);
    twr_lis2dh12_set_update_interval(&accelerometer, TWR_TICK_INFINITY);

    twr_scheduler_plan_now(countdown_task_id);
}

void transition_to_alarm_state() {
    app_ctx.total_ms = ALARM_DURATION_MS;
    app_ctx.alarm_blink_tick = 0;
    app_ctx.state = STATE_ALARM;

    twr_rtc_get_timestamp(&app_ctx.start_time);

    twr_scheduler_plan_now(alarm_task_id);
}

void lcd_event_handler(twr_module_lcd_event_t event, void *event_param) {
    (void) event_param;

    switch (app_ctx.state) {
        case STATE_SLEEP: {
            if (event == TWR_MODULE_LCD_EVENT_LEFT_CLICK) {
                transition_to_time_setting_state();
                increment_minutes();
            } else if (event == TWR_MODULE_LCD_EVENT_RIGHT_CLICK) {
                transition_to_time_setting_state();
                increment_seconds();
            } else if (event == TWR_MODULE_LCD_EVENT_LEFT_HOLD) {
                set_time(2, 30);
                transition_to_countdown_state();
            } else if (event == TWR_MODULE_LCD_EVENT_RIGHT_HOLD) {
                transition_to_time_setting_state();
                set_time(5, 0);
            }
            break;
        }
        case STATE_TIME_SETTING: {
            if (event == TWR_MODULE_LCD_EVENT_LEFT_CLICK) {
                increment_minutes();
            } else if (event == TWR_MODULE_LCD_EVENT_RIGHT_CLICK) {
                increment_seconds();
            } else if (event == TWR_MODULE_LCD_EVENT_LEFT_HOLD) {
                reset_minutes();
            } else if (event == TWR_MODULE_LCD_EVENT_RIGHT_HOLD) {
                reset_seconds();
            } else if (event == TWR_MODULE_LCD_EVENT_BOTH_HOLD) {
                transition_to_countdown_state();
            }
            break;
        }
        case STATE_COUNTDOWN: {
            if (event == TWR_MODULE_LCD_EVENT_BOTH_HOLD) {
                dismiss_countdown();
            }
            break;
        }
        case STATE_ALARM: {
            if (event == TWR_MODULE_LCD_EVENT_LEFT_CLICK
                || event == TWR_MODULE_LCD_EVENT_RIGHT_CLICK
                || event == TWR_MODULE_LCD_EVENT_LEFT_HOLD
                || event == TWR_MODULE_LCD_EVENT_RIGHT_HOLD
                || event == TWR_MODULE_LCD_EVENT_BOTH_HOLD
            ) {
                dismiss_alarm();
            }
            break;
        }
        default:
            break;
    }
}

void lis2dh12_event_handler(twr_lis2dh12_t *self, twr_lis2dh12_event_t event, void *event_param) {
    (void) self;
    (void) event_param;

    if (event == TWR_LIS2DH12_EVENT_ALARM && app_ctx.state == STATE_TIME_SETTING) {
        transition_to_countdown_state();
    }
}

void application_init(void) {
    twr_rtc_init();

    twr_module_lcd_init();
    twr_gfx_t *pgfx = twr_module_lcd_get_gfx();
    twr_gfx_set_font(pgfx, FONT);
    twr_module_lcd_set_event_handler(lcd_event_handler, NULL);
    twr_module_lcd_off();

    twr_lis2dh12_init(&accelerometer, TWR_I2C_I2C0, 0x19);
    twr_lis2dh12_set_event_handler(&accelerometer, lis2dh12_event_handler, NULL);

    twr_led_init_virtual(&led_lcd_blue, TWR_MODULE_LCD_LED_BLUE, twr_module_lcd_get_led_driver(), true);
    twr_led_set_mode(&led_lcd_blue, TWR_LED_MODE_OFF);

    draw_task_id = twr_scheduler_register(draw_task, NULL, TWR_TICK_INFINITY);
    countdown_task_id = twr_scheduler_register(countdown_task, NULL, TWR_TICK_INFINITY);
    alarm_task_id = twr_scheduler_register(alarm_task, NULL, TWR_TICK_INFINITY);
}
