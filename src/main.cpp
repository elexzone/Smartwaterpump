#include <Arduino.h>
#include <NewPing.h>
#include <WiFi.h>
#include "time.h"

// Provide the token generation process info.
#include "addons/TokenHelper.h"
// Provide the RTDB payload printing info and other helper functions.
#include "addons/RTDBHelper.h"

// Wi-Fi and Firebase credentials
#define WIFI_SSID "Ravishka_Msi"
#define WIFI_PASSWORD "123456789"
#define API_KEY "AIzaSyCRIWwPiypjo5ukFPfwUAszsIcpa1_xFkg"
#define DATABASE_URL "https://smart-water-pump-8fa44-default-rtdb.firebaseio.com"

// Firebase objects
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

unsigned long lastSendMillis = 0;

// Timers for separate calls
unsigned long floatDataTimer = 0;
unsigned long stringDataTimer = 0;
const unsigned long interval = 5000; // 5 seconds between each type of data send

bool signupOK = false;


// WiFi credentials
const char *ssid = "Ravishka_Msi";
const char *password = "123456789";

// NTP settings
const char *ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 19800; // Sri Lanka UTC +5:30 (5hours×3600seconds/hour)+(30minutes×60seconds/minute)=19800seconds
const int daylightOffset_sec = 0; // No daylight saving time

#define TdsSensorPin 35
#define TRIGGER_PIN 12 // Ultrasonic
#define ECHO_PIN 14    // Ultrasonic
#define Pump 2         // Pump  pin 2
#define filtervalve 5  //  filtervalve  pin 5
#define BUTTON_PIN 23  // Button pin 3 (interrupt pin)
// TDS meter
#define VREF 3.3  // Analog reference voltage (Volt) of the ADC
#define SCOUNT 30 // Number of sample points

#define MAX_DISTANCE 400 // 400cm
NewPing sonar(TRIGGER_PIN, ECHO_PIN, MAX_DISTANCE);
volatile bool PumpState = false; // To track the Pump state
float Tankhight = 40.00;

int analogBuffer[SCOUNT]; // Store the analog value in the array, read from ADC
int analogBufferTemp[SCOUNT];
int analogBufferIndex = 0;
float averageVoltage = 0;
float tdsValue = 0;
float temperature = 25; // Current temperature for compensation

// Time range for peak and off-peak hours in CEB Srilanka
struct tm peakStart = {0, 30, 18}; // 6:30 PM
struct tm peakEnd = {0, 30, 22};   // 10:30 PM

struct tm offPeakStart = {0, 30, 22}; // 10:30 PM
struct tm offPeakEnd = {0, 30, 5};    // 5:30 AM

// Function to get local time as a String
String LocalTime()
{
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo))
  {
    Serial.println("Failed to obtain time");
    return "00:00:00"; // Return default time if unable to fetch
  }

  char timeStr[9]; // HH:MM:SS = 8 characters + 1 null terminator
  strftime(timeStr, sizeof(timeStr), "%H:%M:%S", &timeinfo);
  return String(timeStr); // Return the formatted time as a String
}

// Function to compare two times
bool isTimeInRange(struct tm currentTime, struct tm startTime, struct tm endTime)
{
  int currentMinutes = currentTime.tm_hour * 60 + currentTime.tm_min;
  int startMinutes = startTime.tm_hour * 60 + startTime.tm_min;
  int endMinutes = endTime.tm_hour * 60 + endTime.tm_min;

  // Handle cases where end time is past midnight
  if (endMinutes < startMinutes)
  {
    if (currentMinutes >= startMinutes || currentMinutes <= endMinutes)
    {
      return true;
    }
  }
  else
  {
    if (currentMinutes >= startMinutes && currentMinutes <= endMinutes)
    {
      return true;
    }
  }
  return false;
}

// Median filtering algorithm
int getMedianNum(int bArray[], int iFilterLen)
{
  int bTab[iFilterLen];
  for (byte i = 0; i < iFilterLen; i++)
    bTab[i] = bArray[i];

  int i, j, bTemp;
  for (j = 0; j < iFilterLen - 1; j++)
  {
    for (i = 0; i < iFilterLen - j - 1; i++)
    {
      if (bTab[i] > bTab[i + 1])
      {
        bTemp = bTab[i];
        bTab[i] = bTab[i + 1];
        bTab[i + 1] = bTemp;
      }
    }
  }
  if ((iFilterLen & 1) > 0)
  {
    bTemp = bTab[(iFilterLen - 1) / 2];
  }
  else
  {
    bTemp = (bTab[iFilterLen / 2] + bTab[iFilterLen / 2 - 1]) / 2;
  }
  return bTemp;
}

float TDSreading(float temperature)
{
  static unsigned long analogSampleTimepoint = millis();
  static unsigned long printTimepoint = millis();

  if (millis() - analogSampleTimepoint > 40U)
  { // Every 40 milliseconds
    analogSampleTimepoint = millis();
    analogBuffer[analogBufferIndex] = analogRead(TdsSensorPin); // Read analog value
    analogBufferIndex++;
    if (analogBufferIndex == SCOUNT)
    {
      analogBufferIndex = 0;
    }
  }

  if (millis() - printTimepoint > 800U)
  { // Every 800 milliseconds
    printTimepoint = millis();
    for (int copyIndex = 0; copyIndex < SCOUNT; copyIndex++)
    {
      analogBufferTemp[copyIndex] = analogBuffer[copyIndex];
    }

    // Median filtering and voltage calculation
    float averageVoltage = getMedianNum(analogBufferTemp, SCOUNT) * (float)VREF / 4096.0;

    // Temperature compensation
    float compensationCoefficient = 1.0 + 0.02 * (temperature - 25.0);
    float compensationVoltage = averageVoltage / compensationCoefficient;

    // Convert voltage to TDS value
    float tdsValue = (133.42 * compensationVoltage * compensationVoltage * compensationVoltage - 255.86 * compensationVoltage * compensationVoltage + 857.39 * compensationVoltage) * 0.5;

    return tdsValue; // Return the calculated TDS value
  }

  return -1; // Return -1 if no new TDS value is ready
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

String HourStatus()
{
  String status = "";
  struct tm currentTimeStruct;

  if (getLocalTime(&currentTimeStruct))
  {

    if (isTimeInRange(currentTimeStruct, peakStart, peakEnd))
    {
      Serial.println("PEAK");
      status = "PEAK";
    }
    else if (isTimeInRange(currentTimeStruct, offPeakStart, offPeakEnd))
    {
      Serial.println("OFF-PEAK");
      status = "OFF-PEAK";
    }
    else
    {
      Serial.println("REGULAR");
      status = "REGULAR";
    }
  }
  else
  {
    Serial.println("Failed to get local time!");
    status = "UNKNOWN";
  }
  Serial.println("Hour Status: " + status);
  return status;
}
// Send a float to Firebase
void send_Sensordata(float floatdata, String floatpath) {
  if (Firebase.ready() && signupOK) {
    if (Firebase.RTDB.setFloat(&fbdo, floatpath, floatdata)) {
      Serial.println("PASSED");
      Serial.println(floatdata);
      Serial.println("PATH: " + fbdo.dataPath());
      Serial.println("TYPE: " + fbdo.dataType());
    } else {
      Serial.println("FAILED");
      Serial.println("REASON: " + fbdo.errorReason());
    }
  }
}

// Send a string to Firebase
void send_Message(String message, String databasePath) {
  if (Firebase.ready() && signupOK) {
    if (Firebase.RTDB.setString(&fbdo, databasePath.c_str(), message)) {
      Serial.println("PASSED");
      Serial.println(message);
      Serial.println("PATH: " + fbdo.dataPath());
      Serial.println("TYPE: " + fbdo.dataType());
    } else {
      Serial.println("FAILED");
      Serial.println("REASON: " + fbdo.errorReason());
    }
  }
}

String readString(String path) {
  String receivedString = ""; // Initialize an empty string to hold the result
  // Read data from the Firebase path
  if (Firebase.RTDB.getString(&fbdo, path)) {
    if (fbdo.dataType() == "string") {
      receivedString = fbdo.stringData();
      Serial.println("Received string data: " + receivedString);
    } else {
      Serial.println("Data is not a string!");
    }
  } else {
    Serial.println("Failed to read string data. Reason: " + fbdo.errorReason());
  }
  return receivedString; // Return the string data
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void setup()
{

  Serial.begin(115200);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  // Assign the api key
  config.api_key = API_KEY;

  // Assign the RTDB URL
  config.database_url = DATABASE_URL;

  // Sign up
  if (Firebase.signUp(&config, &auth, "", ""))
  {
    Serial.println("Firebase Connected");
    signupOK = true;
  }
  else
  {
    Serial.printf("%s\n", config.signer.signupError.message.c_str());
  }

  // Assign the callback function for the long-running token generation task
  config.token_status_callback = tokenStatusCallback; // see addons/TokenHelper.h

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  // Connect to WiFi
  Serial.printf("Connecting to %s ", ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
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
  // Read sensor values
  float tds = TDSreading(temperature);
  String tanklevel = tanklevel_check(Tankhight); // Check tank level
  String Hours = HourStatus();  
  
  String readFirebase = readString("Waterpump/waterpump"); 

  // Log TDS value
  if (tds >= 0)
  { // Validate TDS value
    Serial.print("TDS Value: ");

    String formattedTDS = String(tds, 2); 
    Serial.print(tds, 2); // Print TDS value to 2 decimal places
    Serial.println(" ppm");
    // Condition: Stop pump if TDS is critically low
    if (tds > 0.00 && tds <= 8.00)
    {
      digitalWrite(Pump, LOW);
      Serial.println("Water PUMP is OFF due to low TDS");
    }
  }
 
   unsigned long currentMillis = millis();

  // Send float data every 5 seconds
  if (currentMillis - floatDataTimer >= interval) {
    floatDataTimer = currentMillis;
    send_Sensordata(tds, "Waterpump/Tds");
  }

  // Send string data every 10 seconds
  if (currentMillis - stringDataTimer >= interval * 2) { // 10 seconds interval
    stringDataTimer = currentMillis;
    send_Message(Hours, "Waterpump/Hourstatus");
    send_Message(tanklevel, "Waterpump/Waterlevel");
  }

  // Handle tank level conditions
  if (tanklevel == "LOW")
  {
    if ((Hours == "REGULAR" || Hours == "OFF-PEAK") && tds > 8.00 && tds <= 12.00)
    {
      digitalWrite(Pump, HIGH);
      digitalWrite(filtervalve, LOW);
      Serial.println("Water PUMP is ON | (8-12 ppm)");
    }
    else if ((Hours == "REGULAR" || Hours == "OFF-PEAK") && tds > 12.00)
    {
      digitalWrite(Pump, LOW);
      digitalWrite(filtervalve, HIGH);
      Serial.println("Filter valve ON & Water PUMP OFF | (TDS > 12 ppm)");
    }
    else if (readFirebase == "OFF")
    {
      digitalWrite(Pump, LOW);
      Serial.println("Water PUMP OFF | From APP ");
    }
    else if (readFirebase == "ON")
    {
      digitalWrite(Pump, HIGH);
      Serial.println("Water PUMP ON | From APP ");
    }

    else
    {
      digitalWrite(Pump, LOW);
      digitalWrite(filtervalve, LOW);
      Serial.println("Water PUMP & Filter valve OFF | (No condition met)");
    }
  }
  else if (tanklevel == "HIGH")
  {
    // Tank is full
    digitalWrite(Pump, LOW);
    digitalWrite(filtervalve, LOW);
    Serial.println("Water PUMP & Filter valve OFF | (Tank level HIGH)");
  }

  delay(2000); // 2-second delay to stabilize loop
}