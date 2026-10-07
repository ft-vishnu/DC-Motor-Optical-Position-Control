

const int ENCODER_PIN = 4;

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
    Serial.println("OPQC - Build 3");
    Serial.println("Single-Channel Encoder Test");
    Serial.println("==============================");
    Serial.println("Count = 0");
}

void loop()
{
    static long lastCount = 0;

    if (encoderCount != lastCount)
    {
        Serial.print("Count = ");
        Serial.println(encoderCount);

        lastCount = encoderCount;
    }
}