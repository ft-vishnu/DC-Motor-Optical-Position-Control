

const int ENCODER_PIN = 4;

const int AIN1 = 15;
const int AIN2 = 16;
const int PWMA = 17;
const int STBY = 18;

const int COUNTS_PER_REV = 20;
const int MAX_PWM = 255;

float Kp = 20;
float Kd = 10.0;

volatile long encoderCount = 0;
volatile int motorDirection = 1;

float previousError = 0;

unsigned long previousTime;


void IRAM_ATTR encoderPulse()
{
    encoderCount += motorDirection;
}


void setMotor(int direction, int pwm)
{
    motorDirection = direction;

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

    analogWrite(PWMA, constrain(pwm, 0, MAX_PWM));
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
    Serial.println("OPQC - Phase 3");
    Serial.println("Test 4 - PD Position Control");
    Serial.println("==============================");

    Serial.print("Kp = ");
    Serial.println(Kp);

    Serial.print("Kd = ");
    Serial.println(Kd);

    Serial.println();
    Serial.println("Enter angle:");
}


void loop()
{
    if (Serial.available())
    {
        float commandedAngle = Serial.parseFloat();

        while (Serial.available())
        {
            Serial.read();
        }

        long startPosition = encoderCount;

        long requiredCounts =
            lround(
                commandedAngle *
                COUNTS_PER_REV /
                360.0
            );

        long targetPosition =
            startPosition + requiredCounts;

        float previousError = 0;

        unsigned long previousTime = micros();


        Serial.println();

        Serial.print("Commanded Angle = ");
        Serial.print(commandedAngle);
        Serial.println(" degrees");

        Serial.print("Start = ");
        Serial.println(startPosition);

        Serial.print("Required Counts = ");
        Serial.println(requiredCounts);

        Serial.print("Target = ");
        Serial.println(targetPosition);

        Serial.println();


        while (true)
        {
            long currentPosition = encoderCount;

            float error =
                targetPosition - currentPosition;


            if (error == 0)
            {
                stopMotor();

                Serial.println("Target reached");

                Serial.print("Final Count = ");
                Serial.println(encoderCount);

                Serial.println();
                Serial.println("Enter angle:");

                break;
            }


            unsigned long currentTime = micros();

            float dt =
                (currentTime - previousTime) /
                1000000.0;

            previousTime = currentTime;

            if (dt <= 0)
                continue;


            float derivative =
                (error - previousError) / dt;

            float output =
                Kp * error +
                Kd * derivative;

            previousError = error;


            int direction =
                output > 0 ? 1 : -1;

            int pwm =
                constrain(
                    abs((int)output),
                    0,
                    MAX_PWM
                );


            setMotor(direction, pwm);
        }
    }
}