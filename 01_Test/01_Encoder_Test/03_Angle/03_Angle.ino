

const int ENCODER_PIN = 4;

// Change this after measuring your disk
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

    pinMode(ENCODER_PIN, INPUT_PULLUP);

    attachInterrupt(
        digitalPinToInterrupt(ENCODER_PIN),
        encoderPulse,
        RISING
    );

    Serial.println("==============================");
    Serial.println("OPQC - Phase 1");
    Serial.println("Test 3 - Count to Angle");
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