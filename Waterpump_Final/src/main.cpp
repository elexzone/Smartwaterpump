#include <Arduino.h>

#define Pump 2 // Pump  pin 2
#define filtervalve 5 //  filtervalve  pin 5
#define BUTTON_PIN 23 // Button pin 3 (interrupt pin)

volatile bool PumpState = false; // To track the Pump state

void togglePump()
{
  PumpState = !PumpState;        // Toggle Pump state
  digitalWrite(Pump, PumpState); // Update Pump
}

void setup()
{
  pinMode(Pump, OUTPUT);
  pinMode(filtervalve, OUTPUT);

  pinMode(BUTTON_PIN, INPUT_PULLUP);                                       // Set button pin as input with internal pull-up
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), togglePump, FALLING); // Attach interrupt on button press

  // Init
  digitalWrite(Pump, LOW);
  digitalWrite(filtervalve, LOW);
}

void loop()
{

}
