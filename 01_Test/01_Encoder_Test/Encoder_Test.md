# Single Optical Encoder

This phase develops and verifies the **single-channel optical encoder** used for rotary position measurement.

The goal is:

**Optical Sensor → Encoder Pulse → Count → Angular Position**

---

## Hardware

* ESP32-S3 N16R8
* LM393IR-H19F4 optical sensor
* 20-slot optical encoder disk
* Jumper wires

---

## Sensor Connections

| Optical Sensor | ESP32-S3      |
| -------------- | ------------- |
| VCC            | 3V3           |
| GND            | GND           |
| DO             | GPIO 4        |
| AO             | Not connected |

---

# Test 1 — Optical Sensor

### Objective

Verify that the optical sensor produces a digital signal when the encoder disk blocks and unblocks the optical path.

### Result

| Disk Condition | Sensor Output |
| -------------- | ------------: |
| Unblocked      |           `0` |
| Blocked        |           `1` |

### Code

[View Test 1 Code](./Test_1_Optical_Sensor/)

---

# Test 2 — Encoder Pulse Counting

### Objective

Detect the optical sensor's rising edges and convert them into encoder counts using an ESP32 interrupt.

### Result

The ESP32-S3 successfully detects and counts encoder pulses while the disk is rotated.

### Code

[View Test 2 Code](./Test_2_Pulse_Counting/)

---

# Test 3 — Encoder Count to Angle

### Objective

Convert the encoder count into angular position.

The 20-slot disk was experimentally measured to produce:

```text
20 counts = 1 revolution
```

Therefore:

```text
360° / 20 = 18° per count
```

### Angle Conversion

```text
1 count  = 18°
5 counts = 90°
10 counts = 180°
20 counts = 360°
```

The encoder count remains **cumulative** rather than wrapping at 360°.

### Code

[View Test 3 Code](./Test_3_Count_to_Angle/)

---

## Verified Parameters

| Parameter         |                  Value |
| ----------------- | ---------------------: |
| Encoder type      | Single-channel optical |
| Disk slots        |                     20 |
| Counts/revolution |                     20 |
| Angle/count       |                    18° |
| Sensor GPIO       |                 GPIO 4 |
| Detection         |            Rising edge |
| Position          |       Cumulative count |
