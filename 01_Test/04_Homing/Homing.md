# DC Motor Optical Position Control

A closed-loop position control system for a DC motor using optical encoder feedback.

The system uses an optical disk to measure motor rotation and a dedicated physical marker on the disk to establish a repeatable **HOME position**. The homing system was developed in two stages, progressing from detecting the complete marker to detecting it continuously and applying active braking.

---

## Overview

The optical disk normally produces alternating black and white regions:

```text
BLACK | WHITE | BLACK | WHITE | BLACK | WHITE | ...
```

A special home marker is created by covering three consecutive sections:

```text
BLACK | BLACK | BLACK | WHITE
```

This produces a continuous HIGH signal from the optical sensor that is significantly longer than the HIGH signal produced by a normal black section.

The system identifies the home marker by measuring the **duration of the sensor HIGH signal**.

### Encoder Resolution

The disk contains **20 slots per revolution**.

```text
360° / 20 = 18° per count
```

Therefore, the encoder provides a position resolution of:

**18° per count**

---

# Homing

Homing is used to establish a known physical reference position before position-control operations.

The optical sensor produces:

```text
White / Unblocked → LOW
Black / Blocked   → HIGH
```

The normal black sections produce a short HIGH duration, while the wider home marker produces a much longer HIGH duration.

### Measured Values

| Region               | HIGH Duration |
| -------------------- | ------------: |
| Normal black section |     ~13–16 ms |
| Home marker          |        ~46 ms |
| Detection threshold  |         30 ms |

The selected threshold clearly separates the normal encoder sections from the home marker:

```text
HIGH duration < 30 ms
        ↓
Normal black section

HIGH duration ≥ 30 ms
        ↓
Home marker
```

The three black sections of the marker are continuous, so the marker cannot be detected by counting three separate rising edges. Instead, the **continuous HIGH duration** is used.

---

# Homing Test 1 — Home Marker Detection

The first implementation was developed to verify the basic home-marker detection method.

When homing starts, the motor rotates continuously while the optical sensor is monitored.

When the sensor changes:

```text
LOW → HIGH
```

the beginning of the black region is recorded.

The system then waits for:

```text
HIGH → LOW
```

When the black region ends, its total HIGH duration is calculated.

```text
Normal black:

LOW ─────┐
         │<── 13–16 ms ──>
         └─────────────── LOW


Home marker:

LOW ─────┐
         │<──── ~46 ms ─────>
         └────────────────── LOW
```

The measured duration is compared with the **30 ms threshold**.

If the duration exceeds the threshold, the wider region is identified as the home marker and the motor is stopped.

### Test Parameters

```text
HOME_PWM       = 60
HOME_THRESHOLD = 30000 µs
```

```text
30,000 µs = 30 ms
```

### Detection Sequence

```text
Start Homing
     ↓
Motor rotates
     ↓
HIGH region detected
     ↓
Start timing
     ↓
HIGH region ends
     ↓
Calculate duration
     ↓
Compare with 30 ms
     ↓
Normal region ─────→ Continue scanning
     ↓
Home marker
     ↓
Stop motor
```

[View Homing Test 1 Code](./Home_Marker_Detection/Home_Marker_Detection.ino)

---

# Homing Test 2 — Continuous Home Marker Scan

The second implementation improves the detection process by checking the HIGH duration **while the black region is still active**.

Instead of waiting for:

```text
HIGH → LOW
```

the system continuously measures the HIGH duration.

When the duration reaches the threshold:

```text
30 ms
```

the home marker is immediately identified.

### Detection Sequence

```text
Start Homing
     ↓
Motor rotates
     ↓
HIGH region detected
     ↓
Start timing
     ↓
Continuously measure duration
     ↓
Has duration reached 30 ms?
     │
     ├── No → Continue scanning
     │
     └── Yes
          ↓
     Home marker detected
          ↓
      Active brake
          ↓
      Motor stopped
```

### Final Test Parameters

```text
HOME_PWM       = 49.5
HOME_THRESHOLD = 30000 µs
```

### Active Braking

After detecting the home marker, the motor is actively braked using the motor driver before being fully stopped.

This reduces the additional rotation that can occur from motor inertia after the home marker is detected.

[View Homing Test 2 Code](./Home_Final/Home_Final.ino)

---

# Development Progression

The two tests represent the development of the homing system:

```text
Homing Test 1
     │
     │ Detect complete HIGH region
     │
     ▼
Measure marker duration
     │
     ▼
Identify HOME
     │
     ▼
Homing Test 2
     │
     │ Continuously measure HIGH duration
     │
     ▼
Detect marker at threshold
     │
     ▼
Active braking
     │
     ▼
Improved stopping behavior
```

The fundamental detection principle remains the same:

> **The home position is identified by the longer optical HIGH duration produced by the dedicated home marker.**

---

# Hardware

* DC geared motor
* Optical encoder disk
* LM393IR-H19F4 optical sensor
* TB6612FNG motor driver
* Controller
* Motor power supply

---

# Connections

## Optical Sensor

| Sensor | Connection |
| ------ | ---------- |
| VCC    | 3.3 V      |
| GND    | GND        |
| DO     | GPIO 4     |
| AO     | Not used   |

Only the digital output is used for detecting the black and white regions of the disk.

## TB6612FNG

| TB6612FNG        | Connection   |
| ---------------- | ------------ |
| VCC              | 3.3 V        |
| GND              | GND          |
| STBY             | GPIO 18      |
| AIN1             | GPIO 15      |
| AIN2             | GPIO 16      |
| PWMA             | GPIO 17      |
| AO1              | Motor        |
| AO2              | Motor        |
| VM               | Motor supply |
| Motor supply GND | Common GND   |

---

# Encoder Disk

The encoder disk provides:

```text
20 slots / revolution
```

Therefore:

```text
360° / 20 = 18° per count
```

Normal disk pattern:

```text
BLACK | WHITE | BLACK | WHITE | BLACK | WHITE
```

Home-marker pattern:

```text
BLACK | BLACK | BLACK | WHITE
```

The home marker is intentionally wider than a normal black section so that it can be distinguished using time duration.

---

# Control Concept

The overall system is based on optical feedback:

```text
             ┌──────────────────────┐
             │                      │
             ▼                      │
        Optical Disk                │
             │                      │
             ▼                      │
       Optical Sensor               │
             │                      │
             ▼                      │
      Position Feedback             │
             │                      │
             ▼                      │
      Position Controller            │
             │                      │
             ▼                      │
       Motor Driver                 │
             │                      │
             ▼                      │
          DC Motor ─────────────────┘
```

Homing adds a physical reference to the system:

```text
Home Marker
     ↓
Physical Reference
     ↓
HOME Position
     ↓
Relative Position Control
```

---

# System Characteristics

| Parameter                |       Value |
| ------------------------ | ----------: |
| Encoder type             |     Optical |
| Encoder channels         |      Single |
| Disk slots               |          20 |
| Position resolution      | 18° / count |
| Normal black duration    |   ~13–16 ms |
| Home marker duration     |      ~46 ms |
| Home detection threshold |       30 ms |
| Homing Test 1 PWM        |          60 |
| Homing Test 2 PWM        |        49.5 |

---

# Limitations

The current encoder uses a **single optical sensor**.

Therefore, the encoder provides pulse-based position information, but it does not independently determine the direction of rotation. The motor direction is known from the commanded motor direction.

The position resolution is also limited to:

```text
18° per count
```

Therefore, the measured position is discrete rather than continuous.

The home marker establishes a physical reference, but its current geometry does not provide sub-count angular resolution within the marker.

---

# Result

The homing system successfully distinguishes the dedicated home marker from the normal optical encoder pattern using HIGH-duration measurement.

The development progressed from detecting the complete marker to continuously detecting the marker and applying active braking.

This provides a practical physical reference for the subsequent closed-loop DC motor position-control system.
