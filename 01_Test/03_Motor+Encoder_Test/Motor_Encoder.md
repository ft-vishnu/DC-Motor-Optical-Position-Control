# Encoders + Motor

This section integrates the optical encoder and DC motor into a closed-loop rotary position-control system.

The objective is to use encoder feedback to measure motor rotation and control the motor to reach a commanded relative position.

The development was carried out progressively through four tests:

```text
Motor + Encoder
      ↓
Encoder Count
      ↓
Encoder Angle
      ↓
Initial Position Control
      ↓
PD Position Control
```

---

# Hardware

* ESP32-S3 N16R8
* TB6612FNG motor driver
* DC geared motor
* LM393IR-H19F4 optical sensor
* 20-slot optical encoder disk
* Motor power supply
* Jumper wires
* USB cable

---

# Connections

## Optical Encoder

| Optical Sensor | ESP32-S3      |
| -------------- | ------------- |
| VCC            | 3.3V          |
| GND            | GND           |
| DO             | GPIO 4        |
| AO             | Not connected |

The digital output `DO` is used for encoder pulse detection.

The ESP32 detects the rising edge of each encoder pulse using an interrupt.

---

## TB6612FNG

| TB6612FNG        | ESP32-S3       |
| ---------------- | -------------- |
| VCC              | 3.3V           |
| GND              | GND            |
| STBY             | GPIO 18        |
| AIN1             | GPIO 15        |
| AIN2             | GPIO 16        |
| PWMA             | GPIO 17        |
| AO1              | Motor wire 1   |
| AO2              | Motor wire 2   |
| VM               | Motor supply + |
| Motor supply GND | Common GND     |

The ESP32-S3 and motor power supply share a common ground.

---

# Test 1 — Motor + Encoder Count

## Objective

The first test combines the motor and optical encoder.

The motor is driven using the TB6612FNG while the optical encoder detects the rotation of the encoder disk.

The objective is to verify that encoder pulses can be detected while the motor is running.

## Working Principle

```text
ESP32-S3
   │
   ▼
TB6612FNG
   │
   ▼
DC Motor
   │
   ▼
Encoder Disk
   │
   ▼
Optical Sensor
   │
   ▼
ESP32 Interrupt
   │
   ▼
Encoder Count
```

Each detected rising edge increments the encoder count.

```cpp
void IRAM_ATTR encoderPulse()
{
    encoderCount++;
}
```

## Result

The motor was successfully operated while the ESP32 simultaneously counted encoder pulses.

Example encoder counts observed during operation were approximately:

```text
395
396
397
...
403
```

This verified that:

* The motor operates through the TB6612FNG.
* The optical sensor detects the encoder disk.
* Encoder pulses are detected by the ESP32.
* Motor operation and encoder measurement work together.

## Code

[View Test 1 Code](./Test_1_Motor_Encoder_Count/)

---

# Test 2 — Motor + Encoder Angle

## Objective

The second test converts the encoder count into angular position.

The encoder disk contains:

```text
20 slots / revolution
```

The experimentally verified relationship is:

```text
20 counts = 1 revolution
```

Since one revolution is:

```text
360°
```

the angular resolution is:

```text
360° / 20 = 18° per count
```

## Count-to-Angle Relationship

| Encoder Count | Angular Position |
| ------------: | ---------------: |
|             1 |              18° |
|             5 |              90° |
|            10 |             180° |
|            20 |             360° |
|            30 |             540° |
|            40 |             720° |

The encoder count is maintained as a cumulative value rather than being reset after every revolution.

For example:

```text
Count = 20 → 360°
Count = 40 → 720°
```

## Result

The motor and encoder successfully produced cumulative angular-position measurements.

Example measurements:

```text
Count = 36 → Angle = 648°
Count = 40 → Angle = 720°
Count = 41 → Angle = 738°
```

This verified the relationship:

```text
1 count = 18°
```

## Code

[View Test 2 Code](./Test_2_Motor_Encoder_Angle/)

---

# Test 3 — Position Control with Initial PWM

## Objective

The third test introduces position control.

Instead of simply running the motor and measuring its position, the system is given a target position and the motor is commanded to stop when the target encoder count is reached.

The initial controller uses a fixed PWM value.

## Control Principle

```text
Angle Command
      ↓
Required Encoder Counts
      ↓
Target Position
      ↓
Compare Current Position
      ↓
Target Reached?
   ↙          ↘
 YES           NO
  ↓             ↓
STOP       Fixed PWM
```

For example, a 90° command corresponds to:

```text
90° / 18°
= 5 counts
```

If the current position is 40 counts:

```text
Target = 40 + 5
       = 45 counts
```

The motor runs until the encoder reaches the target.

## Initial Result

The fixed-PWM controller successfully demonstrated basic position control.

However, significant overshoot was observed.

For example:

```text
Required movement = 5 counts
Target = 5
Final position = 8
```

Therefore:

```text
Overshoot = 8 - 5
          = 3 counts
```

Since:

```text
1 count = 18°
```

the overshoot corresponds to:

```text
3 × 18°
= 54°
```

## Observation

The fixed-PWM method does not reduce the motor effort as it approaches the target.

The motor can therefore still have considerable momentum when the target is reached.

This led to the introduction of proportional and derivative control.

## Code

[View Test 3 Code](./Test_3_Initial_PWM_Position_Control/)

---

# Test 4 — PD Position Control

## Objective

The fourth test improves the position controller by using a PD controller.

The controller combines:

```text
P — Proportional control
D — Derivative control
```

The final implementation uses both terms together in a single controller.

The code used for this test is the final position-control implementation.

## P Component — Proportional Control

The proportional component is based on the current position error.

```text
Error = Target Position - Current Position
```

The proportional output is:

```text
P output = Kp × Error
```

Therefore:

* Large position error → larger motor command
* Small position error → smaller motor command
* Zero position error → zero proportional command

This allows the motor to move quickly when it is far from the target and reduce its control effort as it approaches the target.

---

## D Component — Derivative Control

The derivative component considers how quickly the position error is changing.

```text
Derivative = Change in Error / Change in Time
```

The derivative output is:

```text
D output = Kd × Derivative
```

The purpose of the D component in this system is primarily **damping**.

When the motor is approaching the target rapidly, the derivative component reacts to that changing error and reduces excessive motion.

This helps address the inertia and overshoot observed with the initial fixed-PWM controller.

---

## Combined PD Controller

The final control output is:

```text
PD Output = P Output + D Output
```

or:

```text
Output = Kp × Error + Kd × Derivative
```

The complete control loop is therefore:

```text
Target Position
      ↓
Position Error
      ↓
 ┌─────────────┐
 │             │
 ▼             ▼
P Component   D Component
 │             │
 └──────┬──────┘
        ▼
   PD Output
        ↓
Direction + PWM
        ↓
TB6612FNG
        ↓
DC Motor
        ↓
Optical Encoder
        ↓
Current Position
        └──────────→ Feedback
```

---

# PD Controller Tuning

The controller was tuned experimentally.

## Proportional Gain

The proportional gain was adjusted first while keeping the derivative contribution at zero.

A value around:

```text
Kp = 20.5
```

provided good response for the system.

At this setting:

* 180° movement was achieved correctly.
* Larger movements exposed the effect of motor inertia.
* 360° movement showed noticeable overshoot.

This showed that the proportional controller provided the required position control but additional damping was beneficial.

## Derivative Gain

The derivative gain was then introduced while keeping the proportional gain approximately fixed.

The D term was adjusted experimentally to reduce the overshoot observed during larger movements.

The objective was not simply to maximize the D gain, but to obtain a good balance between:

```text
Fast response
+
Low overshoot
+
Stable settling
```

The final selected values were verified experimentally with different movement commands.

---

# Position-Control Verification

The final PD controller was tested using multiple relative angle commands.

## 90°

```text
90° = 5 encoder counts
```

The motor reached the required encoder position.

Result:

```text
PASS
```

---

## 180°

```text
180° = 10 encoder counts
```

The motor successfully reached the required target position.

Example:

```text
Start = 242
Target = 252
Final Count = 252
```

Result:

```text
PASS
```

---

## 360°

```text
360° = 20 encoder counts
```

The motor successfully performed the larger movement.

The 360° test was particularly useful for tuning because the larger movement produced more noticeable motor inertia and overshoot.

After PD tuning, the response was considered satisfactory for the intended working prototype.

Result:

```text
PASS
```

---

# Final Controller Code

The final position-control implementation contains the complete PD controller.

No separate P-controller or D-controller code is used.

The P and D components are implemented together in the same control loop.

## Code

[View Final PD Position Control Code](./Test_4_PD_Position_Control/)

---

# Encoder Resolution

The current encoder provides:

```text
20 counts / revolution
```

Therefore:

```text
1 count = 18°
```

This limits the angular resolution of the system.

For example, a command of:

```text
100°
```

corresponds to:

```text
100 / 18 = 5.56 counts
```

The controller must therefore convert this into an integer encoder count.

If rounded to 6 counts:

```text
6 × 18° = 108°
```

Therefore, even when the controller reaches the correct encoder count, the actual commanded physical angle can differ because of encoder quantization.

Other sources of physical positioning error include:

* Motor inertia
* Gearbox backlash
* Mechanical play
* Encoder disk alignment
* Sensor mounting
* Motor characteristics
* Load on the motor

The system is therefore intended as a **working prototype**, rather than a high-precision positioning system.

---

# Single-Channel Encoder Limitation

The current system uses one optical sensor.

Therefore, it does not provide true quadrature direction detection.

The sensor provides information about:

```text
How much the encoder disk moved
```

but cannot independently determine:

```text
Clockwise or counter-clockwise
```

In the current controlled system, the software uses the commanded motor direction to determine the sign of the encoder position change.

This is suitable for controlled motor movement because the shaft movement is generated by the controller itself.

However, the system cannot independently determine arbitrary manual shaft movement in both directions.

---

# Relative Positioning

The current system operates using relative position commands.

The commanded angle is applied relative to the current encoder position.

For example:

```text
Current position = 40 counts

Command = 90°
90° = 5 counts

New target = 45 counts
```

This allows consecutive relative movements.

The system does not determine an absolute physical shaft angle after power-up.

Without a homing mechanism or absolute position sensor, the ESP32 does not know the physical zero position of the motor after startup.

---

# Final Verified Parameters

| Parameter         |                  Value |
| ----------------- | ---------------------: |
| Controller        |                     PD |
| MCU               |         ESP32-S3 N16R8 |
| Motor driver      |              TB6612FNG |
| Motor             |        DC geared motor |
| Encoder           | Single-channel optical |
| Encoder disk      |               20 slots |
| Counts/revolution |                     20 |
| Angle/count       |                    18° |
| Encoder GPIO      |                 GPIO 4 |
| AIN1              |                GPIO 15 |
| AIN2              |                GPIO 16 |
| PWMA              |                GPIO 17 |
| STBY              |                GPIO 18 |
| PWM range         |                  0–255 |
| Position type     |               Relative |
| Feedback          |          Encoder count |
| Interface         |         Serial Monitor |
| Homing            |        Not implemented |
| Absolute position |          Not available |
| Quadrature        |               Not used |

---

# Result

The four tests successfully developed the system from basic motor and encoder integration to closed-loop position control.

```text
Test 1
Motor + Encoder Count
        ↓
Test 2
Motor + Encoder Angle
        ↓
Test 3
Initial PWM Position Control
        ↓
Test 4
PD Position Control
```

The final system can:

* Drive the DC motor using the TB6612FNG.
* Detect optical encoder pulses.
* Measure cumulative encoder position.
* Convert encoder counts into angular position.
* Calculate a relative target position.
* Control motor output according to position error.
* Use derivative action for improved damping.
* Reach commanded encoder positions for tested movements.
* Perform 90°, 180°, and 360° relative movements successfully.


