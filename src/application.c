#include <application.h>

#define BATTERY_UPDATE_INTERVAL (60 * 60 * 1000)

#define COLOR_BLACK true
#define COLOR_WHITE false

typedef struct {
    bool bucket_full;
    char updated_date[16];    // "DD.MM. HH:MM"
    char emptied_date[16];    // "DD.MM."
    char estimate_date[16];   // "DD.MM."
    char sensor_battery[8];   // "2.6V" or "LOW" or "CRI"
    char display_battery[8];  // "2.6V" or "LOW" or "CRI"
    bool has_data;            // true when we have received first update
} bucket_state_t;

float battery_voltage = 0.0f;
bool battery_low = false;
bool battery_critical = false;
bucket_state_t bucket_state = {0};

twr_led_t led;
twr_led_t led_lcd_red;
twr_gfx_t *pgfx;

void battery_event_handler(twr_module_battery_event_t event, void *event_param)
{
    if (event == TWR_MODULE_BATTERY_EVENT_UPDATE)
    {
        twr_module_battery_get_voltage(&battery_voltage);
        
        // Update display battery status
        if (battery_critical)
        {
            strncpy(bucket_state.display_battery, "CRI", sizeof(bucket_state.display_battery));
        }
        else if (battery_low)
        {
            strncpy(bucket_state.display_battery, "LOW", sizeof(bucket_state.display_battery));
        }
        else
        {
            snprintf(bucket_state.display_battery, sizeof(bucket_state.display_battery), "%.1fV", battery_voltage);
        }
    }
    else if (event == TWR_MODULE_BATTERY_EVENT_LEVEL_LOW)
    {
        battery_low = true;
        strncpy(bucket_state.display_battery, "LOW", sizeof(bucket_state.display_battery));
    }
    else if (event == TWR_MODULE_BATTERY_EVENT_LEVEL_CRITICAL)
    {
        battery_critical = true;
        strncpy(bucket_state.display_battery, "CRI", sizeof(bucket_state.display_battery));
    }
}

void update_display(void)
{
    if (!twr_module_lcd_is_ready())
    {
        return;
    }

    twr_system_pll_enable();

    // Clear display
    twr_gfx_clear(pgfx);

    // Set colors based on bucket state
    bool text_color = bucket_state.bucket_full ? COLOR_WHITE : COLOR_BLACK;
    
    // Fill background if bucket is full (white text on black background)
    if (bucket_state.bucket_full)
    {
        twr_gfx_draw_fill_rectangle(pgfx, 0, 0, 127, 127, COLOR_BLACK);
    }

    // Display labels and values
    const char* updated_val = bucket_state.has_data ? bucket_state.updated_date : "--";
    const char* emptied_val = bucket_state.has_data ? bucket_state.emptied_date : "--";
    const char* estimate_val = bucket_state.has_data ? bucket_state.estimate_date : "--";
    const char* sensor_val = bucket_state.has_data ? bucket_state.sensor_battery : "--";
    const char* display_val = bucket_state.has_data ? bucket_state.display_battery : "--";

    int y_pos = 5;  // Starting position

    // Line 1: Updated label (bigger font)
    twr_gfx_set_font(pgfx, &twr_font_ubuntu_24);
    twr_gfx_draw_string(pgfx, 5, y_pos, "Updated", text_color);
    y_pos += 26;  // Space for bigger font

    // Line 2: Updated value (medium font, right-aligned)
    twr_gfx_set_font(pgfx, &twr_font_ubuntu_15);
    int updated_width = twr_gfx_calc_string_width(pgfx, (char*)updated_val);
    twr_gfx_draw_string(pgfx, 128 - updated_width - 5, y_pos, (char*)updated_val, text_color);
    y_pos += 25;  // Extra space after updated section

    // Switch to smaller font for remaining items
    twr_gfx_set_font(pgfx, &twr_font_ubuntu_13);
    int small_line_height = 17;  // Slightly more spacing for better readability

    // Line 3: Emptied
    twr_gfx_draw_string(pgfx, 5, y_pos, "Emptied", text_color);
    int emptied_width = twr_gfx_calc_string_width(pgfx, (char*)emptied_val);
    twr_gfx_draw_string(pgfx, 128 - emptied_width - 5, y_pos, (char*)emptied_val, text_color);
    y_pos += small_line_height;

    // Line 4: Estimate
    twr_gfx_draw_string(pgfx, 5, y_pos, "Estimate", text_color);
    int estimate_width = twr_gfx_calc_string_width(pgfx, (char*)estimate_val);
    twr_gfx_draw_string(pgfx, 128 - estimate_width - 5, y_pos, (char*)estimate_val, text_color);
    y_pos += small_line_height;

    // Line 5: Sensor
    twr_gfx_draw_string(pgfx, 5, y_pos, "Sensor", text_color);
    int sensor_width = twr_gfx_calc_string_width(pgfx, (char*)sensor_val);
    twr_gfx_draw_string(pgfx, 128 - sensor_width - 5, y_pos, (char*)sensor_val, text_color);
    y_pos += small_line_height;

    // Line 6: Display
    twr_gfx_draw_string(pgfx, 5, y_pos, "Display", text_color);
    int display_width = twr_gfx_calc_string_width(pgfx, (char*)display_val);
    twr_gfx_draw_string(pgfx, 128 - display_width - 5, y_pos, (char*)display_val, text_color);

    // Update the display
    twr_gfx_update(pgfx);

    twr_system_pll_disable();
}

void lcd_button_event_handler(twr_module_lcd_event_t event, void *event_param)
{
    (void) event_param;

    if (event == TWR_MODULE_LCD_EVENT_LEFT_CLICK)
    {
        // Toggle bucket state for testing
        bucket_state.bucket_full = !bucket_state.bucket_full;
        
        // Update display immediately
        update_display();
        
        // Pulse LED to indicate button press
        twr_led_pulse(&led_lcd_red, 100);
    }
}

void application_init(void)
{
    twr_led_init(&led, TWR_GPIO_LED, false, false);
    twr_led_set_mode(&led, TWR_LED_MODE_OFF);

    twr_radio_init(TWR_RADIO_MODE_NODE_SLEEPING);

    twr_module_battery_init();
    twr_module_battery_set_event_handler(battery_event_handler, NULL);
    twr_module_battery_set_update_interval(BATTERY_UPDATE_INTERVAL);

    twr_module_lcd_init();
    pgfx = twr_module_lcd_get_gfx();
    
    // Set up LCD button event handler
    twr_module_lcd_set_event_handler(lcd_button_event_handler, NULL);

    twr_led_init_virtual(&led_lcd_red, TWR_MODULE_LCD_LED_RED, twr_module_lcd_get_led_driver(), true);

    // Initialize bucket state with empty bucket and dummy data for layout testing
    bucket_state.bucket_full = false;
    bucket_state.has_data = true;  // Show dummy values instead of "--"
    strncpy(bucket_state.sensor_battery, "2.6V", sizeof(bucket_state.sensor_battery));
    strncpy(bucket_state.display_battery, "3.1V", sizeof(bucket_state.display_battery));
    strncpy(bucket_state.updated_date, "12.10. 13:00", sizeof(bucket_state.updated_date));  // Longest possible
    strncpy(bucket_state.emptied_date, "5.9.", sizeof(bucket_state.emptied_date));
    strncpy(bucket_state.estimate_date, "15.10.", sizeof(bucket_state.estimate_date));

    twr_radio_pairing_request("bucket-monitor", FW_VERSION);

    // Initial display update
    update_display();

    twr_led_pulse(&led, 2000);
}

void application_task(void)
{
    // Update display periodically - called by scheduler when needed
    update_display();
}
