# "Bucket is empty/full" Application

This is a description of a simple embedded application running on a device with an LCD module. Read more about capabilities and API/SDK in src/docs.md.

The goal is to implement a simple screen and minimal logic for sending/receiving messages.

## Display Layout (persistent):
1. Updated       12.10. 13:00
2. Emptied              5.9.
3. Estimate           15.10.
4. Sensor              2.6V (or LOW/CRI)
5. Display             2.6V (or LOW/CRI)

Notice that labels are left-aligned and values are right-aligned.
Time format: 24h (HH:MM), Date format: D.M. (Czech standard)
CRI for battery levels = CRITICAL.

## Display States
The display color scheme indicates bucket state:
1. Empty bucket = black text on white background
2. Full bucket = white text on black background

## Initial State
Display shows "--" for all values until first successful update. No LED blinking in this state.

## Button Functions
- Button 1: Request immediate status update (sends standard request message)
- Button 2: Acknowledge alert (stops LED blinking until next triggering event)

## Alert Logic
All 6 RGB LEDs (see docs for details) blink red simultaneously (20 times at 2-second intervals) every 15 minutes when:
- Full bucket event received
- Critical battery level (sensor or display)
- No update received for 3+ hours (communication failure)

Blinking stops when:
- Alert is acknowledged by user pressing Button 2
- Bucket transitions from full to empty
- Successful update received (after communication failure alert)
- Battery returns to normal (LOW or voltage value)

Note: Blinking resumes if new, previously unacknowledged alert conditions arise after acknowledgment.

## Update Protocol
1. Device sends ready message to central point (on schedule or Button 1 press)
2. Device waits ~400ms for response
3. Response via MQTT with minimal JSON or CSV format:
   - Bucket state (empty/full)
   - Current date/time
   - Sensor battery (float voltage, "LOW", or "CRI")
   - Display battery (float voltage, "LOW", or "CRI")
   - Date of last emptying
   - Estimated full date

Message format should use minimal space (e.g., comma-separated values in string format).

## Error Handling
Failed/corrupted updates: Keep displaying previous values but start LED blinking pattern. User can verify staleness via "Updated" timestamp.

## Persistence (future enhancement)
Values could be stored across power cycles using EEPROM:
- https://sdk.hardwario.com/group__twr__config.html
- https://docs.hardwario.com/tower/firmware-sdk/how-to/how-to-eeprom-twr-config
