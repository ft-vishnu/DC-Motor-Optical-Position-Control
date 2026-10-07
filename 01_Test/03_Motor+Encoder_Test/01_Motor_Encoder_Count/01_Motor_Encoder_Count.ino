// OPQC - Phase 3
// Test 1 - Motor + Encoder Count

const int ENCODER_PIN = 4;

const int AIN1 = 15;
const int AIN2 = 16;
const int PWMA = 17;
const int STBY = 18;

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

    // Enable TB6612FNG
    digitalWrite(STBY, HIGH);

    // Forward direction
    digitalWrite(AIN1, HIGH);
    digitalWrite(AIN2, LOW);

    // Start motor
    analogWrite(PWMA, 100);

    Serial.println("==============================");
    Serial.println("OPQC - Phase 3");
    Serial.println("Test 1 - Motor + Encoder Count");
    Serial.println("==============================");
    Serial.println("Motor running...");
}

void loop()
{
    static long lastCount = 0;

    if (encoderCount != lastCount)
    {
        Serial.print("Encoder Count = ");
        Serial.println(encoderCount);

        lastCount = encoderCount;
    }
}