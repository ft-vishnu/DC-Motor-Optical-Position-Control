# DC Motor Optical Position Control

This project is a DC motor position-control system using an optical encoder for position feedback.

The project is developed step by step. Each part is tested separately before moving to the next stage, and all the stages are finally combined into one working system.

## How It Works

The motor is driven by a TB6612FNG through the ESP32-S3. An optical sensor reads a 20-slot encoder disk attached to the motor and provides the position feedback.

The controller uses this feedback to move the motor to the required position.

The system also includes a physical home marker on the encoder disk for establishing a reference position, and a web interface for controlling and monitoring the motor.

The encoder resolution is:

**20 counts/revolution = 18°/count**

## Project Workflow

Follow the folders in this order:

### 1. Encoder Test

**Purpose:** Test the optical sensor and build the encoder position measurement.

* Optical sensor
* Pulse counting
* Count to angle

[Open Encoder Test](./01_Test/01_Encoder_Test/Encoder_Test.md)

---

### 2. Motor Test

**Purpose:** Test the DC motor and TB6612FNG independently.

* Motor direction
* PWM
* Direction and PWM control

[Open Motor Test](./01_Test/02_Motor_Test/Motor.md)

---

### 3. Motor + Encoder

**Purpose:** Combine the motor and encoder and develop position control.

* Motor with encoder feedback
* Position measurement
* Target position
* PD position control

[Open Motor + Encoder](./01_Test/03_Motor+Encoder_Test/Motor_Encoder.md)

---

### 4. Homing

**Purpose:** Add a physical reference position to the system.

* Home marker
* Home marker detection
* Continuous marker scanning
* Motor braking

[Open Homing](./01_Test/04_Homing/Homing.md)

---

### 5. Webserver

**Purpose:** Control and monitor the motor through a web interface.

* Current position
* Angle commands
* HOME
* STOP
* Virtual position dial

[Open Webserver](./01_Test/05_Web_Server/01_Webserver_Test/01_Webserver_Test.ino)

---

### 6. Final

All the tested parts are combined into the final system.

[Open Final](./02_Final/Final.md)

The Final folder contains its own README with the complete implementation and usage details.

## Hardware

* ESP32-S3 N16R8
* DC geared motor
* TB6612FNG motor driver
* LM393IR-H19F4 optical sensor
* 20-slot encoder disk
* Motor power supply




