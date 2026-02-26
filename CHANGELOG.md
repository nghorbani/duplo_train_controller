# Changelog

All notable changes to this project (forked from [mav00/LDTrainRemote](https://github.com/mav00/LDTrainRemote)) are documented in this file.

## [v1.1.0] - 2026-02-26
- Fixed stop mode on initial BLE connect: throttle block now captures the actual current throttle position (not a hardcoded 0), preventing the train from immediately unblocking and moving if the throttle was already past the resume threshold at connect time
- Extracted shared applyStopMode() function used by both the stop button and the connect path, ensuring identical stop-mode and unblock behaviour in both cases; also plays the brake sound on connect as audible confirmation
- Fixed stop button double-press bug: pressing stop again while already blocked no longer resets speedAtBlock to 0, which previously caused the train to immediately resume
- Added throttle block on initial BLE connection so the train stays still regardless of throttle position until the user moves it past the unblock threshold
- Increased STOP_UNBLOCK_THRESHOLD by 10% (20 → 22) for safer resume behavior

## [v1.0.0] - 2026-02-21
- Refactored monolithic main.cpp into separate modules: config.h, pins.h, power.h/.cpp, train_control.h/.cpp, buttons.h/.cpp
- Moved all tunable #define constants into include/config.h for single-file configuration
- Moved pin assignments and hardware extern declarations into include/pins.h
- Extracted inactivity tracking and deep sleep logic into power module (src/power.cpp)
- Extracted potentiometer reading, speed mapping, and motor control into train_control module (src/train_control.cpp)
- Extracted button handling and LED color cycling into buttons module (src/buttons.cpp)
- Decomposed 110-line handlePoti() into focused helper functions (readAndNormalizePot, checkPotUnblock, sendMotorCommand, updateSteamSound, trackPotActivity, printDebug)
- Replaced magic numbers (25ms debounce, 20ms loop delay) with named constants BUTTON_DEBOUNCE_MS and LOOP_DELAY_MS
- Updated README.md to reference include/config.h instead of src/main.cpp for configuration
- Changed departure sound from STATION_DEPARTURE to HORN for consistency with music button
- Added inactivity auto-off: ESP32 enters deep sleep after 10 minutes of no button presses or potentiometer movement; stops the train motor and shuts down the LEGO hub via BLE before sleeping (power-cycle to restart)
- Added steam / choo-choo sound (DuploTrainBaseSound::STEAM) that plays once when potentiometer reaches maximum forward speed; re-arms after pot returns to dead zone
- Added INACTIVITY_TIMEOUT_MS and POT_ACTIVITY_THRESHOLD configuration constants
- Added ResponsiveAnalogRead library for adaptive potentiometer smoothing
- Added continuous speed mapping (SPEED_MIN..64) replacing discrete threshold steps (64, 32, 16, 0, -32, -64)
- Added reversed potentiometer mapping (low ADC = full forward, high ADC = full backward) to match physical handle direction
- Added potentiometer calibration constants (POT_MIN, POT_MAX) with inverted normalized 0-1000 range
- Added center dead zone (400-599 normalized) for reliable neutral/stop position
- Added minimum motor speed threshold (SPEED_MIN=20) to prevent stalling at low speeds
- Added STOP_UNBLOCK_THRESHOLD configuration constant (default 20) for emergency stop resume sensitivity
- Added potentiometer blocking mechanism on stop button press (unblocks when pot moves STOP_UNBLOCK_THRESHOLD+ units)
- Added periodic BLE command resend every 500ms while motor is running
- Added brake sound effect when decelerating to stop via potentiometer
- Added comprehensive serial debug output (raw/filtered ADC, voltage, min/max tracking, BLE status)
- Added startup calibration info printed to serial monitor
- Added ESP32 12-bit ADC resolution configuration for ResponsiveAnalogRead
- Added debounce intervals (25ms) to all four buttons
- Changed pin assignments: BTN_MUSIC 25->18, BTN_LICHT 26->19, BTN_WASSER 27->22, BTN_STOP 14->23
- Changed main loop delay from 50ms to 20ms for more responsive control
- Changed stop button to set potBlocked flag and track speed at time of stop
- Changed platformio.ini upload/monitor port from Linux (//dev/ttyUSB0) to Windows (COM6)
- Changed platformio.ini to add upload_speed (115200) and monitor_port (COM6)
- Removed gLastStatePtiSpeed variable (was unused in original)
- Removed vendored lib/legoino-master directory; Legoino is now fetched via PlatformIO lib_deps
- Removed lib_extra_dirs from platformio.ini (no longer needed)
