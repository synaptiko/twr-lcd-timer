#include <application.h>
// #include <string.h>

// #define BATTERY_UPDATE_INTERVAL (60 * 60 * 1000)

// #define COLOR_BLACK true
// #define COLOR_WHITE false

// typedef struct {
//     bool bucket_full;
//     char updated_date[16];    // "DD.MM. HH:MM"
//     char emptied_date[16];    // "DD.MM."
//     char estimate_date[16];   // "DD.MM."
//     char sensor_battery[8];   // "2.6V" or "LOW" or "CRI"
//     char display_battery[8];  // "2.6V" or "LOW" or "CRI"
//     bool has_data;            // true when we have received first update
// } bucket_state_t;

// float battery_voltage = 0.0f;
// bool battery_low = false;
// bool battery_critical = false;
// bucket_state_t bucket_state = {0};

// twr_led_t led_lcd_red;
twr_led_t led_lcd_green;
// twr_led_t led_lcd_blue;
// twr_gfx_t *pgfx;

// // Alert system state
// typedef struct {
//     bool alert_active;
//     bool alert_acknowledged;
//     twr_tick_t last_update_time;
//     twr_tick_t alert_blink_start;
//     uint8_t blink_count;
// } alert_state_t;

// alert_state_t alert = {0};

void process_bucket_response(uint64_t *id, const char *topic, void *value, void *param);

static const twr_radio_sub_t subs[] = {
    {"bucket/response", TWR_RADIO_SUB_PT_STRING, process_bucket_response, (void *) 1},
};

// void check_alert_conditions(void)
// {
//     bool should_alert = false;

//     // Check for alert conditions
//     if (bucket_state.bucket_full)
//     {
//         should_alert = true; // Full bucket
//     }
//     else if (battery_critical ||
//              (bucket_state.has_data &&
//               (strcmp(bucket_state.sensor_battery, "CRI") == 0 ||
//                strcmp(bucket_state.display_battery, "CRI") == 0)))
//     {
//         should_alert = true; // Critical battery
//     }
//     else if (bucket_state.has_data &&
//              (twr_tick_get() - alert.last_update_time) > (3 * 60 * 60 * 1000))
//     {
//         should_alert = true; // No update for 3+ hours
//     }

//     // Start new alert if conditions are met and not already acknowledged
//     if (should_alert && !alert.alert_acknowledged)
//     {
//         if (!alert.alert_active)
//         {
//             alert.alert_active = true;
//             alert.alert_blink_start = twr_tick_get();
//             alert.blink_count = 0;
//         }
//     }
//     else if (!should_alert)
//     {
//         // Clear alert if conditions are resolved
//         alert.alert_active = false;
//         alert.alert_acknowledged = false;
//     }
// }

// void update_alert_leds(void)
// {
//     if (!alert.alert_active) return;

//     twr_tick_t now = twr_tick_get();
//     twr_tick_t elapsed = now - alert.alert_blink_start;

//     // Blink 20 times at 2-second intervals, every 15 minutes
//     if (elapsed < (20 * 2000)) // First 40 seconds of 15-minute cycle
//     {
//         bool led_on = (elapsed % 2000) < 100; // 100ms pulses

//         if (led_on)
//         {
//             twr_led_set_mode(&led_lcd_red, TWR_LED_MODE_ON);
//             twr_led_set_mode(&led_lcd_green, TWR_LED_MODE_ON);
//             twr_led_set_mode(&led_lcd_blue, TWR_LED_MODE_ON);
//         }
//         else
//         {
//             twr_led_set_mode(&led_lcd_red, TWR_LED_MODE_OFF);
//             twr_led_set_mode(&led_lcd_green, TWR_LED_MODE_OFF);
//             twr_led_set_mode(&led_lcd_blue, TWR_LED_MODE_OFF);
//         }
//     }
//     else if (elapsed >= (15 * 60 * 1000)) // Reset every 15 minutes
//     {
//         alert.alert_blink_start = now;
//     }
//     else
//     {
//         // Between blink cycles - LEDs off
//         twr_led_set_mode(&led_lcd_red, TWR_LED_MODE_OFF);
//         twr_led_set_mode(&led_lcd_green, TWR_LED_MODE_OFF);
//         twr_led_set_mode(&led_lcd_blue, TWR_LED_MODE_OFF);
//     }
// }

// void battery_event_handler(twr_module_battery_event_t event, void *event_param)
// {
//     if (event == TWR_MODULE_BATTERY_EVENT_UPDATE)
//     {
//         twr_module_battery_get_voltage(&battery_voltage);

//         // Update display battery status
//         if (battery_critical)
//         {
//             strncpy(bucket_state.display_battery, "CRI", sizeof(bucket_state.display_battery));
//         }
//         else if (battery_low)
//         {
//             strncpy(bucket_state.display_battery, "LOW", sizeof(bucket_state.display_battery));
//         }
//         else
//         {
//             snprintf(bucket_state.display_battery, sizeof(bucket_state.display_battery), "%.1fV", battery_voltage);
//         }
//     }
//     else if (event == TWR_MODULE_BATTERY_EVENT_LEVEL_LOW)
//     {
//         battery_low = true;
//         strncpy(bucket_state.display_battery, "LOW", sizeof(bucket_state.display_battery));
//     }
//     else if (event == TWR_MODULE_BATTERY_EVENT_LEVEL_CRITICAL)
//     {
//         battery_critical = true;
//         strncpy(bucket_state.display_battery, "CRI", sizeof(bucket_state.display_battery));
//     }
// }

// void update_display(void)
// {
//     if (!twr_module_lcd_is_ready())
//     {
//         return;
//     }

//     twr_system_pll_enable();

//     // Clear display
//     twr_gfx_clear(pgfx);

//     // Set colors based on bucket state
//     bool text_color = bucket_state.bucket_full ? COLOR_WHITE : COLOR_BLACK;

//     // Fill background if bucket is full (white text on black background)
//     if (bucket_state.bucket_full)
//     {
//         twr_gfx_draw_fill_rectangle(pgfx, 0, 0, 127, 127, COLOR_BLACK);
//     }

//     // Display labels and values
//     const char* updated_val = bucket_state.has_data ? bucket_state.updated_date : "--";
//     const char* emptied_val = bucket_state.has_data ? bucket_state.emptied_date : "--";
//     const char* estimate_val = bucket_state.has_data ? bucket_state.estimate_date : "--";
//     const char* sensor_val = bucket_state.has_data ? bucket_state.sensor_battery : "--";
//     const char* display_val = bucket_state.has_data ? bucket_state.display_battery : "--";

//     int y_pos = 5;  // Starting position

//     // Line 1: Updated label (bigger font)
//     twr_gfx_set_font(pgfx, &twr_font_ubuntu_24);
//     twr_gfx_draw_string(pgfx, 5, y_pos, "Updated", text_color);
//     y_pos += 26;  // Space for bigger font

//     // Line 2: Updated value (medium font, right-aligned)
//     twr_gfx_set_font(pgfx, &twr_font_ubuntu_15);
//     int updated_width = twr_gfx_calc_string_width(pgfx, (char*)updated_val);
//     twr_gfx_draw_string(pgfx, 128 - updated_width - 5, y_pos, (char*)updated_val, text_color);
//     y_pos += 25;  // Extra space after updated section

//     // Switch to smaller font for remaining items
//     twr_gfx_set_font(pgfx, &twr_font_ubuntu_13);
//     int small_line_height = 17;  // Slightly more spacing for better readability

//     // Line 3: Emptied
//     twr_gfx_draw_string(pgfx, 5, y_pos, "Emptied", text_color);
//     int emptied_width = twr_gfx_calc_string_width(pgfx, (char*)emptied_val);
//     twr_gfx_draw_string(pgfx, 128 - emptied_width - 5, y_pos, (char*)emptied_val, text_color);
//     y_pos += small_line_height;

//     // Line 4: Estimate
//     twr_gfx_draw_string(pgfx, 5, y_pos, "Estimate", text_color);
//     int estimate_width = twr_gfx_calc_string_width(pgfx, (char*)estimate_val);
//     twr_gfx_draw_string(pgfx, 128 - estimate_width - 5, y_pos, (char*)estimate_val, text_color);
//     y_pos += small_line_height;

//     // Line 5: Sensor
//     twr_gfx_draw_string(pgfx, 5, y_pos, "Sensor", text_color);
//     int sensor_width = twr_gfx_calc_string_width(pgfx, (char*)sensor_val);
//     twr_gfx_draw_string(pgfx, 128 - sensor_width - 5, y_pos, (char*)sensor_val, text_color);
//     y_pos += small_line_height;

//     // Line 6: Display
//     twr_gfx_draw_string(pgfx, 5, y_pos, "Display", text_color);
//     int display_width = twr_gfx_calc_string_width(pgfx, (char*)display_val);
//     twr_gfx_draw_string(pgfx, 128 - display_width - 5, y_pos, (char*)display_val, text_color);

//     // Update the display
//     twr_gfx_update(pgfx);

//     twr_system_pll_disable();
// }

// void parse_bucket_status_response(const char *csv_data)
// {
//     if (csv_data == NULL) return;

//     // Make a copy since strtok modifies the string
//     char buffer[128];
//     strncpy(buffer, csv_data, sizeof(buffer) - 1);
//     buffer[sizeof(buffer) - 1] = '\0';

//     char *token = strtok(buffer, ",");
//     int field = 0;

//     while (token != NULL && field < 6)
//     {
//         switch (field)
//         {
//             case 0: // Bucket state: "empty" or "full"
//                 bucket_state.bucket_full = (strcmp(token, "full") == 0);
//                 break;

//             case 1: // Current date/time: "DD.MM. HH:MM"
//                 strncpy(bucket_state.updated_date, token, sizeof(bucket_state.updated_date) - 1);
//                 bucket_state.updated_date[sizeof(bucket_state.updated_date) - 1] = '\0';
//                 break;

//             case 2: // Sensor battery: "2.6V", "LOW", or "CRI"
//                 strncpy(bucket_state.sensor_battery, token, sizeof(bucket_state.sensor_battery) - 1);
//                 bucket_state.sensor_battery[sizeof(bucket_state.sensor_battery) - 1] = '\0';
//                 break;

//             case 3: // Display battery: "2.6V", "LOW", or "CRI"
//                 strncpy(bucket_state.display_battery, token, sizeof(bucket_state.display_battery) - 1);
//                 bucket_state.display_battery[sizeof(bucket_state.display_battery) - 1] = '\0';
//                 break;

//             case 4: // Last emptied date: "DD.MM."
//                 strncpy(bucket_state.emptied_date, token, sizeof(bucket_state.emptied_date) - 1);
//                 bucket_state.emptied_date[sizeof(bucket_state.emptied_date) - 1] = '\0';
//                 break;

//             case 5: // Estimated full date: "DD.MM."
//                 strncpy(bucket_state.estimate_date, token, sizeof(bucket_state.estimate_date) - 1);
//                 bucket_state.estimate_date[sizeof(bucket_state.estimate_date) - 1] = '\0';
//                 break;

//             default:
//                 // Ignore extra fields
//                 break;
//         }

//         token = strtok(NULL, ",");
//         field++;
//     }

//     // Mark that we have received valid data and update timestamp
//     bucket_state.has_data = true;
//     alert.last_update_time = twr_tick_get();
// }

void process_bucket_response(uint64_t *id, const char *topic, void *value, void *param)
{
    // (void) id; // Unused - we accept from any gateway

    // TODO: remove, temporary for testing
    twr_led_pulse(&led_lcd_green, 1000);

    // if (strcmp(subtopic, "bucket/response") == 0 && value != NULL)
    // {
    //     // Parse the CSV response and update bucket state
    //     parse_bucket_status_response(value);

    //     // Update display with new data
    //     update_display();

    //     // Pulse blue LED to indicate successful update
    //     twr_led_pulse(&led_lcd_blue, 50);
    // }
}

// void request_bucket_status(void)
// {
//     // Send bucket status request message
//     if (twr_radio_pub_string("bucket/request", "status"))
//     {
//         // Pulse blue LED to indicate request sent successfully
//         twr_led_pulse(&led_lcd_blue, 100);
//     }
//     else
//     {
//         // Pulse red LED to indicate transmission failed
//         twr_led_pulse(&led_lcd_red, 200);
//     }
// }

// void lcd_button_event_handler(twr_module_lcd_event_t event, void *event_param)
// {
//     (void) event_param;

//     if (event == TWR_MODULE_LCD_EVENT_LEFT_CLICK)
//     {
//         // Button 1: Request immediate status update
//         request_bucket_status();
//     }
//     else if (event == TWR_MODULE_LCD_EVENT_LEFT_HOLD) {
//         twr_radio_pairing_request("bucket-monitor", FW_VERSION);

//         twr_led_pulse(&led_lcd_green, 2000);
//     }
//     else if (event == TWR_MODULE_LCD_EVENT_RIGHT_CLICK)
//     {
//         // Button 2: Acknowledge alert (stops LED blinking)
//         if (alert.alert_active)
//         {
//             alert.alert_acknowledged = true;
//             alert.alert_active = false;

//             // Turn off all LEDs
//             twr_led_set_mode(&led_lcd_red, TWR_LED_MODE_OFF);
//             twr_led_set_mode(&led_lcd_green, TWR_LED_MODE_OFF);
//             twr_led_set_mode(&led_lcd_blue, TWR_LED_MODE_OFF);

//             // Brief green pulse to confirm acknowledgment
//             twr_led_pulse(&led_lcd_green, 200);
//         }
//         else
//         {
//             // Toggle bucket state for testing when no alert active
//             bucket_state.bucket_full = !bucket_state.bucket_full;
//             update_display();
//             twr_led_pulse(&led_lcd_red, 100);
//         }
//     }
// }

void application_init(void)
{
    // Disable sleep in case of receiving data
    twr_system_deep_sleep_disable();

    twr_radio_init(TWR_RADIO_MODE_NODE_LISTENING);
    // twr_radio_init(TWR_RADIO_MODE_NODE_SLEEPING);
    // TODO: change to 400ms later on
    twr_radio_set_subs((twr_radio_sub_t *) subs, sizeof(subs)/sizeof(twr_radio_sub_t));
    // twr_radio_set_rx_timeout_for_sleeping_node(2000);

    twr_radio_pairing_request("bucket-monitor", FW_VERSION);

    // twr_module_battery_init();
    // twr_module_battery_set_event_handler(battery_event_handler, NULL);
    // twr_module_battery_set_update_interval(BATTERY_UPDATE_INTERVAL);

    // twr_module_lcd_init();
    // pgfx = twr_module_lcd_get_gfx();

    // // Set up LCD button event handler
    // twr_module_lcd_set_event_handler(lcd_button_event_handler, NULL);

    // twr_led_init_virtual(&led_lcd_red, TWR_MODULE_LCD_LED_RED, twr_module_lcd_get_led_driver(), true);
    twr_led_init_virtual(&led_lcd_green, TWR_MODULE_LCD_LED_GREEN, twr_module_lcd_get_led_driver(), true);
    // twr_led_init_virtual(&led_lcd_blue, TWR_MODULE_LCD_LED_BLUE, twr_module_lcd_get_led_driver(), true);

    // // Initialize bucket state with empty bucket and dummy data for layout testing
    // bucket_state.bucket_full = false;
    // bucket_state.has_data = true;  // Show dummy values instead of "--"
    // strncpy(bucket_state.sensor_battery, "2.6V", sizeof(bucket_state.sensor_battery));
    // strncpy(bucket_state.display_battery, "3.1V", sizeof(bucket_state.display_battery));
    // strncpy(bucket_state.updated_date, "12.10. 13:00", sizeof(bucket_state.updated_date));  // Longest possible
    // strncpy(bucket_state.emptied_date, "5.9.", sizeof(bucket_state.emptied_date));
    // strncpy(bucket_state.estimate_date, "15.10.", sizeof(bucket_state.estimate_date));

    // // Initialize alert system
    // alert.last_update_time = twr_tick_get();

    // // Initial display update
    // update_display();

    twr_led_pulse(&led_lcd_green, 2000);
    // // Example message to send:
    // // node/bucket-monitor:0/bucket/response "full,13.10. 13:00,2.6V,3.1V,5.9.,15.10."
}

// void application_task(void)
// {
//     // Check alert conditions and update LEDs
//     check_alert_conditions();
//     update_alert_leds();

//     // Update display periodically - called by scheduler when needed
//     update_display();
// }
