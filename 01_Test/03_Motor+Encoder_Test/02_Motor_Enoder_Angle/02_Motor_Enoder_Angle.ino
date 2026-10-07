

const int ENCODER_PIN = 4;

const int AIN1 = 15;
const int AIN2 = 16;
const int PWMA = 17;
const int STBY = 18;

const int COUNTS_PER_REV = 20;

volatile long encoderCount = 0;

void IRAM_ATTR encoderPulse()
{
    encoderCount++;
}

void setup()
{
    Serial.begin(115200);
    delay(1000);

    // Encoder
    pinMode(ENCODER_PIN, INPUT_PULLUP);

    attachInterrupt(
        digitalPinToInterrupt(ENCODER_PIN),
        encoderPulse,
        RISING
    );

    // Motor driver
    pinMode(AIN1, OUTPUT);
    pinMode(AIN2, OUTPUT);
    pinMode(PWMA, OUTPUT);
    pinMode(STBY, OUTPUT);

    digitalWrite(STBY, HIGH);

    // Forward
    digitalWrite(AIN1, HIGH);
    digitalWrite(AIN2, LOW);

    // Start motor
    analogWrite(PWMA, 100);

    Serial.println("==============================");
    Serial.println("OPQC - Phase 3");
    Serial.println("Test 2 - Motor + Encoder Angle");
    Serial.println("==============================");
}

void loop()
{
    static long lastCount = -1;

    if (encoderCount != lastCount)
    {
        float angle =
            (encoderCount * 360.0) / COUNTS_PER_REV;

        Serial.print("Count = ");
        Serial.print(encoderCount);

        Serial.print(" | Angle = ");
        Serial.print(angle);

        Serial.println(" degrees");

        lastCount = encoderCount;
    }
}