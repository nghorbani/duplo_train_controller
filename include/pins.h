#pragma once

#include <Lpf2Hub.h>
#include <Bounce2.h>
#include <ResponsiveAnalogRead.h>

// Pin assignments
#define BTN_MUSIC   18
#define BTN_LICHT   19
#define BTN_WASSER  22
#define BTN_STOP    23
#define PTI_SPEED   15

// Hardware objects (defined in main.cpp)
extern Lpf2Hub myHub;
extern byte port;

extern Bounce pbMusic;
extern Bounce pbLight;
extern Bounce pbWater;
extern Bounce pbStop;

extern ResponsiveAnalogRead potReader;
