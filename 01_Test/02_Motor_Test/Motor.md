# Motor Control

This phase develops and verifies basic DC motor control using the **TB6612FNG motor driver** and ESP32-S3.

The goal is:

**ESP32-S3 → TB6612FNG → DC Motor**

---

## Hardware

* ESP32-S3 N16R8
* TB6612FNG motor driver
* DC geared motor
* Motor power supply
* Jumper wires

---

## TB6612FNG Connections

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
| Motor supply GND | GND            |

The ESP32-S3 and motor power supply share a common ground.

---

# Test 1 — Single Motor Direction Control

### Objective

Verify basic motor control through the TB6612FNG.

The motor is commanded to:

**Forward → Stop → Reverse → Stop**

### Code

[View Test 1 Code](./01_Single_Motor/01_Single_Motor.ino)

---

# Test 2 — PWM Speed Control

### Objective

Verify variable motor speed control using PWM.

The motor is tested at different PWM values:

```text
80  → Low speed
160 → Medium speed
255 → Full speed
```

### Why is the PWM value 0–255?

The PWM value used in this test is an **8-bit value**.

An 8-bit number has:

$$
2^8 = 256
$$

possible values:

```text
0, 1, 2, ... 254, 255
```

Therefore:

```text
0   → 0% duty cycle → OFF
255 → 100% duty cycle → Full output
```

The duty cycle can be calculated as:

$$
Duty\ Cycle = \frac{PWM\ Value}{255}\times100
$$

For example:

| PWM Value | Approx. Duty Cycle |
| --------: | -----------------: |
|         0 |                 0% |
|        80 |                31% |
|       160 |                63% |
|       200 |                78% |
|       255 |               100% |

The PWM value controls the **average power delivered to the motor**, allowing its speed to be varied.

### Code

[View Test 2 Code](./02_PWM/02_PWM.ino)

---

# Test 3 — Direction + PWM Control

### Objective

Verify that motor direction and variable speed can be controlled together.

The motor is commanded to:

```text
Forward → Low Speed
Forward → High Speed
Stop
Reverse → Low Speed
Reverse → High Speed
Stop
```

### Code

[View Test 3 Code](./03_Direction_PWM/03_Direction_PWM.ino)
