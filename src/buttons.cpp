#include <Arduino.h>
#include <Lpf2HubConst.h>
#include "pins.h"
#include "buttons.h"
#include "train_control.h"
#include "power.h"

// --- Private state ---
static int gLightOn = 0;
static short gColor = NONE;

static Color getNextColor() {
    if (gColor == 255) {
        gColor = 0;
        return (Color)0;
    }

    gColor++;

    if (gColor == NUM_COLORS) {
        gColor = 255;
    }
    return (Color)gColor;
}

void handleButtons() {
    if (pbMusic.update()) {
        if (pbMusic.fell()) {
            resetActivityTimer();
            myHub.playSound((byte)DuploTrainBaseSound::HORN);
            delay(100);
        }
    }
    if (pbLight.update()) {
        if (pbLight.fell()) {
            resetActivityTimer();
            myHub.setLedColor(getNextColor());
            delay(100);
        }
    }
    if (pbWater.update()) {
        if (pbWater.fell()) {
            resetActivityTimer();
            myHub.playSound((byte)DuploTrainBaseSound::WATER_REFILL);
            delay(300);
        }
    }
    if (pbStop.update()) {
        if (pbStop.fell()) {
            resetActivityTimer();
            applyStopMode();
        }
    }
}
