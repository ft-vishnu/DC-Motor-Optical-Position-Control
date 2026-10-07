// OPQC - Phase 2
// Test 3 - Direction + PWM Control

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

    digitalWrite(STBY, HIGH);
}

void loop()
{
    // ---------- FORWARD - LOW SPEED ----------
    digitalWrite(AIN1, HIGH);
    digitalWrite(AIN2, LOW);
    analogWrite(PWMA, 80);

    delay(2000);

    // ---------- FORWARD - HIGH SPEED ----------
    analogWrite(PWMA, 200);

    delay(2000);

    // ---------- STOP ----------
    analogWrite(PWMA, 0);

    delay(1000);

    // ---------- REVERSE - LOW SPEED ----------
    digitalWrite(AIN1, LOW);
    digitalWrite(AIN2, HIGH);
    analogWrite(PWMA, 80);

    delay(2000);

    // ---------- REVERSE - HIGH SPEED ----------
    analogWrite(PWMA, 200);

    delay(2000);

    // ---------- STOP ----------
    analogWrite(PWMA, 0);

    delay(1000);
}