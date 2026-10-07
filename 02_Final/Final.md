# FINAL INTEGRATION  (DC-Motor-Optical-Position-Control)

Closed-loop angular position control of a DC motor using an optical encoder.

The final system combines the motor, optical encoder, TB6612FNG motor driver, homing system, PD position control and a Wi-Fi web interface into one working setup.

## Code

[DC-Motor-Optical-Position-Control.ino](./DC-Motor-Optical-Position-Control_V1/DC-Motor-Optical-Position-Control_V1.ino)

## Hardware

* ESP32-S3
* DC geared motor
* TB6612FNG motor driver
* LM393IR-H19F4 optical sensor
* 20-slot encoder disc
* Encoder disc with one wider home marker
* Motor power supply

## Connections

| Component         | ESP32-S3 |
| ----------------- | -------: |
| Optical sensor DO |   GPIO 4 |
| TB6612 AIN1       |  GPIO 15 |
| TB6612 AIN2       |  GPIO 16 |
| TB6612 PWMA       |  GPIO 17 |
| TB6612 STBY       |  GPIO 18 |

TB6612:

* AO1 and AO2 → motor
* VM → motor supply
* VCC → 3.3 V
* GND → common ground

The ESP32, sensor, motor driver and motor supply must share a common ground.

## Encoder

The encoder disc has 20 normal slots per revolution.

**20 counts/revolution = 18° per count**

The disc also contains one wider marker used for homing.

The optical sensor output is HIGH when the slot is detected. Encoder counts are generated from the sensor transitions.

Because this is a single-channel encoder, direction is determined from the motor command rather than from the encoder itself.

## Position Control

The motor position is controlled using encoder feedback.

For every move:

```text
Target position
      ↓
Encoder feedback
      ↓
Position error
      ↓
PD controller
      ↓
Motor direction + PWM
      ↓
Motor
      ↓
Encoder
```

The controller uses:

```text
error      = target - current
derivative = (error - previousError) / dt
output     = Kp × error + Kd × derivative
```

Current values:

* Kp = 20
* Kd = 10
* Maximum PWM = 255

The motor is actively braked after reaching the target and the final encoder position is checked before completing the move.

## Homing

The wider section on the encoder disc is used as the physical home reference.

When HOME is commanded:

1. The motor starts moving in the homing direction.
2. The encoder signal is monitored.
3. Normal slot widths are measured.
4. The wider marker is identified.
5. The motor is braked and allowed to settle.
6. The encoder position is set to zero.

After successful homing:

```text
Home position = 0 count
```

Homing should be performed before using the system for a known reference position.

## Web Interface

The ESP32 creates its own Wi-Fi network.

**SSID:** `OPQC_Motor`
**Password:** `12345678`

Connect to the network and open:

```text
http://192.168.4.1
```

The dashboard provides:

* Current encoder count
* Current angle
* Angle command
* Quick angle buttons
* HOME
* STOP
* System status
* Virtual position dial

The angle command is relative to the current position.

For example:

```text
+90°  → move 90° forward
-90°  → move 90° backward
```

Only multiples of 18° can be commanded because of the encoder resolution.

The virtual dial follows the actual encoder position.

## Main System States

The final firmware handles the main operating states:

* READY
* MOVING
* HOMING
* STOPPING
* STOPPED
* FAULT

STOP is available while the system is operating and immediately stops the current operation.

## Software Setup

1. Install Arduino IDE.
2. Install the ESP32 Arduino core.
3. Select the ESP32-S3 board.
4. Select the correct COM port.
5. Open `OPQC.ino`.
6. Upload the program.
7. Open Serial Monitor at **115200 baud**.
8. Connect to the `OPQC_Motor` Wi-Fi network.
9. Open `192.168.4.1`.
10. Run HOME before normal position commands.

No external libraries are required apart from the libraries included with the ESP32 Arduino core.

## Important Settings

The main configuration values are located near the beginning of `OPQC.ino`.

```cpp
COUNTS_PER_REV = 20

Kp = 20
Kd = 10.0

MAX_PWM = 255

HOME_PWM = 60
```

Other homing, braking, timeout and encoder settings can also be adjusted in the same section.

## Limitations

### Single-channel encoder

The encoder does not provide independent direction information. The firmware assumes the direction from the last motor command.

For more reliable bidirectional position feedback, a quadrature encoder should be used.

### Resolution

The current encoder provides:

```text
18° / count
```

Therefore the system cannot measure or command arbitrary angles between encoder counts.

### Relative commands

The web interface currently uses relative movement commands.

For example, entering `90` moves the motor 90° from its current position rather than moving it to an absolute 90° position.

### Homing

Homing currently operates in one direction and uses the physical marker on the encoder disc as the reference.

## Final Result

The final integration combines:

* Optical encoder feedback
* DC motor control
* TB6612FNG motor driver
* PD position control
* Active braking
* Physical homing
* Encoder position monitoring
* Wi-Fi control
* Web-based motor commands
* Live virtual position dial

The individual hardware and control stages were tested separately before combining them into this final system.
