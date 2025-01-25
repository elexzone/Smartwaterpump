// #include <WiFi.h>
// #include "time.h"

// // WiFi credentials
// const char* ssid       = "Ravishka_Msi";
// const char* password   = "123456789";

// // NTP settings
// const char* ntpServer = "pool.ntp.org";
// const long  gmtOffset_sec = 19800;       // Sri Lanka UTC +5:30  (5hours×3600seconds/hour)+(30minutes×60seconds/minute)=19800seconds   
// const int   daylightOffset_sec = 0;     // No daylight saving time

// // Function to get local time as a String
// String LocalTime() {
//   struct tm timeinfo;
//   if (!getLocalTime(&timeinfo)) {
//     Serial.println("Failed to obtain time");
//     return "00:00:00"; // Return default time if unable to fetch
//   }

//   char timeStr[9]; // HH:MM:SS = 8 characters + 1 null terminator
//   strftime(timeStr, sizeof(timeStr), "%H:%M:%S", &timeinfo);
//   return String(timeStr); // Return the formatted time as a String
// }

// void setup() {
//   Serial.begin(115200);

//   // Connect to WiFi
//   Serial.printf("Connecting to %s ", ssid);
//   WiFi.begin(ssid, password);
//   while (WiFi.status() != WL_CONNECTED) {
//     delay(500);
//     Serial.print(".");
//   }
//   Serial.println(" CONNECTED");

//   // Initialize and configure time
//   configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
//   Serial.println("Time initialized!");

//   // Display initial time
//   Serial.println("Current Time: " + LocalTime());

// }

// void loop() {
//   String Peak_time = "";
//   String OffPeak_time = "";
//   String currentTime = LocalTime();
//   Serial.println("Current Time: " + currentTime);
//   delay(100); 

  
// }
#include <WiFi.h>
#include "time.h"

// WiFi credentials
const char* ssid       = "Ravishka_Msi";
const char* password   = "123456789";

// NTP settings
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 19800;       // Sri Lanka UTC +5:30 (5hours×3600seconds/hour)+(30minutes×60seconds/minute)=19800seconds   
const int   daylightOffset_sec = 0;     // No daylight saving time

// Time range for peak and off-peak hours
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

void setup() {
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
}

void loop() {
  String currentTime = LocalTime();
  Serial.println("Current Time: " + currentTime);

  // Get the current time as struct tm
  struct tm currentTimeStruct;
  if (getLocalTime(&currentTimeStruct)) {
    // Check if the current time is in peak hours
    if (isTimeInRange(currentTimeStruct, peakStart, peakEnd)) {
      Serial.println("Current time is in PEAK hours.");
    } 
    // Check if the current time is in off-peak hours
    else if (isTimeInRange(currentTimeStruct, offPeakStart, offPeakEnd)) {
      Serial.println("Current time is in OFF-PEAK hours.");
    } else {
      Serial.println("Current time is in REGULAR hours.");
    }
  }

  delay(1000); // Update every second
}
