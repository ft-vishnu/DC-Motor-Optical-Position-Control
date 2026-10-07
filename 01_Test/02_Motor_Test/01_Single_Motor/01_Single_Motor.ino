

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

    // Enable the TB6612
    digitalWrite(STBY, HIGH);
}

void loop()
{
    // ---------- FORWARD ----------
    digitalWrite(AIN1, HIGH);
    digitalWrite(AIN2, LOW);
    digitalWrite(PWMA, HIGH);

    delay(2000);

    // ---------- STOP ----------
    digitalWrite(PWMA, LOW);

    delay(1000);

    // ---------- REVERSE ----------
    digitalWrite(AIN1, LOW);
    digitalWrite(AIN2, HIGH);
    digitalWrite(PWMA, HIGH);

    delay(2000);

    // ---------- STOP ----------
    digitalWrite(PWMA, LOW);

    delay(1000);
}