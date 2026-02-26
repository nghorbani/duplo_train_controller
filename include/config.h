#pragma once

// Potentiometer calibration — adjust after observing Min/Max in Serial Monitor.
// Handle fully forward (physically) reads LOW, handle fully backward reads HIGH.
#define POT_MIN              400    // ADC value when handle is fully FORWARD
#define POT_MAX              2300   // ADC value when handle is fully BACKWARD
#define POT_REVERSED         false  // Set to true if potentiometer GND/VCC are swapped
#define SPEED_MIN            20     // Minimum motor speed (both directions); range: SPEED_MIN..64
#define STOP_UNBLOCK_THRESHOLD 22   // speed-unit change needed to resume after emergency stop

// Inactivity auto-off configuration
#define INACTIVITY_TIMEOUT_MS  (10UL * 60UL * 1000UL)  // 10 minutes
#define POT_ACTIVITY_THRESHOLD 15  // minimum speed-unit change to count as pot movement

// Timing
#define DEBUG_INTERVAL         500UL   // Print debug every 500ms
#define BLE_RESEND_INTERVAL    500UL   // Resend BLE command every 500ms
#define BUTTON_DEBOUNCE_MS     25
#define LOOP_DELAY_MS          20
