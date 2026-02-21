# Changelog

All notable changes to this project (forked from [mav00/LDTrainRemote](https://github.com/mav00/LDTrainRemote)) are documented in this file.

## [Unreleased]
- Added ResponsiveAnalogRead library for adaptive potentiometer smoothing
- Added continuous speed mapping (SPEED_MIN..64) replacing discrete threshold steps (64, 32, 16, 0, -32, -64)
- Added reversed potentiometer mapping (low ADC = full forward, high ADC = full backward) to match physical handle direction
- Added potentiometer calibration constants (POT_MIN, POT_MAX) with inverted normalized 0-1000 range
- Added center dead zone (400-599 normalized) for reliable neutral/stop position
- Added minimum motor speed threshold (SPEED_MIN=20) to prevent stalling at low speeds
- Added potentiometer blocking mechanism on stop button press (unblocks when pot moves 10+ units)
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
