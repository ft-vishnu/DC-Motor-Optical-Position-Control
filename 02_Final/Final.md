# OPQC: Optical Position Control

Closed-loop angular position control of a DC motor using an ESP32-S3, a TB6612FNG motor driver and a single-channel optical encoder. The ESP32 hosts its own Wi-Fi access point and a web dashboard with a live virtual dial, position commands, homing and an emergency stop.

---

## Table of Contents

1. [Features](#features)
2. [Hardware](#hardware)
3. [Wiring](#wiring)
4. [Encoder Disc Requirements](#encoder-disc-requirements)
5. [Software Setup](#software-setup)
6. [Quick Start](#quick-start)
7. [Using the Web Dashboard](#using-the-web-dashboard)
8. [How It Works](#how-it-works)
9. [Configuration Reference](#configuration-reference)
10. [HTTP API](#http-api)
11. [Serial Monitor Output](#serial-monitor-output)
12. [Tuning Guide](#tuning-guide)
13. [Troubleshooting](#troubleshooting)
14. [Known Limitations](#known-limitations)

---

## Features

- **PD position control** in 18° steps (20 counts per revolution)
- **Marker-based homing**: finds a wide marker slot on the encoder disc, independent of motor speed
- **State-machine firmware**: every operation has a defined start, end and failure path
- **Self-correcting moves**: after braking and settling, the final position is verified and corrected (up to 3 passes)
- **Safety watchdogs**: stall detection and move/home timeouts raise a `FAULT` state instead of running forever
- **Interrupt-timed encoder**: edge-triggered ISR with glitch filtering and exact pulse-width capture
- **Self-hosted web UI**: no router or internet needed, works from any phone or laptop
- **Virtual dial** that follows the *actual* encoder angle (never the commanded angle) and always rotates the short way round

---

## Hardware

| Part | Notes |
|---|---|
| ESP32-S3 dev board | Arduino framework |
| TB6612FNG motor driver | Channel A is used |
| DC motor | Sized for your supply voltage |
| Single-channel optical encoder | Slotted optical sensor + disc, 20 slots plus one wide marker |
| Motor power supply | Connected to the TB6612 `VM` pin |

---

## Wiring

| Signal | ESP32-S3 GPIO | Connects to |
|---|---|---|
| Encoder output | **4** | Optical sensor output (internal pull-up enabled) |
| AIN1 | **15** | TB6612 `AIN1` |
| AIN2 | **16** | TB6612 `AIN2` |
| PWMA | **17** | TB6612 `PWMA` |
| STBY | **18** | TB6612 `STBY` |

Other connections:

- TB6612 `AO1` / `AO2` go to the motor terminals.
- TB6612 `VM` goes to the motor supply, and `VCC` to 3.3 V logic.
- **All grounds must be common**: ESP32, TB6612, encoder and motor supply.
- Swapping the motor leads reverses the physical direction. Positive counts correspond to `AIN1=HIGH, AIN2=LOW`.

---

## Encoder Disc Requirements

The firmware assumes:

- **20 normal slots per revolution**, giving 18° per count.
- **One wider slot (the home marker)** that produces a HIGH pulse noticeably longer than the others (by default more than 2× the average).
- The sensor output is **HIGH in the slot/marker** and counts happen on **rising edges**.

If your sensor polarity is inverted, the marker will not be detected. Invert the logic or flip the sensor wiring.

---

## Software Setup

1. Install the Arduino IDE (or PlatformIO) with the **ESP32 Arduino core 3.x**. Version 3.x is needed for `analogWrite()`.
2. Select your ESP32-S3 board and the correct USB/COM port.
3. Open `OPQC.ino`.
4. Upload.

No external libraries are needed. `WiFi.h` and `WebServer.h` ship with the ESP32 core.

---

## Quick Start

1. Power the motor supply and the ESP32.
2. Open the Serial Monitor at **115200 baud**. You should see `System READY` and the IP address.
3. On your phone or laptop, join Wi-Fi:
   - **SSID:** `OPQC_Motor`
   - **Password:** `12345678`
4. Browse to **`http://192.168.4.1`** (the default ESP32 AP address; the serial monitor prints it).
5. Press **HOME** first so the system knows where zero is.
6. Use **Quick Angles** or type an angle and press **ROTATE**.

> Change the SSID and password in the source before using this anywhere other than your bench.

---

## Using the Web Dashboard

| Element | Purpose |
|---|---|
| **Current Position** | Actual angle and raw encoder count |
| **Virtual dial** | Pointer follows the real encoder angle, 0° at top, clockwise positive |
| **Angle input + ROTATE** | Relative move. Must be a non-zero multiple of 18 |
| **Quick Angles** | One-tap relative moves: ±18, ±36, ±54, ±90, ±180 |
| **STOP** | Brakes immediately, aborting any move or homing. Always enabled |
| **HOME** | Runs the homing routine |
| **System Status** | `READY`, `HOME`, `MOVING`, `HOMING`, `STOPPING`, `STOPPED`, `FAULT`, `OFFLINE` |
| **Homed / Target / Message** | Whether zero is established, the current target count, and the last result |

While the system is busy, all command buttons except STOP are disabled. Rejected commands show a red message under the buttons.

**Moves are relative:** `+90` means "90° from where the motor is now", not "go to 90°".

---

## How It Works

### State machine

```
READY ──move──► MOVING ──arrived──► BRAKING ──► SETTLING ──► verify ──► READY
                  ▲                                             │
                  └────────── correction (max 3) ◄──────────────┘

READY ──home──► HOME_CLEAR ──► HOME_SEARCH ──marker──► BRAKING ──► SETTLING ──► READY (homed, count = 0)

Any busy state ──STOP──► BRAKING ──► SETTLING ──► STOPPED
Any busy state ──stall / timeout──► FAULT
```

`STOPPED` and `FAULT` accept new move or home commands.

### Encoder

- The ISR fires on **both edges** with a 100 µs debounce window.
- **Rising edge:** adds the commanded direction (+1 or −1) to the count and timestamps the edge.
- **Falling edge:** stores the HIGH pulse width in a 16-entry ring buffer for the homing logic.
- Position (degrees) = `count × 18`.

### PD position control

Runs every 1 ms during `MOVING`:

```
error      = target − count
derivative = (error − previousError) / dt
output     = Kp × error + Kd × derivative
direction  = sign(output)
pwm        = clamp(|output|, 0, 255)
```

The move completes when `error == 0` or the error changes sign (overshoot). The motor then actively brakes for 120 ms and coasts to rest for 300 ms before the final position is checked. If the count is still off, a correction pass runs (maximum 3). If it is still off after that, the move ends with `Off target by N count(s)`.

### Homing

1. **Clear:** if the sensor starts HIGH (possibly inside the marker), drive forward until it goes LOW.
2. **Learn:** measure the first 3 normal HIGH pulses to establish the average width.
3. **Detect:** a pulse wider than `HOME_MARKER_RATIO × average` (and above `HOME_MIN_MARKER_US`) is the marker. Detection happens at its *trailing edge*, so it does not depend on speed.
4. **Brake and settle**, then set the encoder to **0** at the rest position.

Homing drives at a slow constant PWM (`HOME_PWM`) after a short stiction kick, and runs forward only.

---

## Configuration Reference

All constants are at the top of `OPQC.ino`.

| Constant | Default | Meaning |
|---|---|---|
| `AP_SSID` / `AP_PASS` | `OPQC_Motor` / `12345678` | Wi-Fi access point (password must be 8+ chars) |
| `COUNTS_PER_REV` | 20 | Encoder slots per revolution |
| `EDGE_DEBOUNCE_US` | 100 | Ignore edges closer than this (µs) |
| `Kp` | 20 | Proportional gain |
| `Kd` | 10.0 | Derivative gain |
| `MAX_PWM` | 255 | PWM ceiling |
| `MIN_PWM` | 0 | Minimum PWM when output is nonzero (0 = disabled) |
| `CONTROL_INTERVAL_US` | 1000 | PD loop period |
| `BRAKE_MS` | 120 | Active brake duration |
| `SETTLE_MS` | 300 | Wait for the rotor to stop before checking position |
| `STALL_MS` | 1500 | No encoder activity for this long while driving means stall |
| `MOVE_BASE_TIMEOUT` | 3000 | Base move timeout (ms) |
| `MOVE_PER_COUNT_MS` | 1000 | Extra timeout per count of travel |
| `MAX_RETRIES` | 3 | Correction passes after settling |
| `HOME_PWM` | 60 | Homing drive strength |
| `HOME_KICK_PWM` / `HOME_KICK_MS` | 150 / 40 | Initial kick to overcome stiction |
| `HOME_TIMEOUT_MS` | 20000 | Abort homing after this long |
| `HOME_MARKER_RATIO` | 2.0 | Marker must be this many times wider than the average pulse |
| `HOME_MIN_MARKER_US` | 2000 | Absolute minimum marker width (µs) |
| `HOME_BASELINE_PULSES` | 3 | Normal pulses needed before a marker can be judged |

In the web page script: `DIAL_SIGN = 1` (set to `-1` if the dial turns the wrong way).

---

## HTTP API

All endpoints use `GET`.

| Endpoint | Description | Responses |
|---|---|---|
| `/` | Dashboard page | 200 HTML |
| `/status` | Live state as JSON | 200 JSON |
| `/move?angle=<deg>` | Relative move (multiple of 18, non-zero) | 200 started, 400 missing arg, 409 rejected with reason |
| `/stop` | Brake and stop | 200 |
| `/home` | Start homing | 200 started, 409 busy |

Example `/status` response:

```json
{"count":10,"angle":180.0,"target":10,"status":"READY","busy":false,"homed":true,"msg":"Target reached"}
```

Example: `http://192.168.4.1/move?angle=-90`

---

## Serial Monitor Output

At 115200 baud you will see lines such as:

```
MOVE 90.0 deg | start=0 need=5 target=5
Target reached, count=5
HOMING: CLEARING REGION
HOMING: SEARCHING
HOME MARKER: 18250 us (normal avg 4120 us)
HOME OK: encoder = 0
Correction 1, error=-1
FAULT: STALL
```

The `HOME MARKER` line shows the marker width against the normal average, which is useful for tuning the ratio.

---

## Tuning Guide

**Homing never finds the marker or times out**
- Check the serial log. If no `HOME MARKER` line appears, the marker may be less than 2× the normal pulse width. Lower `HOME_MARKER_RATIO` (for example 1.5).
- Confirm the marker is HIGH-active on your sensor.

**`HOME STALL`**
- Raise `HOME_PWM` (try 70–90) or `HOME_KICK_PWM`.

**Motor does not finish the last count**
- Set `MIN_PWM` to about 40–60 so a 1-count error produces enough torque.

**Overshoot or oscillation**
- Lower `Kp`, or adjust `Kd`. Note that `Kd` acts on a count-quantized signal, so it behaves as a pulsed brake rather than a smooth derivative.

**Frequent `Off target` messages**
- Increase `SETTLE_MS`, or reduce overshoot with lower gains.

**Noisy or false counts**
- Increase `EDGE_DEBOUNCE_US`, add shielding to the encoder wires, or add a small capacitor at the sensor output.

---

## Troubleshooting

| Symptom | Likely cause / fix |
|---|---|
| Can't see the `OPQC_Motor` network | Check power, wait ~5 s after boot, confirm the serial monitor shows the AP IP |
| Page loads but status says `OFFLINE` | Wi-Fi dropped; rejoin `OPQC_Motor` |
| Buttons greyed out | System is busy; press STOP or wait for READY |
| `409 Angle must be a non-zero multiple of 18` | Use a value like 18, 36, 54, 90, 180 |
| Dial turns opposite to the motor | Set `DIAL_SIGN = -1`, or swap the motor leads |
| Count drifts after many moves | Single-channel limitation (see below); re-home periodically |
| Compile error on `analogWrite` | Update to the ESP32 Arduino core 3.x |

---

## Known Limitations

- **Single-channel encoder has no direction sensing.** Direction is assumed from the last motor command. If the rotor reverses while still coasting, counts can be misattributed. Re-homing resets any accumulated error. A quadrature (A/B) encoder is the proper fix.
- **Positioning resolution is 18°.** Angles that are not multiples of 18 are rejected.
- **Moves are relative.** Absolute positioning would need homing plus a "go to angle" layer built on top.
- **Homing direction is forward only.**
- **Wi-Fi is open to anyone with the password.** Change the defaults, as there is no authentication on the HTTP endpoints.
- **Control and web serving share one loop.** This is fine for normal use. The ISR keeps counting accurately regardless, but very heavy web traffic could add a few milliseconds of control jitter. If that happens, move the control loop to a pinned FreeRTOS task.

---
