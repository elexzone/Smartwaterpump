#include <Arduino.h>
#include <NewPing.h>

// Define sensor pins
#define TRIGGER_PIN 12
#define ECHO_PIN 14

// Maximum distance to measure (in centimeters)
#define MAX_DISTANCE 400

NewPing sonar(TRIGGER_PIN, ECHO_PIN, MAX_DISTANCE);


float waterlevel() {

  float distance = sonar.ping_cm(); // Distance in cm
  Serial.println(String("Water level: ") + distance + String (" cm"));
  return distance;
}


void setup() {

  Serial.begin(115200);
}

void loop() {
  float tanklevel = waterlevel();
  delay (500); 
}
