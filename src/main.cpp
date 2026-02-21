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
 * Configuration (adjust the #defines below):
 *   POT_MIN / POT_MAX — ADC range of the potentiometer (read via Serial Monitor)
 *   POT_REVERSED      — set to true if pot GND/VCC wiring is swapped
 *   SPEED_MIN          — minimum motor speed to avoid stalling (range: SPEED_MIN..64)
 *
 * Libraries: Legoino, NimBLE-Arduino, Bounce2, ResponsiveAnalogRead
 */

#include <Lpf2Hub.h>      //legoino
#include <Lpf2HubConst.h> //legoino
#include <Bounce2.h>      //bounce2
#include <ResponsiveAnalogRead.h>
#include <esp_sleep.h>

Lpf2Hub myHub;
byte port = (byte)PoweredUpHubPort::A;

//Pin declaration
#define BTN_MUSIC 18
#define BTN_LICHT 19
#define BTN_WASSER 22
#define BTN_STOP 23
#define PTI_SPEED 15

Bounce pbMusic = Bounce();
Bounce pbLight = Bounce();
Bounce pbWater = Bounce();
Bounce pbStop = Bounce();

// Filtered potentiometer reader (adaptive smoothing)
ResponsiveAnalogRead potReader(PTI_SPEED, true);

// Potentiometer calibration — adjust after observing Min/Max in Serial Monitor.
// Handle fully forward (physically) reads LOW, handle fully backward reads HIGH.
#define POT_MIN 400   // ADC value when handle is fully FORWARD
#define POT_MAX 2300  // ADC value when handle is fully BACKWARD
#define POT_REVERSED false  // Set to true if potentiometer GND/VCC are swapped
#define SPEED_MIN 20  // Minimum motor speed (both directions); range: SPEED_MIN..64
#define STOP_UNBLOCK_THRESHOLD 20  // speed-unit change needed to resume after emergency stop

// Inactivity auto-off configuration
#define INACTIVITY_TIMEOUT_MS  (10UL * 60UL * 1000UL)  // 10 minutes
#define POT_ACTIVITY_THRESHOLD 15  // minimum speed-unit change to count as pot movement

// Debug tracking
int debugRawMin = 4095;
int debugRawMax = 0;
unsigned long lastDebugPrint = 0;
unsigned long lastBleSend = 0;
const unsigned long DEBUG_INTERVAL = 500;       // Print debug every 500ms
const unsigned long BLE_RESEND_INTERVAL = 500;  // Resend BLE command every 500ms

static int gLightOn = 0;
static short gColor = NONE;
static int gSpeed = 0;
static bool potBlocked = false;
static int speedAtBlock = 0;

// Inactivity tracking
static unsigned long lastActivityTime = 0;
static int lastActivitySpeed = 0;

// Max-speed steam sound: plays once at full forward, re-arms when pot returns to dead zone
static bool steamPlayed = false;

void resetActivityTimer() {
    lastActivityTime = millis();
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

void handlePoti()
{
  // Read raw value for debug comparison
  int rawValue = analogRead(PTI_SPEED);

  // Update filtered reader and get smoothed value
  potReader.update();
  int filtered = potReader.getValue();

  // Track min/max of raw readings
  if (rawValue < debugRawMin) debugRawMin = rawValue;
  if (rawValue > debugRawMax) debugRawMax = rawValue;

  // Calculate voltage (ESP32: 12-bit ADC, 3.3V reference)
  float voltage = rawValue * 3.3f / 4095.0f;
  float voltageFiltered = filtered * 3.3f / 4095.0f;

  // Normalize to 0-1000 range.  Result: 1000 = full forward, 500 = center/stop, 0 = full backward.
  // Mapping is inverted by default (low ADC = forward); POT_REVERSED flips it for swapped wiring.
  int normalized = constrain(filtered, POT_MIN, POT_MAX);
  normalized = POT_REVERSED
    ? map(normalized, POT_MIN, POT_MAX, 0, 1000)
    : map(normalized, POT_MIN, POT_MAX, 1000, 0);

  // Map normalized value to continuous speed with dead zone in center
  //  600-1000 → forward:  SPEED_MIN to 64 (continuous)
  //  400-599  → STOP dead zone (0, sent once)
  //    0-399  → backward: -SPEED_MIN to -64 (continuous)
  int speed = 0;
  if      (normalized >= 600) speed = map(normalized, 600, 1000, SPEED_MIN, 64);
  else if (normalized >= 400) speed = 0;    // STOP (dead zone)
  else                        speed = map(normalized, 399, 0, -SPEED_MIN, -64);

  // If pot is blocked by stop button, check if pot has moved enough to unblock
  unsigned long now = millis();
  const char* bleStatus = "skip";

  if (potBlocked)
  {
    if (abs(speed - speedAtBlock) >= STOP_UNBLOCK_THRESHOLD)
    {
      potBlocked = false;  // Pot moved enough — unblock
    }
    else
    {
      bleStatus = "BLOCKED";
      // Debug output every DEBUG_INTERVAL ms
      if (now - lastDebugPrint >= DEBUG_INTERVAL)
      {
        lastDebugPrint = now;
        Serial.printf("[POT] Raw:%4d | Filt:%4d | V:%.2f | Norm:%4d | Min:%4d Max:%4d | Speed:%3d | BLE:%s\n",
                      rawValue, filtered, voltageFiltered,
                      normalized, debugRawMin, debugRawMax, speed, bleStatus);
      }
      return;  // Don't send anything while blocked
    }
  }

  // Send BLE command: immediately on change, periodically resend only when moving
  if (speed != gSpeed)
  {
    if (gSpeed == 0 && speed > 0)
    {
      myHub.playSound((byte)DuploTrainBaseSound::STATION_DEPARTURE);
      delay(100);
    }
    if (speed == 0 && gSpeed != 0)
    {
      myHub.setBasicMotorSpeed(port, 0);
      delay(100);
      myHub.playSound((byte)DuploTrainBaseSound::BRAKE);
      delay(100);
    }
    gSpeed = speed;
    myHub.setBasicMotorSpeed(port, speed);
    lastBleSend = now;
    bleStatus = "SENT(changed)";
  }
  else if (speed != 0 && now - lastBleSend >= BLE_RESEND_INTERVAL)
  {
    // Only resend periodically when moving; stop is sent once and that's it
    myHub.setBasicMotorSpeed(port, gSpeed);
    lastBleSend = now;
    bleStatus = "resent";
  }

  // Play steam sound once when pot reaches max forward; re-arms after returning to dead zone
  if (speed >= 64 && !steamPlayed) {
    myHub.playSound((byte)DuploTrainBaseSound::STEAM);
    delay(100);
    steamPlayed = true;
  } else if (speed == 0) {
    steamPlayed = false;
  }

  // Detect potentiometer movement for inactivity tracking
  if (abs(speed - lastActivitySpeed) >= POT_ACTIVITY_THRESHOLD) {
    lastActivitySpeed = speed;
    resetActivityTimer();
  }

  // Debug output every DEBUG_INTERVAL ms
  if (now - lastDebugPrint >= DEBUG_INTERVAL)
  {
    lastDebugPrint = now;
    Serial.printf("[POT] Raw:%4d | Filt:%4d | V:%.2f | Norm:%4d | Min:%4d Max:%4d | Speed:%3d | BLE:%s\n",
                  rawValue, filtered, voltageFiltered,
                  normalized, debugRawMin, debugRawMax, speed, bleStatus);
  }
}


Color getNextColor()
{
    if(gColor == 255)
    { 
        gColor = 0;
        return (Color)0;
    }
       
    gColor++;
    
    if(gColor == NUM_COLORS)
    {   
        gColor =255;
    }
    return (Color) gColor;
} 

void handleButtons()
{
  if(pbMusic.update())
  {
    if(pbMusic.fell())
    {
      resetActivityTimer();
      myHub.playSound((byte)DuploTrainBaseSound::HORN);
      delay(100);
    }
  }
  if(pbLight.update())
  {
    if(pbLight.fell())
    {
      resetActivityTimer();
      myHub.setLedColor(getNextColor());
      delay(100);
    }
  }
  if(pbWater.update())
  {
    if(pbWater.fell())
    {
      resetActivityTimer();
      myHub.playSound((byte)DuploTrainBaseSound::WATER_REFILL);
      delay(100);
    }
  }
  if(pbStop.update())
  {
    if(pbStop.fell())
    {
      resetActivityTimer();
      potBlocked = true;
      speedAtBlock = gSpeed;
      gSpeed = 0;
      myHub.setBasicMotorSpeed(port, 0);
      delay(100);
      myHub.playSound((byte)DuploTrainBaseSound::BRAKE);
    }
  }
}

void setup() {
  Serial.begin(115200);

  // ESP32 ADC is 12-bit; tell ResponsiveAnalogRead about it
  potReader.setAnalogResolution(4096);

  //define Pin Modes
  pbMusic.attach(BTN_MUSIC, INPUT_PULLUP);
  pbMusic.interval(25);
  pbLight.attach(BTN_LICHT, INPUT_PULLUP);
  pbLight.interval(25);
  pbWater.attach(BTN_WASSER, INPUT_PULLUP);
  pbWater.interval(25);
  pbStop.attach(BTN_STOP, INPUT_PULLUP);
  pbStop.interval(25);

  myHub.init();

  Serial.println("=== POT CALIBRATION ===");
  Serial.printf("POT_MIN (full forward): %d\n", POT_MIN);
  Serial.printf("POT_MAX (full backward): %d\n", POT_MAX);
  Serial.printf("POT_REVERSED: %s\n", POT_REVERSED ? "true" : "false");
  Serial.println("If these don't match your pot, update POT_MIN/POT_MAX in code.");
  Serial.println("=======================");

  lastActivityTime = millis();
}

void loop() {

    // Auto-off after inactivity
    if (millis() - lastActivityTime >= INACTIVITY_TIMEOUT_MS) {
        enterDeepSleep();
    }

    if (myHub.isConnecting()) {
        myHub.connectHub();
        if (myHub.isConnected()) {
            Serial.println("We are now connected to the HUB");
        } else {
            Serial.println("We have failed to connect to the HUB");
        }
    }
    if (myHub.isConnected()) {
        handleButtons();
        handlePoti();
    } 
    delay(20);
}