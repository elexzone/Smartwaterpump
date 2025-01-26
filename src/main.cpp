#include <Arduino.h>
#include <NewPing.h>
#include <WiFi.h>
#include "time.h"

// WiFi credentials
const char* ssid       = "Ravishka_Msi";
const char* password   = "123456789";

// NTP settings
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 19800;       // Sri Lanka UTC +5:30 (5hours×3600seconds/hour)+(30minutes×60seconds/minute)=19800seconds   
const int   daylightOffset_sec = 0;     // No daylight saving time 

#define TRIGGER_PIN 12 // Ultrasonic
#define ECHO_PIN 14    // Ultrasonic
#define Pump 2         // Pump  pin 2
#define filtervalve 5  //  filtervalve  pin 5
#define BUTTON_PIN 23  // Button pin 3 (interrupt pin)

#define MAX_DISTANCE 400 // 400cm
NewPing sonar(TRIGGER_PIN, ECHO_PIN, MAX_DISTANCE);
volatile bool PumpState = false; // To track the Pump state


// Time range for peak and off-peak hours at CEB 
struct tm peakStart = {0, 30, 18};  // 6:30 PM
struct tm peakEnd = {0, 30, 22};    // 10:30 PM
struct tm offPeakStart = {0, 30, 22}; // 10:30 PM
struct tm offPeakEnd = {0, 30, 5};  // 5:30 AM

// Function to get local time as a String
String LocalTime() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return "00:00:00"; // Return default time if unable to fetch
  }

  char timeStr[9]; // HH:MM:SS = 8 characters + 1 null terminator
  strftime(timeStr, sizeof(timeStr), "%H:%M:%S", &timeinfo);
  return String(timeStr); // Return the formatted time as a String
}

// Function to compare two times
bool isTimeInRange(struct tm currentTime, struct tm startTime, struct tm endTime) {
  int currentMinutes = currentTime.tm_hour * 60 + currentTime.tm_min;
  int startMinutes = startTime.tm_hour * 60 + startTime.tm_min;
  int endMinutes = endTime.tm_hour * 60 + endTime.tm_min;

  // Handle cases where end time is past midnight
  if (endMinutes < startMinutes) {
    if (currentMinutes >= startMinutes || currentMinutes <= endMinutes) {
      return true;
    }
  } else {
    if (currentMinutes >= startMinutes && currentMinutes <= endMinutes) {
      return true;
    }
  }
  return false;
}

float waterlevel()
{

  float distance = sonar.ping_cm();
  // Serial.println(String("Water level: ") + distance + String (" cm"));
  return distance;
}

String tanklevel_check(float tankhight)
{
  float reading = waterlevel();
  float Tanklevel = tankhight + 3.00 - reading;

  String level = "";
  if (Tanklevel < tankhight)
  {
    level = "LOW";
  }
  else
  {
    level = "HIGH";
  }
  Serial.println(String("Water level: ") + level);
  return level;
}



void togglePump()
{
  PumpState = !PumpState;        // Toggle Pump state
  digitalWrite(Pump, PumpState); // Update Pump
}

String HourStatus() {
  String status = ""; 
  struct tm currentTimeStruct;

  if (getLocalTime(&currentTimeStruct)) {
   
    if (isTimeInRange(currentTimeStruct, peakStart, peakEnd)) {
      Serial.println("PEAK");
      status = "PEAK";
    } else if (isTimeInRange(currentTimeStruct, offPeakStart, offPeakEnd)) {
      Serial.println("OFF-PEAK");
      status = "OFF-PEAK";
    } else {
      Serial.println("REGULAR");
      status = "REGULAR";
    }
  } else {
    Serial.println("Failed to get local time!");
    status = "UNKNOWN";
  }
  Serial.println("Hour Status: " + status);
  return status; 
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void setup()
{

  Serial.begin(115200);

   // Connect to WiFi
  Serial.printf("Connecting to %s ", ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" CONNECTED");

  // Initialize and configure time
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  Serial.println("Time initialized!");


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
  String tanklevel = tanklevel_check(40.00);
  String Hours = HourStatus(); 

  if (tanklevel == "LOW")
  {
    digitalWrite(Pump, HIGH);
  }
  else if (tanklevel == "HIGH")
  {
    digitalWrite(Pump, LOW);
  }

  delay(500);
}
