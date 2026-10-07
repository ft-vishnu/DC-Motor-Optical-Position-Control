

const int ENCODER_PIN = 4;

const int AIN1 = 15;
const int AIN2 = 16;
const int PWMA = 17;
const int STBY = 18;

const int HOME_PWM = 60;

// Based on measured values:
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

void stopMotor()
{
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

    stopMotor();

    Serial.println("==============================");
    Serial.println("OPQC - Homing Test 3");
    Serial.println("Home Marker Detection");
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
            Serial.println();
            Serial.println("Homing scan started...");
            Serial.println();

            encoderCount = 0;

            int previousState = digitalRead(ENCODER_PIN);

            bool blocked = false;

            unsigned long blockStartTime = 0;

            setMotor(1, HOME_PWM);

            while (true)
            {
                int currentState = digitalRead(ENCODER_PIN);

                // Start of blocked region
                if (currentState == HIGH &&
                    previousState == LOW)
                {
                    blocked = true;

                    blockStartTime = micros();
                }

                // End of blocked region
                if (currentState == LOW &&
                    previousState == HIGH)
                {
                    if (blocked)
                    {
                        blocked = false;

                        unsigned long blockDuration =
                            micros() - blockStartTime;

                        Serial.print("Blocked = ");
                        Serial.print(blockDuration);
                        Serial.println(" us");

                        // Home marker detected
                        if (blockDuration > HOME_THRESHOLD)
                        {
                            stopMotor();

                            Serial.println();
                            Serial.println("==============================");
                            Serial.println("HOME MARKER DETECTED");
                            Serial.println("==============================");
                            Serial.print("Duration = ");
                            Serial.print(blockDuration);
                            Serial.println(" us");

                            Serial.println();
                            Serial.println("Motor stopped.");
                            Serial.println("Press H to scan again.");

                            break;
                        }
                    }
                }

                previousState = currentState;
            }
        }
    }
}