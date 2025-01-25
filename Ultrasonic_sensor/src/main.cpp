#include <Arduino.h>
#include <NewPing.h>

// Define sensor pins
#define TRIGGER_PIN 12
#define ECHO_PIN 14

#define MAX_DISTANCE 400 // 400cm
NewPing sonar(TRIGGER_PIN, ECHO_PIN, MAX_DISTANCE);

float waterlevel()
{

  float distance = sonar.ping_cm();
  // Serial.println(String("Water level: ") + distance + String (" cm"));
  return distance;
}

float tanklevel_check(float tankhight)
{

  float reading = waterlevel();
  float Tanklevel = tankhight - reading;
  Serial.println(String("Water level: ") + Tanklevel + String(" cm"));
  return Tanklevel;
}

void setup()
{
  Serial.begin(115200);
}

void loop()
{
  float tanklevel = tanklevel_check(50.00);
  delay(500);
}
