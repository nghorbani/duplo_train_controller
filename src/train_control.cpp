#include <Arduino.h>
#include "config.h"
#include "pins.h"
#include "train_control.h"
#include "power.h"

// --- Shared state (extern in train_control.h) ---
int gSpeed = 0;
bool potBlocked = false;
int speedAtBlock = 0;

// --- Private state ---
static int debugRawMin = 4095;
static int debugRawMax = 0;
static unsigned long lastDebugPrint = 0;
static unsigned long lastBleSend = 0;
static int lastActivitySpeed = 0;
static bool steamPlayed = false;

// --- Internal helpers ---

struct PotReading {
    int rawValue;
    int filtered;
    float voltage;
    int normalized;
    int speed;
};

static PotReading readAndNormalizePot() {
    PotReading r;

    r.rawValue = analogRead(PTI_SPEED);

    potReader.update();
    r.filtered = potReader.getValue();

    r.voltage = r.filtered * 3.3f / 4095.0f;

    // Normalize to 0-1000 range.  1000 = full forward, 500 = center/stop, 0 = full backward.
    r.normalized = constrain(r.filtered, POT_MIN, POT_MAX);
    r.normalized = POT_REVERSED
        ? map(r.normalized, POT_MIN, POT_MAX, 0, 1000)
        : map(r.normalized, POT_MIN, POT_MAX, 1000, 0);

    // Map normalized value to speed with dead zone in center
    //  600-1000 → forward:  SPEED_MIN to 64
    //  400-599  → STOP dead zone
    //    0-399  → backward: -SPEED_MIN to -64
    if      (r.normalized >= 600) r.speed = map(r.normalized, 600, 1000, SPEED_MIN, 64);
    else if (r.normalized >= 400) r.speed = 0;
    else                          r.speed = map(r.normalized, 399, 0, -SPEED_MIN, -64);

    return r;
}

static bool checkPotUnblock(int speed) {
    if (!potBlocked) return false;

    if (abs(speed - speedAtBlock) >= STOP_UNBLOCK_THRESHOLD) {
        potBlocked = false;
        return false;  // unblocked
    }
    return true;  // still blocked
}

void applyStopMode() {
    if (!potBlocked) {
        // Capture current throttle position so the unblock threshold is measured
        // from where the throttle actually is, not from a hardcoded 0.
        potReader.update();
        PotReading r = readAndNormalizePot();
        speedAtBlock = r.speed;
    }
    potBlocked = true;
    gSpeed = 0;
    myHub.setBasicMotorSpeed(port, 0);
    delay(100);
    myHub.playSound((byte)DuploTrainBaseSound::BRAKE);
}

static const char* sendMotorCommand(int speed, unsigned long now) {
    if (speed != gSpeed) {
        if (gSpeed == 0 && speed > 0) {
            myHub.playSound((byte)DuploTrainBaseSound::HORN);
            delay(100);
        }
        if (speed == 0 && gSpeed != 0) {
            myHub.setBasicMotorSpeed(port, 0);
            delay(100);
            myHub.playSound((byte)DuploTrainBaseSound::BRAKE);
            delay(100);
        }
        gSpeed = speed;
        myHub.setBasicMotorSpeed(port, speed);
        lastBleSend = now;
        return "SENT(changed)";
    }

    if (speed != 0 && now - lastBleSend >= BLE_RESEND_INTERVAL) {
        myHub.setBasicMotorSpeed(port, gSpeed);
        lastBleSend = now;
        return "resent";
    }

    return "skip";
}

static void updateSteamSound(int speed) {
    if (speed >= 64 && !steamPlayed) {
        myHub.playSound((byte)DuploTrainBaseSound::HORN);
        delay(100);
        steamPlayed = true;
    } else if (speed == 0) {
        steamPlayed = false;
    }
}

static void trackPotActivity(int speed) {
    if (abs(speed - lastActivitySpeed) >= POT_ACTIVITY_THRESHOLD) {
        lastActivitySpeed = speed;
        resetActivityTimer();
    }
}

static void printDebug(const PotReading& r, const char* bleStatus, unsigned long now) {
    if (now - lastDebugPrint >= DEBUG_INTERVAL) {
        lastDebugPrint = now;
        Serial.printf("[POT] Raw:%4d | Filt:%4d | V:%.2f | Norm:%4d | Min:%4d Max:%4d | Speed:%3d | BLE:%s\n",
                      r.rawValue, r.filtered, r.voltage,
                      r.normalized, debugRawMin, debugRawMax, r.speed, bleStatus);
    }
}

// --- Public entry point ---

void handlePoti() {
    PotReading reading = readAndNormalizePot();

    // Track min/max of raw readings
    if (reading.rawValue < debugRawMin) debugRawMin = reading.rawValue;
    if (reading.rawValue > debugRawMax) debugRawMax = reading.rawValue;

    unsigned long now = millis();

    if (checkPotUnblock(reading.speed)) {
        printDebug(reading, "BLOCKED", now);
        return;
    }

    const char* bleStatus = sendMotorCommand(reading.speed, now);
    updateSteamSound(reading.speed);
    trackPotActivity(reading.speed);
    printDebug(reading, bleStatus, now);
}
