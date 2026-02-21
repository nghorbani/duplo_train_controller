#include <Arduino.h>
#include <esp_sleep.h>
#include "pins.h"
#include "power.h"

static unsigned long lastActivityTime = 0;

void resetActivityTimer() {
    lastActivityTime = millis();
}

unsigned long getLastActivityTime() {
    return lastActivityTime;
}

void enterDeepSleep() {
    Serial.println("[SLEEP] Inactivity timeout. Shutting down...");

    if (myHub.isConnected()) {
        myHub.setBasicMotorSpeed(port, 0);
        delay(200);
        myHub.playSound((byte)DuploTrainBaseSound::BRAKE);
        delay(500);
        myHub.shutDownHub();
        delay(500);
    }

    Serial.println("[SLEEP] Entering deep sleep. Power-cycle to restart.");
    Serial.flush();
    esp_deep_sleep_start();
}
