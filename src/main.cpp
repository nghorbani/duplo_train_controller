/**
 * LEGO Duplo Train BLE Remote Controller
 *
 * Controls a LEGO Duplo Train via Bluetooth Low Energy (BLE) using an ESP32
 * microcontroller. A potentiometer provides variable speed control (forward,
 * stop, and reverse), while four momentary buttons trigger horn, light colour
 * cycling, water refill sound, and emergency stop.
 *
 * Hardware setup:
 *   Board  : ESP32 NodeMCU-32S (CP2102 USB)
 *   Pot    : pin 15 (PTI_SPEED) — linear potentiometer for speed
 *   Buttons: pin 18 (BTN_MUSIC)  — horn / music sound
 *            pin 19 (BTN_LICHT)  — cycle LED colour
 *            pin 22 (BTN_WASSER) — water refill sound
 *            pin 23 (BTN_STOP)   — emergency stop (blocks pot until moved)
 *
 * Configuration: see include/config.h
 * Libraries: Legoino, NimBLE-Arduino, Bounce2, ResponsiveAnalogRead
 */

#include <Arduino.h>
#include "config.h"
#include "pins.h"
#include "power.h"
#include "train_control.h"
#include "buttons.h"

// --- Hardware object definitions (externed via pins.h) ---
Lpf2Hub myHub;
byte port = (byte)PoweredUpHubPort::A;

Bounce pbMusic = Bounce();
Bounce pbLight = Bounce();
Bounce pbWater = Bounce();
Bounce pbStop = Bounce();

ResponsiveAnalogRead potReader(PTI_SPEED, true);

void setup() {
    Serial.begin(115200);

    // ESP32 ADC is 12-bit; tell ResponsiveAnalogRead about it
    potReader.setAnalogResolution(4096);

    // Attach buttons with debounce
    pbMusic.attach(BTN_MUSIC, INPUT_PULLUP);
    pbMusic.interval(BUTTON_DEBOUNCE_MS);
    pbLight.attach(BTN_LICHT, INPUT_PULLUP);
    pbLight.interval(BUTTON_DEBOUNCE_MS);
    pbWater.attach(BTN_WASSER, INPUT_PULLUP);
    pbWater.interval(BUTTON_DEBOUNCE_MS);
    pbStop.attach(BTN_STOP, INPUT_PULLUP);
    pbStop.interval(BUTTON_DEBOUNCE_MS);

    myHub.init();

    Serial.println("=== POT CALIBRATION ===");
    Serial.printf("POT_MIN (full forward): %d\n", POT_MIN);
    Serial.printf("POT_MAX (full backward): %d\n", POT_MAX);
    Serial.printf("POT_REVERSED: %s\n", POT_REVERSED ? "true" : "false");
    Serial.println("If these don't match your pot, update POT_MIN/POT_MAX in include/config.h.");
    Serial.println("=======================");

    resetActivityTimer();
}

void loop() {
    // Auto-off after inactivity
    if (millis() - getLastActivityTime() >= INACTIVITY_TIMEOUT_MS) {
        enterDeepSleep();
    }

    if (myHub.isConnecting()) {
        myHub.connectHub();
        if (myHub.isConnected()) {
            Serial.println("We are now connected to the HUB");
            applyStopMode();
        } else {
            Serial.println("We have failed to connect to the HUB");
        }
    }
    if (myHub.isConnected()) {
        handleButtons();
        handlePoti();
    }
    delay(LOOP_DELAY_MS);
}
