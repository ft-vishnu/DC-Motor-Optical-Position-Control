// OPQC - Phase 2
// Test 2 - TB6612FNG PWM Speed Control

const int AIN1 = 15;
const int AIN2 = 16;
const int PWMA = 17;
const int STBY = 18;

void setup()
{
    pinMode(AIN1, OUTPUT);
    pinMode(AIN2, OUTPUT);
    pinMode(PWMA, OUTPUT);
    pinMode(STBY, OUTPUT);

    // Enable motor driver
    digitalWrite(STBY, HIGH);

    // Forward direction
    digitalWrite(AIN1, HIGH);
    digitalWrite(AIN2, LOW);
}

void loop()
{
    // ---------- LOW SPEED ----------
    analogWrite(PWMA, 80);

    delay(2000);

    // ---------- MEDIUM SPEED ----------
    analogWrite(PWMA, 160);

    delay(2000);

    // ---------- HIGH SPEED ----------
    analogWrite(PWMA, 255);

    delay(2000);

    // ---------- STOP ----------
    analogWrite(PWMA, 0);

    delay(1000);
}