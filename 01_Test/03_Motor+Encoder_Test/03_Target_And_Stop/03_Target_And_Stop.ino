// OPQC - Phase 3
// Test 3 - Relative Position Command
//
// Goal:
// Move the motor by a commanded angle relative to the
// current software position.
//
// IMPORTANT LIMITATIONS:
// 1. This project uses ONE optical sensor.
//    The encoder cannot independently determine direction.
//    Direction is inferred from the commanded motor direction.
//
// 2. Encoder resolution:
//    20 counts/revolution
//    360 / 20 = 18 degrees/count
//
// 3. Therefore the position resolution is 18 degrees.
//
//    Example:
//    90 degrees  = 5 counts  -> exact
//    180 degrees = 10 counts -> exact
//    45 degrees  = 2.5 counts -> cannot be exact
//
// 4. This is RELATIVE position control.
//    The ESP32 does not know the absolute physical shaft
//    position after power-up.
//
// 5. No homing sensor is used.
//
// 6. This test uses fixed motor speed.
//    P position control will be tested later.
//
// ------------------------------------------------------------

const int ENCODER_PIN = 4;

const int AIN1 = 15;
const int AIN2 = 16;
const int PWMA = 17;
const int STBY = 18;

const int COUNTS_PER_REV = 20;

const int MOTOR_SPEED = 100;

// Change this value to test different angles.
//
// Positive = Forward
// Negative = Reverse
//
// Examples:
// +90
// -90
// +180
// -180
//
float commandedAngle = 90.0;

volatile long encoderCount = 0;

void IRAM_ATTR encoderPulse()
{
    encoderCount++;
}

void setup()
{
    Serial.begin(115200);
    delay(1000);

    // ---------------- Encoder ----------------

    pinMode(ENCODER_PIN, INPUT_PULLUP);

    attachInterrupt(
        digitalPinToInterrupt(ENCODER_PIN),
        encoderPulse,
        RISING
    );

    // ---------------- Motor Driver ----------------

    pinMode(AIN1, OUTPUT);
    pinMode(AIN2, OUTPUT);
    pinMode(PWMA, OUTPUT);
    pinMode(STBY, OUTPUT);

    digitalWrite(STBY, HIGH);

    // -------------------------------------------
    // Convert angle to encoder counts
    // -------------------------------------------

    long requiredCounts =
        lround(commandedAngle * COUNTS_PER_REV / 360.0);

    long startPosition = encoderCount;

    long targetPosition =
        startPosition + requiredCounts;

    Serial.println("==============================");
    Serial.println("OPQC - Phase 3");
    Serial.println("Test 3 - Relative Position Command");
    Serial.println("==============================");

    Serial.print("Current Count = ");
    Serial.println(startPosition);

    Serial.print("Commanded Angle = ");
    Serial.print(commandedAngle);
    Serial.println(" degrees");

    Serial.print("Required Counts = ");
    Serial.println(requiredCounts);

    Serial.print("Target Count = ");
    Serial.println(targetPosition);

    // ---------------- Motor Direction ----------------

    if (requiredCounts > 0)
    {
        // Forward
        digitalWrite(AIN1, HIGH);
        digitalWrite(AIN2, LOW);

        analogWrite(PWMA, MOTOR_SPEED);
    }
    else if (requiredCounts < 0)
    {
        // Reverse
        digitalWrite(AIN1, LOW);
        digitalWrite(AIN2, HIGH);

        analogWrite(PWMA, MOTOR_SPEED);
    }
    else
    {
        // No movement required
        analogWrite(PWMA, 0);

        Serial.println("No movement required.");
        return;
    }

    // ---------------- Position Check ----------------

    while (true)
    {
        long currentPosition = encoderCount;

        if (requiredCounts > 0)
        {
            if (currentPosition >= targetPosition)
            {
                break;
            }
        }
        else
        {
            if (currentPosition <= targetPosition)
            {
                break;
            }
        }
    }

    // ---------------- Stop Motor ----------------

    analogWrite(PWMA, 0);

    // Give the motor driver a moment to stop.
    delay(50);

    Serial.println("------------------------------");
    Serial.println("Target reached");

    Serial.print("Final Count = ");
    Serial.println(encoderCount);

    float finalAngle =
        encoderCount * 360.0 / COUNTS_PER_REV;

    Serial.print("Final Angle = ");
    Serial.print(finalAngle);
    Serial.println(" degrees");

    Serial.println("------------------------------");
}

void loop()
{
    // Nothing required here.
}