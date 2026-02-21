# Duplo Train Controller

An ESP32-based BLE remote controller for the LEGO Duplo Train. Uses a
potentiometer for variable speed (forward / stop / reverse) and four buttons
for horn, LED colour, water refill sound, and emergency stop.

This is the codebase for the Lego Duplo Train controller project. The 3D-printed remote enclosure is available on:

- **MakerWorld:** <https://makerworld.com/en/models/462749-remote-compatible-with-brick-duplo-train-on-esp32?from=search#profileId-716511>
- **Thingiverse:** <https://www.thingiverse.com/thing:6620027>

## Hardware

| Component        | Details |
|------------------|---------|
| **Board**        | BT 2.4G WLAN-Modul ESP32-32 CP2102 USB Micro ([AliExpress](https://de.aliexpress.com/item/1005006987264613.html?spm=a2g0o.order_list.order_list_main.5.30555c5fLBoiG0&gatewayAdapt=glo2deu)) |
| **Potentiometer**| ([10K Linear pot](https://de.aliexpress.com/item/1005007593151146.html?spm=a2g0o.order_list.order_list_main.29.1d535c5fEI0K7B&gatewayAdapt=glo2deu)) connected to **pin 15** |
| **BTN_MUSIC**    | Momentary push-button on **pin 18** -- horn sound |
| **BTN_LICHT**    | Momentary push-button on **pin 19** -- cycle LED colour |
| **BTN_WASSER**   | Momentary push-button on **pin 22** -- water refill sound |
| **BTN_STOP**     | Momentary push-button on **pin 23** -- emergency stop |

All buttons use the ESP32 internal pull-up resistors (`INPUT_PULLUP`), so
each button should connect its pin to GND when pressed.

## Features

- **Continuous speed control** — potentiometer mapped to forward / stop / reverse with smooth ramping (speed range `SPEED_MIN` to 64)
- **Adaptive analog smoothing** — ResponsiveAnalogRead library filters ADC noise for jitter-free input
- **Center dead zone** — a configurable band around the midpoint where speed is zero, preventing accidental movement
- **Emergency stop with pot-lock** — the stop button immediately halts the motor; the potentiometer stays locked until moved past a configurable threshold (`STOP_UNBLOCK_THRESHOLD`)
- **Inactivity auto-sleep** — after 10 minutes of no input the controller stops the train, shuts down BLE, and puts the ESP32 into deep sleep to save battery
- **Steam sound at max speed** — plays a horn sound when the motor reaches full throttle (once per acceleration cycle)
- **BLE command resending** — periodically resends motor commands to guard against dropped BLE packets
- **Modular codebase** — split from the original monolithic `main.cpp` into focused modules: `train_control`, `buttons`, `power`, and shared `config`/`pins` headers
- **Centralized configuration** — all tunable parameters (pot calibration, timeouts, thresholds, pin assignments) live in `include/config.h`

## Programming (VSCode + PlatformIO)

1. Install [Visual Studio Code](https://code.visualstudio.com/).
2. Install the **PlatformIO IDE** extension from the VSCode marketplace.
3. Open this project folder in VSCode (**File > Open Folder**).
4. PlatformIO will automatically detect `platformio.ini` and install the
   required libraries (`Legoino`, `NimBLE-Arduino`, `Bounce2`,
   `ResponsiveAnalogRead`).
5. Connect the ESP32 board via USB.
6. Click the **PlatformIO: Upload** button (arrow icon in the bottom toolbar)
   or run `pio run -t upload` in the terminal to compile and flash the firmware.
7. Open the **Serial Monitor** (plug icon in the PlatformIO toolbar, or run
   `pio device monitor`) at **115200 baud** to view debug output.

> **Note:** The upload and monitor port is configured as `COM6` in
> `platformio.ini`. Change it to match your system if needed.

## Configuration

All configuration is done via `#define` constants in
`include/config.h`. After changing any value, re-upload the firmware.

### POT_MIN / POT_MAX (Potentiometer ADC Range)

The ESP32 ADC is 12-bit (0--4095). The potentiometer rarely uses the full
range, so `POT_MIN` and `POT_MAX` define the actual endpoints.

**How to calibrate:**
1. Upload the firmware and open the Serial Monitor at 115200 baud.
2. Move the potentiometer handle fully forward and fully backward several
   times.
3. Watch the `Min:` and `Max:` values in the debug output -- these are the
   observed raw ADC extremes.
4. Replace `POT_MIN` with the observed minimum and `POT_MAX` with the observed
   maximum in `include/config.h`, then re-upload.

Default values: `POT_MIN = 400`, `POT_MAX = 2300`.

### POT_REVERSED (Potentiometer Direction)

If the potentiometer is wired with GND and VCC swapped (i.e. forward motion
produces high ADC instead of low), set:

```cpp
#define POT_REVERSED true
```

This flips the internal mapping so the control direction is correct without
re-wiring. Default: `false`.

### SPEED_MIN (Minimum Motor Speed)

The lowest speed value sent to the train motor (in both directions). Values
below this threshold cause the motor to stall rather than turn. Increase if
the train struggles to start; decrease for finer low-speed control.

Default: `20` (valid range sent to motor: `SPEED_MIN` to `64`).

### Dead Zone

The normalised potentiometer range (0--1000) is split into three regions:

| Normalised range | Behaviour |
|------------------|-----------|
| 600 -- 1000      | **Forward** -- speed ramps from `SPEED_MIN` to 64 |
| 400 -- 599       | **Dead zone** -- motor stopped (speed = 0) |
| 0 -- 399         | **Backward** -- speed ramps from `-SPEED_MIN` to -64 |

The dead zone in the centre prevents accidental movement when the handle is
near the midpoint.

### STOP_UNBLOCK_THRESHOLD (Emergency Stop Resume Sensitivity)

After the emergency stop button is pressed, the potentiometer is locked until
the calculated speed changes by at least `STOP_UNBLOCK_THRESHOLD` units from
the speed at the moment of the stop. This prevents accidental resumption from
minor potentiometer drift.

Increase for a larger "dead zone" after stop (safer but requires more handle
movement to resume); decrease for quicker re-engagement.

Default: `20` (speed range is -64 to 64).

### Pin Assignments

| Define       | Pin | Function |
|--------------|-----|----------|
| `PTI_SPEED`  | 15  | Potentiometer analog input |
| `BTN_MUSIC`  | 18  | Horn / music sound |
| `BTN_LICHT`  | 19  | Cycle LED colour |
| `BTN_WASSER` | 22  | Water refill sound |
| `BTN_STOP`   | 23  | Emergency stop (blocks pot until handle is moved) |

## Serial Monitor (Debug Info)

Open the Serial Monitor at **115200 baud** to get real-time debug output.
The firmware prints a status line every 500 ms:

```
[POT] Raw:1842 | Filt:1840 | V:1.48 | Norm: 241 | Min: 398 Max:2301 | Speed:-28 | BLE:SENT(changed)
```

| Field   | Meaning |
|---------|---------|
| `Raw`   | Unfiltered ADC reading |
| `Filt`  | Smoothed ADC (via ResponsiveAnalogRead) |
| `V`     | Voltage of filtered reading (0--3.3 V) |
| `Norm`  | Normalised position (0--1000; 1000 = full forward) |
| `Min`/`Max` | Observed raw ADC extremes since boot (use these for calibrating `POT_MIN`/`POT_MAX`) |
| `Speed` | Motor speed sent to train (-64 to 64, 0 = stopped) |
| `BLE`   | BLE status: `SENT(changed)`, `resent`, `skip`, or `BLOCKED` |

Use the `Min` and `Max` values to calibrate `POT_MIN` and `POT_MAX` as
described above.

## License

This project is licensed under the GNU General Public License v3.0.
See [LICENSE](LICENSE) for details.
