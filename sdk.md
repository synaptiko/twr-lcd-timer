# LCD Module API Overview and Implementation Guide

In case you are not sure how something works or what things are available, you can consult following links:
- SDK: https://sdk.hardwario.com/index.html
- Complete device docs: https://docs.hardwario.com/tower/

## LCD Display Specifications
- **Resolution**: 128 x 128 pixels
- **Colors**: the monochrome display

## Integrated Components
- **2 Push Buttons**: Located below the LCD screen
- **6 RGB LEDs**: Miniature LEDs for status indication

## Core API Functions

### 1. LCD Module Initialization & Power Control

```c
// Initialize the LCD module (must be called first)
void twr_module_lcd_init(void);

// Power management functions
bool twr_module_lcd_on(void);   // Turn on display
bool twr_module_lcd_off(void);  // Turn off display (save power)

// Check if LCD is ready for commands
bool twr_module_lcd_is_ready(void);

// Clear the display
void twr_module_lcd_clear(void);

// Update display (send buffer to LCD)
bool twr_module_lcd_update(void);
```

**Important**: After calling `twr_module_lcd_off()`, you MUST call `twr_module_lcd_on()` before any drawing operations. The display won't turn on automatically.

### 2. Graphics Functions (twr_gfx)

```c
// Get the graphics context (required for GFX operations)
twr_gfx_t* twr_module_lcd_get_gfx(void);

// Text drawing functions
void twr_gfx_set_font(twr_gfx_t *self, const twr_font_t *font);
int twr_gfx_draw_char(twr_gfx_t *self, int left, int top, uint8_t ch, bool color);
int twr_gfx_draw_string(twr_gfx_t *self, int left, int top, char *str, bool color);
int twr_gfx_printf(twr_gfx_t *self, int left, int top, bool color, const char *format, ...);

// Calculate text dimensions
int twr_gfx_calc_char_width(twr_gfx_t *self, uint8_t ch);
int twr_gfx_calc_string_width(twr_gfx_t *self, char *str);

// Basic shapes
void twr_gfx_draw_pixel(twr_gfx_t *self, int x, int y, bool color);
void twr_gfx_draw_line(twr_gfx_t *self, int x0, int y0, int x1, int y1, bool color);
void twr_gfx_draw_rectangle(twr_gfx_t *self, int x0, int y0, int x1, int y1, bool color);
void twr_gfx_draw_fill_rectangle(twr_gfx_t *self, int x0, int y0, int x1, int y1, bool color);
void twr_gfx_draw_circle(twr_gfx_t *self, int x0, int y0, int radius, bool color);
void twr_gfx_draw_fill_circle(twr_gfx_t *self, int x0, int y0, int radius, bool color);

// Display rotation
void twr_gfx_set_rotation(twr_gfx_t *self, twr_gfx_rotation_t rotation);
twr_gfx_rotation_t twr_gfx_get_rotation(twr_gfx_t *self);

// Clear and update
void twr_gfx_clear(twr_gfx_t *self);
bool twr_gfx_update(twr_gfx_t *self);  // Send buffer to display
```

**Note**: Color parameter is `bool` - `true` for black pixels, `false` for white pixels.

### 2.1 Font System Details

#### Available Pre-built Fonts
```c
// Font declarations (from twr_font_common.h)
extern const twr_font_t twr_font_ubuntu_11;  // ~11 pixels height
extern const twr_font_t twr_font_ubuntu_13;  // ~13 pixels height
extern const twr_font_t twr_font_ubuntu_15;  // ~15 pixels height
extern const twr_font_t twr_font_ubuntu_24;  // ~24 pixels height
extern const twr_font_t twr_font_ubuntu_28;  // ~28 pixels height
extern const twr_font_t twr_font_ubuntu_33;  // ~33 pixels height
```

#### Font Structure
```c
// Font image structure - contains bitmap for each character
typedef struct {
    const uint8_t *image;  // Bitmap data
    uint8_t width;         // Character width in pixels
    uint8_t heigth;        // Character height in pixels (typo in SDK: "heigth")
} twr_font_image_t;

// Font character mapping
typedef struct {
    uint16_t code;                    // Unicode character code
    const twr_font_image_t *image;    // Pointer to character bitmap
} twr_font_char_t;

// Main font structure
typedef struct {
    uint16_t length;                   // Number of characters in font
    const twr_font_char_t *chars;      // Array of character definitions
} twr_font_t;
```

#### Important Font Notes
- **Font Size Naming**: The number in the font name (11, 13, 15, etc.) represents the intended font size/height in pixels
- **Actual Height**: The real bitmap height may be slightly larger than the nominal size to accommodate ascenders/descenders
- **Variable Width**: Each character can have different widths (proportional fonts)
- **Character Set**: Fonts include standard ASCII characters and may include extended characters

#### Font Usage Examples
```c
// Small font for status/debug info
twr_gfx_set_font(pgfx, &twr_font_ubuntu_11);

// Medium fonts for normal text
twr_gfx_set_font(pgfx, &twr_font_ubuntu_13);  // Default, good readability
twr_gfx_set_font(pgfx, &twr_font_ubuntu_15);  // Slightly larger

// Large fonts for headers/important values
twr_gfx_set_font(pgfx, &twr_font_ubuntu_24);  // Clear headers
twr_gfx_set_font(pgfx, &twr_font_ubuntu_28);  // Large values
twr_gfx_set_font(pgfx, &twr_font_ubuntu_33);  // Maximum size

// Calculate text dimensions before drawing
int text_width = twr_gfx_calc_string_width(pgfx, "Temperature");
// Center text: x = (128 - text_width) / 2
```

#### Display Space Considerations
With 128x128 pixel display:
- **twr_font_ubuntu_11**: ~11 lines of text, ~16-20 chars per line
- **twr_font_ubuntu_13**: ~9 lines of text, ~14-18 chars per line
- **twr_font_ubuntu_15**: ~8 lines of text, ~12-16 chars per line
- **twr_font_ubuntu_24**: ~5 lines of text, ~8-10 chars per line
- **twr_font_ubuntu_28**: ~4 lines of text, ~6-8 chars per line
- **twr_font_ubuntu_33**: ~3 lines of text, ~5-6 chars per line

*Note: Actual character count depends on specific characters (proportional width)*

### 3. LCD Module Button Handling

```c
// Button event types
typedef enum {
    TWR_MODULE_LCD_EVENT_LEFT_CLICK,
    TWR_MODULE_LCD_EVENT_LEFT_HOLD,
    TWR_MODULE_LCD_EVENT_LEFT_RELEASE,
    TWR_MODULE_LCD_EVENT_RIGHT_CLICK,
    TWR_MODULE_LCD_EVENT_RIGHT_HOLD,
    TWR_MODULE_LCD_EVENT_RIGHT_RELEASE,
    TWR_MODULE_LCD_EVENT_BOTH_CLICK,
    TWR_MODULE_LCD_EVENT_BOTH_HOLD
} twr_module_lcd_event_t;

// Button channels
typedef enum {
    TWR_MODULE_LCD_BUTTON_LEFT = 0,
    TWR_MODULE_LCD_BUTTON_RIGHT = 1
} twr_module_lcd_button_t;

// Set event handler for buttons
void twr_module_lcd_set_event_handler(void (*event_handler)(twr_module_lcd_event_t, void *), void *event_param);

// Button timing configuration (all times in ticks)
void twr_module_lcd_set_button_scan_interval(twr_tick_t scan_interval);
void twr_module_lcd_set_button_debounce_time(twr_tick_t debounce_time);
void twr_module_lcd_set_button_hold_time(twr_tick_t hold_time);
void twr_module_lcd_set_button_click_timeout(twr_tick_t click_timeout);
```

### 4. LCD Integrated LED Control

```c
// LED channels
typedef enum {
    TWR_MODULE_LCD_LED_RED = 0,
    TWR_MODULE_LCD_LED_GREEN = 1,
    TWR_MODULE_LCD_LED_BLUE = 2
} twr_module_lcd_led_t;

// Get LED driver
const twr_led_driver_t* twr_module_lcd_get_led_driver(void);

// Initialize virtual LED
void twr_led_init_virtual(twr_led_t *self, int channel, const twr_led_driver_t *driver, int idle_state);

// LED control functions
void twr_led_set_mode(twr_led_t *self, twr_led_mode_t mode);
void twr_led_pulse(twr_led_t *self, twr_tick_t duration);  // duration in milliseconds
void twr_led_blink(twr_led_t *self, int count);
```

**LED Modes**:
- `TWR_LED_MODE_OFF` - LED off
- `TWR_LED_MODE_ON` - LED on
- `TWR_LED_MODE_BLINK_SLOW` - Slow blinking
- `TWR_LED_MODE_BLINK_FAST` - Fast blinking

## Power Management Strategies

### 1. Event-Driven Programming Model
Use event handlers instead of polling to save power:

```c
// Example: Temperature sensor with event-driven updates
void temperature_event_handler(twr_tmp112_t *self, twr_tmp112_event_t event, void *param) {
    if (event == TWR_TMP112_EVENT_UPDATE) {
        float temperature;
        if (twr_tmp112_get_temperature_celsius(self, &temperature)) {
            // Update LCD display
            update_lcd_temperature(temperature);
        }
    }
}

// Set update interval (e.g., every 5 minutes)
twr_tmp112_set_update_interval(&temperature, 5 * 60 * 1000);
```

### 2. Task Scheduler for Periodic Updates
Use the scheduler for timed operations instead of busy-waiting:

```c
// Schedule a task to run after specific time
twr_scheduler_task_id_t task_id;

void update_display_task(void* param) {
    // Update display content
    refresh_display_data();

    // Reschedule for next update (e.g., 10 seconds)
    twr_scheduler_plan_current_from_now(10000);
}

// In application_init()
task_id = twr_scheduler_register(update_display_task, NULL, twr_tick_get() + 10000);
```

### 3. LCD Power Optimization Tips

1. **Turn off LCD when not needed**:
```c
// After showing information for a period
twr_scheduler_register(lcd_off_task, NULL, twr_tick_get() + 30000);  // Off after 30s
```

2. **Use update intervals wisely**:
- Set longer intervals for non-critical data
- Group multiple updates together before calling `twr_gfx_update()`

3. **Minimize display updates**:
- Only update changed portions when possible
- Batch drawing operations before calling update

## Key Implementation Notes

### Drawing Operations
1. Always get the GFX context first: `pgfx = twr_module_lcd_get_gfx();`
2. Clear display before drawing new content: `twr_gfx_clear(pgfx);`
3. Batch all drawing operations before calling `twr_gfx_update(pgfx);`
4. Coordinate system: (0,0) is top-left, (127,127) is bottom-right

### Power Efficiency Best Practices
1. **LCD Auto-off**: Implement timeout to turn off display after inactivity
2. **Event-driven**: Use interrupts and events instead of polling
3. **Batch Updates**: Group display updates to minimize refresh cycles
