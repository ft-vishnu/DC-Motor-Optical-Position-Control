

const int ENCODER_PIN = 4;

const int AIN1 = 15;
const int AIN2 = 16;
const int PWMA = 17;
const int STBY = 18;

const int HOME_PWM = 49.5;

// Measured at PWM = 60
// Normal slot  ≈ 13-16 ms
// Home marker  ≈ 46 ms
const unsigned long HOME_THRESHOLD = 30000;

volatile long encoderCount = 0;

void IRAM_ATTR encoderPulse()
{
    encoderCount++;
}

void setMotor(int direction, int pwm)
{
    if (direction > 0)
    {
        digitalWrite(AIN1, HIGH);
        digitalWrite(AIN2, LOW);
    }
    else
    {
        digitalWrite(AIN1, LOW);
        digitalWrite(AIN2, HIGH);
    }

    analogWrite(PWMA, pwm);
}

void brakeMotor()
{
    digitalWrite(AIN1, HIGH);
    digitalWrite(AIN2, HIGH);

    analogWrite(PWMA, 255);

    delay(100);

    analogWrite(PWMA, 0);

    digitalWrite(AIN1, LOW);
    digitalWrite(AIN2, LOW);
}

void setup()
{
    Serial.begin(115200);
    delay(1000);

    pinMode(ENCODER_PIN, INPUT_PULLUP);

    attachInterrupt(
        digitalPinToInterrupt(ENCODER_PIN),
        encoderPulse,
        RISING
    );

    pinMode(AIN1, OUTPUT);
    pinMode(AIN2, OUTPUT);
    pinMode(PWMA, OUTPUT);
    pinMode(STBY, OUTPUT);

    digitalWrite(STBY, HIGH);

    analogWrite(PWMA, 0);

    Serial.println("==============================");
    Serial.println("OPQC - Homing Test 9");
    Serial.println("Continuous Home Marker Scan");
    Serial.println("==============================");

    Serial.println();
    Serial.println("Press H to start.");
}

void loop()
{
    if (Serial.available())
    {
        char command = Serial.read();

        if (command == 'H' || command == 'h')
        {
            home();
        }
    }
}

void home()
{
    Serial.println();
    Serial.println("==============================");
    Serial.println("HOMING STARTED");
    Serial.println("==============================");

    encoderCount = 0;

    // Start continuous scan
    setMotor(1, HOME_PWM);

    int previousState = digitalRead(ENCODER_PIN);

    bool blocked = false;

    unsigned long blockStartTime = 0;

    while (true)
    {
        int currentState = digitalRead(ENCODER_PIN);

        // --------------------------------------------
        // Start of black region
        // --------------------------------------------

        if (!blocked &&
            currentState == HIGH &&
            previousState == LOW)
        {
            blocked = true;

            blockStartTime = micros();
        }

        // --------------------------------------------
        // Black region is currently active
        // --------------------------------------------

        if (blocked)
        {
            unsigned long duration =
                micros() - blockStartTime;

            // Home marker detected
            if (duration >= HOME_THRESHOLD)
            {
                brakeMotor();

                Serial.println();
                Serial.println("==============================");
                Serial.println("HOME MARKER DETECTED");
                Serial.println("==============================");

                Serial.print("Detection time = ");
                Serial.print(duration);
                Serial.println(" us");

                Serial.print("Encoder Count = ");
                Serial.println(encoderCount);

                Serial.println();
                Serial.println("Motor stopped.");
                Serial.println("HOME position found.");

                Serial.println();
                Serial.println("Press H to scan again.");

                break;
            }
        }

        // --------------------------------------------
        // End of black region
        // --------------------------------------------

        if (blocked &&
            currentState == LOW &&
            previousState == HIGH)
        {
            unsigned long duration =
                micros() - blockStartTime;

            Serial.print("Black = ");
            Serial.print(duration);
            Serial.println(" us");

            blocked = false;
        }

        previousState = currentState;
    }
}

