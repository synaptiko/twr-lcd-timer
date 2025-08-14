# Embedded Timer Application Specification

## 1. Overview

A simple countdown timer application for an embedded low-powered device with minimal UI capabilities, designed for kitchen timers or teeth brushing scenarios.

## 2. Hardware Components

### 2.1 Display
- **Type**: Black and white LCD module
- **Resolution**: 128x128 pixels
- **Capabilities**: Text rendering (various sizes), basic shapes (pixels, lines, rectangles, filled rectangles, circles, filled circles, rounded corners)
- **Power State**: Can be turned off to save power

### 2.2 Input Controls
- **Left Button**: Below display
- **Right Button**: Below display
- **Events**: Click and Hold for each button
- **Accelerometer**: Shake detection via threshold alarm

### 2.3 Visual Indicators
- **LED Strip**: 6 RGB LEDs above display (controlled as unified strip)
- **Functions**: Solid light, blinking patterns

## 3. Application States

### 3.1 State Machine Definition

```
States:
1. SLEEP (default)
2. TIME_SETTING
3. COUNTDOWN_ACTIVE
4. ALARM
```

### 3.2 State Descriptions

#### SLEEP State
- **Display**: OFF (blank screen)
- **LEDs**: OFF
- **Entry Condition**: Initial state, or after alarm dismissal, or when time is reset to 00:00
- **Exit Condition**: Any button click
- **Power Consumption**: Minimal

#### TIME_SETTING State
- **Display**: Shows MM:SS format
- **LEDs**: OFF
- **Entry Condition**: From SLEEP via button click
- **Exit Conditions**:
  - To COUNTDOWN_ACTIVE: Hold both buttons OR shake device (when time > 00:00)
  - To SLEEP: When time returns to 00:00
- **Display Update**: Only on time change

#### COUNTDOWN_ACTIVE State
- **Display**: Shows decreasing time with progress visualization
- **LEDs**: OFF
- **Entry Condition**: From TIME_SETTING via dual button hold or shake
- **Exit Condition**: To ALARM when timer reaches 00:00
- **Display Update**: Every second

#### ALARM State
- **Display**: Blinking "00:00"
- **LEDs**: Fast blinking
- **Entry Condition**: From COUNTDOWN_ACTIVE when timer reaches 00:00
- **Exit Condition**: To SLEEP via any button press/hold OR after 10 seconds timeout
- **Duration**: Maximum 10 seconds

## 4. User Interface Design

### 4.1 Time Display Format
- **Format**: MM:SS (minutes:seconds)
- **Font**: Large size, centered (note: SDK only has proportional font available; we need to consider this while showing the margin around time)
- **Color**: Black text on white background

### 4.2 Progress Visualization (During Countdown)
```
Layer Structure (bottom to top):
1. Black background rectangle (full screen initially)
2. White rectangle (centered, with margin)
3. Black text showing MM:SS

Progress Animation:
- Black background height = (remaining_time / total_time) * 128
- Decreases from top as timer counts down
```

### 4.3 Display Layout Calculations
- Text width: Use SDK measurement function
- Text position: Center horizontally and vertically
- White rectangle: Text bounds + margin (e.g., 8 pixels padding)
- Update frequency: Once per second during countdown

## 5. Control Behavior

### 5.1 TIME_SETTING State Controls

| Action | Effect |
|--------|--------|
| Left Button Click | Increase minutes by 1 (max 10, wrap to 0) |
| Right Button Click | Increase seconds by 10 (at 60: add 1 minute, reset seconds to 0) |
| Left Button Hold | Reset minutes to 0 |
| Right Button Hold | Reset seconds to 0 |
| Both Buttons Hold | Start timer (if time > 00:00) |
| Device Shake | Start timer (if time > 00:00) |

### 5.2 Time Setting Constraints
- **Minutes Range**: 0-10 (increment: 1)
- **Seconds Range**: 0-50 (increment: 10)
- **Maximum Total Time**: 10:00 (10 minutes)
- **Wrap Behavior**:
  - Minutes: 10 → 0
  - Seconds: 50 → 0 (if minutes < 10), 50 → add minute + reset to 0

### 5.3 COUNTDOWN_ACTIVE State Controls
- **All buttons**: Disabled (no pause/stop functionality)
- **Display Update**: Every second tick

### 5.4 ALARM State Controls
- **Any button press/hold**: Dismiss alarm → return to SLEEP

## 6. Timer Logic

### 6.1 Countdown Behavior
```
Every second:
1. Decrement seconds by 1
2. If seconds < 0:
   - If minutes > 0:
     - Decrement minutes by 1
     - Set seconds to 59
   - Else:
     - Transition to ALARM state
3. Update display
4. Update progress visualization
```

## 7. Alarm Behavior

### 7.1 Visual Feedback
- **Display**: "00:00" blinking at 2Hz (250ms on, 250ms off)
- **LEDs**: Synchronized blinking with display

### 7.2 Duration
- **Maximum**: 10 seconds
- **Early Dismissal**: Any button interaction

### 7.3 Auto-dismiss
- After 10 seconds without user interaction → SLEEP state

## 8. Power Management

### 8.1 Display Control
- **OFF**: In SLEEP state
- **ON**: All other states
- **Redraw Policy**: Only on state change or value update

### 8.2 LED Control
- **OFF**: All states except ALARM
- **Blinking**: Only in ALARM state
