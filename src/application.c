#include <application.h>

#define BATTERY_UPDATE_INTERVAL (60 * 60 * 1000)

#define COLOR_BLACK true

float battery_voltage = 0.0f;
bool battery_low = false;
bool battery_critical = false;

twr_led_t led;
twr_led_t led_lcd_red;

void battery_event_handler(twr_module_battery_event_t event, void *event_param)
{
    if (event == TWR_MODULE_BATTERY_EVENT_UPDATE)
    {
        twr_module_battery_get_voltage(&battery_voltage);
    }
    else if (event == TWR_MODULE_BATTERY_EVENT_LEVEL_LOW)
    {
        battery_low = true;
    }
    else if (event == TWR_MODULE_BATTERY_EVENT_LEVEL_CRITICAL)
    {
        battery_critical = true;
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

    twr_led_init_virtual(&led_lcd_red, TWR_MODULE_LCD_LED_RED, twr_module_lcd_get_led_driver(), true);

    twr_radio_pairing_request("lcd-display", FW_VERSION);

    twr_led_pulse(&led, 2000);
}

void application_task(void)
{
    static char str_battery[10];

    if (!twr_module_lcd_is_ready())
    {
    	return;
    }

    twr_system_pll_enable();

    twr_module_lcd_clear();

    twr_module_lcd_set_font(&twr_font_ubuntu_24);
    twr_module_lcd_draw_string(20, 25, "\xb0" "C   ", COLOR_BLACK);

    twr_module_lcd_set_font(&twr_font_ubuntu_15);
    twr_module_lcd_draw_string(10, 80, "Battery", COLOR_BLACK);

    snprintf(str_battery, sizeof(str_battery), "%.1f", battery_voltage);
    twr_module_lcd_draw_string(40, 100, str_battery, COLOR_BLACK);

    twr_module_lcd_update();

    twr_system_pll_disable();
}
