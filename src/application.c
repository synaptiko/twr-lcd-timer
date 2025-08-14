#include <application.h>
#include <time.h>
#include <string.h>

// Application states
typedef enum {
    STATE_SLEEP,
    STATE_TIME_SETTING,
    STATE_COUNTDOWN_ACTIVE,
    STATE_ALARM
} app_state_t;

// Application context
static struct {
    app_state_t state;
    uint8_t minutes;
    uint8_t seconds;
    uint16_t total_seconds;
    uint16_t remaining_seconds;
    uint8_t alarm_blink_tick;  // Counter for alarm blinking
    uint8_t alarm_duration_tick;  // Counter for 10 second timeout
    struct timespec start_time;  // RTC timestamp when countdown started
} app_ctx = {
    .state = STATE_SLEEP,
    .minutes = 0,
    .seconds = 0,
    .total_seconds = 0,
    .remaining_seconds = 0,
    .alarm_blink_tick = 0,
    .alarm_duration_tick = 0
};

// LED instances
static twr_led_t led_lcd_red;
static twr_led_t led_lcd_blue;

// Task IDs
static twr_scheduler_task_id_t countdown_task_id;
static twr_scheduler_task_id_t alarm_task_id;

// Accelerometer instance
static twr_lis2dh12_t accelerometer;

// Forward declarations
void countdown_task(void *param);
void alarm_task(void *param);
static void dismiss_alarm(void);
void lis2dh12_event_handler(twr_lis2dh12_t *self, twr_lis2dh12_event_t event, void *event_param);

static void dismiss_alarm(void)
{
    // Clear display before turning off
    twr_gfx_t *pgfx = twr_module_lcd_get_gfx();
    twr_gfx_clear(pgfx);
    twr_gfx_update(pgfx);
    
    // Transition to sleep
    app_ctx.state = STATE_SLEEP;
    twr_led_set_mode(&led_lcd_red, TWR_LED_MODE_OFF);
    twr_led_set_mode(&led_lcd_blue, TWR_LED_MODE_OFF);
    twr_module_lcd_off();
    
    // Cancel the alarm task
    twr_scheduler_plan_absolute(alarm_task_id, TWR_TICK_INFINITY);
}

void lcd_event_handler(twr_module_lcd_event_t event, void *event_param)
{
    (void) event_param;
    
    // Handle alarm state for any button event
    if (app_ctx.state == STATE_ALARM)
    {
        switch (event)
        {
            case TWR_MODULE_LCD_EVENT_LEFT_CLICK:
            case TWR_MODULE_LCD_EVENT_RIGHT_CLICK:
            case TWR_MODULE_LCD_EVENT_LEFT_HOLD:
            case TWR_MODULE_LCD_EVENT_RIGHT_HOLD:
            case TWR_MODULE_LCD_EVENT_BOTH_HOLD:
                dismiss_alarm();
                return;
            
            // These events are not used but must be handled to avoid warnings
            case TWR_MODULE_LCD_EVENT_LEFT_PRESS:
            case TWR_MODULE_LCD_EVENT_LEFT_RELEASE:
            case TWR_MODULE_LCD_EVENT_RIGHT_PRESS:
            case TWR_MODULE_LCD_EVENT_RIGHT_RELEASE:
                // Intentionally ignored - we only use CLICK and HOLD events
                break;
                
            default:
                break;
        }
    }

    switch (event)
    {
        case TWR_MODULE_LCD_EVENT_LEFT_CLICK:
        {
            if (app_ctx.state == STATE_SLEEP)
            {
                // Wake up from sleep and set initial time to 01:00
                app_ctx.minutes = 1;
                app_ctx.seconds = 0;
                app_ctx.state = STATE_TIME_SETTING;
                twr_module_lcd_on();
                twr_scheduler_plan_now(0);
            }
            else if (app_ctx.state == STATE_TIME_SETTING)
            {
                // Increase minutes by 1 (wrap from 10 to 0)
                app_ctx.minutes++;
                if (app_ctx.minutes > 10)
                {
                    app_ctx.minutes = 0;
                }
                
                // Check if we wrapped to 00:00
                if (app_ctx.minutes == 0 && app_ctx.seconds == 0)
                {
                    // Clear display before going to sleep to avoid showing old value
                    twr_gfx_t *pgfx = twr_module_lcd_get_gfx();
                    twr_gfx_clear(pgfx);
                    twr_gfx_update(pgfx);
                    
                    app_ctx.state = STATE_SLEEP;
                    twr_module_lcd_off();
                }
                
                twr_scheduler_plan_now(0);
            }
            break;
        }
        
        case TWR_MODULE_LCD_EVENT_RIGHT_CLICK:
        {
            if (app_ctx.state == STATE_SLEEP)
            {
                // Wake up from sleep and set initial time to 00:10
                app_ctx.minutes = 0;
                app_ctx.seconds = 10;
                app_ctx.state = STATE_TIME_SETTING;
                twr_module_lcd_on();
                twr_scheduler_plan_now(0);
            }
            else if (app_ctx.state == STATE_TIME_SETTING)
            {
                // Special case: at 10:00, wrap to 00:00
                if (app_ctx.minutes == 10 && app_ctx.seconds == 0)
                {
                    app_ctx.minutes = 0;
                    app_ctx.seconds = 0;
                    
                    // Clear display and go to sleep
                    twr_gfx_t *pgfx = twr_module_lcd_get_gfx();
                    twr_gfx_clear(pgfx);
                    twr_gfx_update(pgfx);
                    
                    app_ctx.state = STATE_SLEEP;
                    twr_module_lcd_off();
                }
                else
                {
                    // Normal increment
                    app_ctx.seconds += 10;
                    
                    // Handle wrapping: if seconds reach 60, add 1 minute and reset seconds
                    if (app_ctx.seconds >= 60)
                    {
                        app_ctx.seconds = 0;
                        app_ctx.minutes++;
                        
                        // If minutes exceed 10, wrap to 0
                        if (app_ctx.minutes > 10)
                        {
                            app_ctx.minutes = 0;
                        }
                    }
                    // Normal wrap from 50 to 0 if we're not at the minute boundary
                    else if (app_ctx.seconds > 50)
                    {
                        app_ctx.seconds = 0;
                    }
                    
                    // Check if we wrapped to 00:00
                    if (app_ctx.minutes == 0 && app_ctx.seconds == 0)
                    {
                        // Clear display before going to sleep
                        twr_gfx_t *pgfx = twr_module_lcd_get_gfx();
                        twr_gfx_clear(pgfx);
                        twr_gfx_update(pgfx);
                        
                        app_ctx.state = STATE_SLEEP;
                        twr_module_lcd_off();
                    }
                }
                
                twr_scheduler_plan_now(0);
            }
            break;
        }
        
        case TWR_MODULE_LCD_EVENT_LEFT_HOLD:
        {
            if (app_ctx.state == STATE_ALARM)
            {
                // Dismiss alarm and go to sleep
                app_ctx.state = STATE_SLEEP;
                twr_led_set_mode(&led_lcd_red, TWR_LED_MODE_OFF);
                twr_led_set_mode(&led_lcd_blue, TWR_LED_MODE_OFF);
                twr_module_lcd_off();
                twr_scheduler_unregister(alarm_task_id);
            }
            else if (app_ctx.state == STATE_TIME_SETTING)
            {
                // Reset minutes to 0
                app_ctx.minutes = 0;
                
                // Check if we now have 00:00
                if (app_ctx.seconds == 0)
                {
                    // Clear display and go to sleep
                    twr_gfx_t *pgfx = twr_module_lcd_get_gfx();
                    twr_gfx_clear(pgfx);
                    twr_gfx_update(pgfx);
                    
                    app_ctx.state = STATE_SLEEP;
                    twr_module_lcd_off();
                }
                
                twr_scheduler_plan_now(0);
            }
            break;
        }
        
        case TWR_MODULE_LCD_EVENT_RIGHT_HOLD:
        {
            if (app_ctx.state == STATE_ALARM)
            {
                // Dismiss alarm and go to sleep
                app_ctx.state = STATE_SLEEP;
                twr_led_set_mode(&led_lcd_red, TWR_LED_MODE_OFF);
                twr_led_set_mode(&led_lcd_blue, TWR_LED_MODE_OFF);
                twr_module_lcd_off();
                twr_scheduler_unregister(alarm_task_id);
            }
            else if (app_ctx.state == STATE_TIME_SETTING)
            {
                // Reset seconds to 0
                app_ctx.seconds = 0;
                
                // Check if we now have 00:00
                if (app_ctx.minutes == 0)
                {
                    // Clear display and go to sleep
                    twr_gfx_t *pgfx = twr_module_lcd_get_gfx();
                    twr_gfx_clear(pgfx);
                    twr_gfx_update(pgfx);
                    
                    app_ctx.state = STATE_SLEEP;
                    twr_module_lcd_off();
                }
                
                twr_scheduler_plan_now(0);
            }
            break;
        }
        
        case TWR_MODULE_LCD_EVENT_BOTH_HOLD:
        {
            if (app_ctx.state == STATE_TIME_SETTING)
            {
                // Start timer only if time > 00:00
                if (app_ctx.minutes > 0 || app_ctx.seconds > 0)
                {
                    // Calculate total seconds for countdown
                    app_ctx.total_seconds = app_ctx.minutes * 60 + app_ctx.seconds;
                    app_ctx.remaining_seconds = app_ctx.total_seconds;
                    
                    // Get current RTC timestamp as start time
                    twr_rtc_get_timestamp(&app_ctx.start_time);
                    
                    // Transition to COUNTDOWN_ACTIVE state
                    app_ctx.state = STATE_COUNTDOWN_ACTIVE;
                    
                    // Start the countdown timer
                    twr_scheduler_plan_now(0);
                    twr_scheduler_plan_from_now(countdown_task_id, 200);
                }
            }
            break;
        }
        
        // These events are not used but must be handled to avoid warnings
        case TWR_MODULE_LCD_EVENT_LEFT_PRESS:
        case TWR_MODULE_LCD_EVENT_LEFT_RELEASE:
        case TWR_MODULE_LCD_EVENT_RIGHT_PRESS:
        case TWR_MODULE_LCD_EVENT_RIGHT_RELEASE:
            // Intentionally ignored - we only use CLICK and HOLD events
            break;
            
        default:
            break;
    }
}

void countdown_task(void *param)
{
    (void) param;
    
    if (app_ctx.state == STATE_COUNTDOWN_ACTIVE)
    {
        // Get current RTC timestamp
        struct timespec current_time;
        twr_rtc_get_timestamp(&current_time);
        
        // Calculate elapsed time in milliseconds
        int64_t elapsed_ms = ((int64_t)(current_time.tv_sec - app_ctx.start_time.tv_sec) * 1000) +
                           ((current_time.tv_nsec - app_ctx.start_time.tv_nsec) / 1000000);
        
        // Calculate remaining seconds
        int elapsed_seconds = (int)(elapsed_ms / 1000);
        int new_remaining = app_ctx.total_seconds - elapsed_seconds;
        if (new_remaining < 0) new_remaining = 0;
        
        app_ctx.remaining_seconds = new_remaining;
        
        // Always update display
        twr_scheduler_plan_now(0);
        
        if (app_ctx.remaining_seconds == 0)
        {
            // Small delay to ensure 00:00 is displayed
            static uint8_t zero_display_count = 0;
            zero_display_count++;
            
            if (zero_display_count < 3)
            {
                // Show 00:00 for a few updates
                twr_scheduler_plan_current_from_now(200);
            }
            else
            {
                // Reset counter for next time
                zero_display_count = 0;
                
                // Now transition to ALARM state
                app_ctx.state = STATE_ALARM;
                app_ctx.alarm_blink_tick = 0;
                app_ctx.alarm_duration_tick = 0;
                
                // Turn on LEDs
                twr_led_set_mode(&led_lcd_red, TWR_LED_MODE_ON);
                twr_led_set_mode(&led_lcd_blue, TWR_LED_MODE_ON);
                
                // Start alarm task for blinking
                twr_scheduler_plan_now(0);
                twr_scheduler_plan_from_now(alarm_task_id, 250);
            }
        }
        else
        {
            // Schedule next countdown tick
            twr_scheduler_plan_current_from_now(200);
        }
    }
}

void alarm_task(void *param)
{
    (void) param;
    
    if (app_ctx.state == STATE_ALARM)
    {
        // Toggle blink state
        app_ctx.alarm_blink_tick++;
        
        // Toggle LEDs every 250ms (2Hz = 4 changes per second)
        if (app_ctx.alarm_blink_tick % 2 == 0)
        {
            // Turn off
            twr_led_set_mode(&led_lcd_red, TWR_LED_MODE_OFF);
            twr_led_set_mode(&led_lcd_blue, TWR_LED_MODE_OFF);
        }
        else
        {
            // Turn on
            twr_led_set_mode(&led_lcd_red, TWR_LED_MODE_ON);
            twr_led_set_mode(&led_lcd_blue, TWR_LED_MODE_ON);
        }
        
        // Update display (will handle blinking)
        twr_scheduler_plan_now(0);
        
        // Increment duration counter (250ms per tick)
        app_ctx.alarm_duration_tick++;
        
        // Check for 10 second timeout (40 ticks * 250ms = 10 seconds)
        if (app_ctx.alarm_duration_tick >= 40)
        {
            // Auto-dismiss alarm after 10 seconds
            dismiss_alarm();
        }
        else
        {
            // Schedule next blink
            twr_scheduler_plan_current_from_now(250);
        }
    }
}

void lis2dh12_event_handler(twr_lis2dh12_t *self, twr_lis2dh12_event_t event, void *event_param)
{
    (void) self;
    (void) event_param;

    if (event == TWR_LIS2DH12_EVENT_ALARM)
    {
        // Handle shake detection - start timer if in TIME_SETTING state
        if (app_ctx.state == STATE_TIME_SETTING)
        {
            // Start timer only if time > 00:00
            if (app_ctx.minutes > 0 || app_ctx.seconds > 0)
            {
                // Calculate total seconds for countdown
                app_ctx.total_seconds = app_ctx.minutes * 60 + app_ctx.seconds;
                app_ctx.remaining_seconds = app_ctx.total_seconds;
                
                // Get current RTC timestamp as start time
                twr_rtc_get_timestamp(&app_ctx.start_time);
                
                // Transition to COUNTDOWN_ACTIVE state
                app_ctx.state = STATE_COUNTDOWN_ACTIVE;
                
                // Start the countdown timer
                twr_scheduler_plan_now(0);
                twr_scheduler_plan_from_now(countdown_task_id, 200);
            }
        }
    }
}

void application_init(void)
{
    // Initialize RTC
    twr_rtc_init();
    
    // Initialize accelerometer with threshold alarm for shake detection
    twr_lis2dh12_init(&accelerometer, TWR_I2C_I2C0, 0x19);
    twr_lis2dh12_set_event_handler(&accelerometer, lis2dh12_event_handler, NULL);
    
    // Configure shake detection alarm
    twr_lis2dh12_alarm_t alarm = {
        .threshold = 0.5f,  // 0.5g threshold for shake detection
        .duration = 1,      // Minimum duration
        .x_high = true,     // Detect high acceleration on X axis
        .y_high = true,     // Detect high acceleration on Y axis
        .z_high = true      // Detect high acceleration on Z axis
    };
    twr_lis2dh12_set_alarm(&accelerometer, &alarm);
    twr_lis2dh12_set_update_interval(&accelerometer, 100);  // Check every 100ms
    
    // Initialize LCD
    twr_module_lcd_init();

    // Initialize LCD LEDs
    twr_led_init_virtual(&led_lcd_red, TWR_MODULE_LCD_LED_RED, twr_module_lcd_get_led_driver(), true);
    twr_led_init_virtual(&led_lcd_blue, TWR_MODULE_LCD_LED_BLUE, twr_module_lcd_get_led_driver(), true);
    
    // Turn off LEDs
    twr_led_set_mode(&led_lcd_red, TWR_LED_MODE_OFF);
    twr_led_set_mode(&led_lcd_blue, TWR_LED_MODE_OFF);

    // Set LCD event handler
    twr_module_lcd_set_event_handler(lcd_event_handler, NULL);

    // Register tasks
    countdown_task_id = twr_scheduler_register(countdown_task, NULL, TWR_TICK_INFINITY);
    alarm_task_id = twr_scheduler_register(alarm_task, NULL, TWR_TICK_INFINITY);

    // Start in sleep mode (LCD off)
    twr_module_lcd_off();
}

void application_task(void)
{
    if (!twr_module_lcd_is_ready())
    {
        return;
    }

    twr_gfx_t *pgfx = twr_module_lcd_get_gfx();

    switch (app_ctx.state)
    {
        case STATE_SLEEP:
            // Display is off in sleep state
            break;

        case STATE_TIME_SETTING:
        {
            // Clear display
            twr_gfx_clear(pgfx);

            // Set large font for time display
            twr_gfx_set_font(pgfx, &twr_font_ubuntu_33);

            // Format time string
            char time_str[8];  // Enough for "MM:SS\0"
            snprintf(time_str, sizeof(time_str), "%02d:%02d", app_ctx.minutes, app_ctx.seconds);

            // Calculate text dimensions for centering
            int text_width = twr_gfx_calc_string_width(pgfx, time_str);
            int text_height = 33; // Approximate height for ubuntu_33 font

            // Center the text
            int x = (128 - text_width) / 2;
            int y = (128 - text_height) / 2;

            // Draw the time
            twr_gfx_draw_string(pgfx, x, y, time_str, true);

            // Update display
            twr_gfx_update(pgfx);
            break;
        }

        case STATE_COUNTDOWN_ACTIVE:
        {
            // Clear display
            twr_gfx_clear(pgfx);
            
            // Get current RTC timestamp for precise progress calculation
            struct timespec current_time;
            twr_rtc_get_timestamp(&current_time);
            
            // Calculate elapsed time in milliseconds
            int64_t elapsed_ms = ((int64_t)(current_time.tv_sec - app_ctx.start_time.tv_sec) * 1000) +
                               ((current_time.tv_nsec - app_ctx.start_time.tv_nsec) / 1000000);
            
            // Calculate progress with millisecond precision
            float remaining_ms = (float)(app_ctx.total_seconds * 1000) - (float)elapsed_ms;
            if (remaining_ms < 0.0f) remaining_ms = 0.0f;
            
            float progress = remaining_ms / (float)(app_ctx.total_seconds * 1000);
            
            // Clamp progress to 0.0-1.0 range
            if (progress > 1.0f) progress = 1.0f;
            if (progress < 0.0f) progress = 0.0f;
            
            // Draw black background from bottom (grows as timer counts down)
            // Initially no black, fills from bottom as time runs out
            int black_height = (int)((1.0f - progress) * 128.0f);
            if (black_height > 0)
            {
                twr_gfx_draw_fill_rectangle(pgfx, 0, 128 - black_height, 127, 127, true);
            }
            
            // Set large font for time display
            twr_gfx_set_font(pgfx, &twr_font_ubuntu_33);
            
            // Calculate time from remaining seconds
            uint8_t minutes = app_ctx.remaining_seconds / 60;
            uint8_t seconds = app_ctx.remaining_seconds % 60;
            
            // Format time string
            char time_str[8];  // Enough for "MM:SS\0"
            snprintf(time_str, sizeof(time_str), "%02d:%02d", minutes, seconds);
            
            // Calculate text dimensions
            int text_width = twr_gfx_calc_string_width(pgfx, time_str);
            int text_height = 33;  // Height for ubuntu_33 font
            
            // Center position
            int x = (128 - text_width) / 2;
            int y = (128 - text_height) / 2;
            
            // Draw white rectangle background for text (with margin)
            int margin = 3;
            twr_gfx_draw_fill_rectangle(pgfx, 
                x - margin, 
                y - margin, 
                x + text_width + margin - 1, 
                y + text_height + margin - 1, 
                false);  // White background
            
            // Draw the time in black
            twr_gfx_draw_string(pgfx, x, y, time_str, true);
            
            // Update display
            twr_gfx_update(pgfx);
            break;
        }

        case STATE_ALARM:
        {
            // Set large font for time display
            twr_gfx_set_font(pgfx, &twr_font_ubuntu_33);
            
            // Display "00:00"
            char time_str[] = "00:00";
            
            // Calculate text dimensions
            int text_width = twr_gfx_calc_string_width(pgfx, time_str);
            int text_height = 33;  // Height for ubuntu_33 font
            
            // Center position
            int x = (128 - text_width) / 2;
            int y = (128 - text_height) / 2;
            
            // Alternate between normal and inverted display
            if (app_ctx.alarm_blink_tick % 2 == 1)
            {
                // Normal display: black text on white background
                twr_gfx_clear(pgfx);
                twr_gfx_draw_string(pgfx, x, y, time_str, true);
            }
            else
            {
                // Inverted display: white text on black background
                // Fill entire screen with black first
                twr_gfx_draw_fill_rectangle(pgfx, 0, 0, 127, 127, true);
                // Draw white text
                twr_gfx_draw_string(pgfx, x, y, time_str, false);
            }
            
            // Update display
            twr_gfx_update(pgfx);
            break;
        }

        default:
            break;
    }
}