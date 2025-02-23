#include <Arduino.h>
#include <WiFi.h>
#include <Firebase_ESP_Client.h>

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

void setup() {
  Serial.begin(115200);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  // Assign Firebase config
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;

  // Sign up to Firebase
  if (Firebase.signUp(&config, &auth, "", "")) {
    Serial.println("Firebase Connected");
    signupOK = true;
  } else {
    Serial.printf("%s\n", config.signer.signupError.message.c_str());
  }

  // Firebase token status callback
  config.token_status_callback = tokenStatusCallback;
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
}

// Send an integer to Firebase
void send_int(int intdata, String intpath) {
  if (Firebase.ready() && signupOK) {
    if (Firebase.RTDB.setInt(&fbdo, intpath, intdata)) {
      Serial.println("PASSED");
      Serial.println("PATH: " + fbdo.dataPath());
      Serial.println("TYPE: " + fbdo.dataType());
    } else {
      Serial.println("FAILED");
      Serial.println("REASON: " + fbdo.errorReason());
    }
  }
}

// Send a float to Firebase
void send_float(float floatdata, String floatpath) {
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


String readString(String path)
{
    String receivedString = ""; // Initialize an empty string to hold the result
    // Read data from the Firebase path
    if (Firebase.RTDB.getString(&fbdo, path))
    {
        if (fbdo.dataType() == "string")
        {
            receivedString = fbdo.stringData();
            Serial.println("Received string data: " + receivedString);
        }
        else
        {
            Serial.println("Data is not a string!");
        }
    }
    else
    {
        Serial.println("Failed to read string data. Reason: " + fbdo.errorReason());
    }
    return receivedString; // Return the string data
}
float readFloat(String path)
{
    float receivedFloat = 0.0; // Default value if reading fails

    // Read data from the Firebase path
    if (Firebase.RTDB.getFloat(&fbdo, path))
    {
        if (fbdo.dataType() == "float")
        {
            receivedFloat = fbdo.floatData();
            Serial.println("Received float data: " + String(receivedFloat, 2)); // Print with 2 decimal places
        }
        else
        {
            Serial.println("Data is not a float!");
        }
    }
    else
    {
        Serial.println("Failed to read float data. Reason: " + fbdo.errorReason());
    }
    
    return receivedFloat; // Return the float data
}
int readInt(String path)
{
    int receivedInt = 0; // Default value if reading fails

    // Read data from the Firebase path
    if (Firebase.RTDB.getInt(&fbdo, path))
    {
        if (fbdo.dataType() == "int")
        {
            receivedInt = fbdo.intData();
            Serial.println("Received integer data: " + String(receivedInt));
        }
        else
        {
            Serial.println("Data is not an integer!");
        }
    }
    else
    {
        Serial.println("Failed to read integer data. Reason: " + fbdo.errorReason());
    }
    
    return receivedInt; // Return the integer data
}



void loop() {
  // Get the current time
  unsigned long currentMillis = millis();

  // Send float data every 5 seconds
  if (currentMillis - floatDataTimer >= interval) {
    floatDataTimer = currentMillis;
    send_float(23.81, "test/float");
    send_float(2.10, "test/float1");
  }

  // Send string data every 10 seconds
  if (currentMillis - stringDataTimer >= interval * 2) { // 10 seconds interval
    stringDataTimer = currentMillis;
    send_Message("Hi", "test/stringData");
    send_Message("LOW", "test/stringData1");
  }

  String data = readString("test/stringData1");
}
