

const int SENSOR_PIN = 4;

void setup()
{
    Serial.begin(115200);
    delay(1000);

    pinMode(SENSOR_PIN, INPUT_PULLUP);

    Serial.println("==============================");
    Serial.println("OPQC - Phase 1");
    Serial.println("Test 1 - Optical Sensor");
    Serial.println("==============================");
}

void loop()
{
    int sensorState = digitalRead(SENSOR_PIN);

    Serial.print("Sensor = ");
    Serial.println(sensorState);

    delay(200);
}