/*
 * Example for how to use SinricPro Blinds device
 * 
 * If you encounter any issues:
 * - check the readme.md at https://github.com/sinricpro/esp8266-esp32-sdk/blob/master/README.md
 * - ensure all dependent libraries are installed
 *   - see https://github.com/sinricpro/esp8266-esp32-sdk/blob/master/README.md#arduinoide
 *   - see https://github.com/sinricpro/esp8266-esp32-sdk/blob/master/README.md#dependencies
 * - open serial monitor and check whats happening
 * - check full user documentation at https://sinricpro.github.io/esp8266-esp32-sdk
 * - visit https://github.com/sinricpro/esp8266-esp32-sdk/issues and check for existing issues or open a new one
 */

// Uncomment the following line to enable serial debug output
// #define ENABLE_DEBUG

#ifdef ENABLE_DEBUG
  #define DEBUG_ESP_PORT Serial
  #define NODEBUG_WEBSOCKETS
  #define NDEBUG
#endif 

#include <Arduino.h>
#if defined(ESP8266)
  #include <ESP8266WiFi.h>
#elif defined(ESP32) || defined(ARDUINO_ARCH_RP2040)
  #include <WiFi.h>
#endif

#include "SinricPro.h"
#include "SinricProBlinds.h"
#include <Stepper.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>

#define WIFI_SSID         "YOUR_WIFI_NETWORK_NAME"
#define WIFI_PASS         "YOUR_WIFI_PASSWORD"
#define APP_KEY           "your-sinricpro-app-key"        // Example: "de0bxxxx-1x3x-4x3x-ax2x-5dabxxxxxxxx"
#define APP_SECRET        "your-sinricpro-app-secret"     // Example: "bdf6c151-f62f-420c-8460-50b789a7bf5e-xxxxxxxxxxxx"
#define BLINDS_ID         "your-device-id"                // Example: "6615f5ab7c9e6c6fe867e628"
#define BAUD_RATE         9600                            // Change baud rate to your need

int blindsPosition = 0;
bool powerState = false;

int motorPosition = 0; // THIS IS FROM BACKEND
int maxSteps = 46500;

Stepper myStepper(200, 18, 12, 19, 13);

// Function to update motor position in the backend
void updateMotorPositionToBackend(int position) {
  // Create JSON payload
  StaticJsonDocument<200> doc;
  doc["stepPosition"] = position;

  // Serialize JSON to string
  String payload;
  serializeJson(doc, payload);

  // Stored the steps else where, so it always remembers where it last was
  // Send HTTP POST request
  HTTPClient client;
  client.begin("your_domain/updatesteps");
  client.addHeader("Content-Type", "application/json");
  int httpCode = client.POST(payload);

  // Handle response
  if (httpCode > 0) {
    String response = client.getString();
    Serial.printf("HTTP response code: %d\n", httpCode);
    Serial.printf("Response: %s\n", response.c_str());
  } else {
    Serial.printf("Error on HTTP request. HTTP code: %d\n", httpCode);
  }

  // End client
  client.end();
}

int getMotorPositionFromBackend() {
  int position = 0;

  HTTPClient client;
  client.begin("your_domain/getsteps");
  int httpCode = client.GET();
  
  if (httpCode > 0) {
    String payload = client.getString();
    Serial.printf("\nStatus Code: %d\n", httpCode);
    Serial.printf("%s\n", payload.c_str());
    
    // Parse JSON payload
    StaticJsonDocument<200> doc; // adjust the size according to your JSON payload size
    DeserializationError error = deserializeJson(doc, payload);
    
    if (!error) {
      position = doc["stepPosition"]; // extract the value from JSON
      Serial.printf("Motor Position: %d\n", position);
    } else {
      Serial.println("Failed to parse JSON payload");
    }
  } else {
    Serial.printf("Error on HTTP request. HTTP code: %d\n", httpCode);
  }

  return position;
}


bool onPowerState(const String &deviceId, bool &state) {
  Serial.printf("Device %s power turned %s \r\n", deviceId.c_str(), state ? "on" : "off");
  powerState = state;

  if (state) { // If power state is turned on
    // If the blinds are already closed, do nothing
    if (blindsPosition == 100) {
      Serial.println("Blinds are already closed, no action needed");
    } else {
      // Otherwise, close the blinds
      int blindRequest = (100 * maxSteps) / 100;
      int moveSteps = blindRequest - motorPosition;

      int speed = (moveSteps < 0) ? 30 : 150; // Set speed based on the sign of moveSteps

      myStepper.setSpeed(speed);
      myStepper.step(-moveSteps);

      updateMotorPositionToBackend(blindRequest);

      motorPosition = blindRequest;

      Serial.printf("Device %s set position to %d\r\n", deviceId.c_str(), blindRequest);

      motor_Off();
    }
  } else { // If power state is turned off
    // If the blinds are already open, do nothing
    if (blindsPosition == 0) {
      Serial.println("Blinds are already open, no action needed");
    } else {
      // Otherwise, open the blinds
      int blindRequest = (0 * maxSteps) / 100;
      int moveSteps = blindRequest - motorPosition;

      int speed = (moveSteps < 0) ? 30 : 150; // Set speed based on the sign of moveSteps

      myStepper.setSpeed(speed);
      myStepper.step(-moveSteps);

      updateMotorPositionToBackend(blindRequest);

      motorPosition = blindRequest;

      Serial.printf("Device %s set position to %d\r\n", deviceId.c_str(), blindRequest);

      motor_Off();
    }
  }

  return true; // request handled properly
}

bool onRangeValue(const String &deviceId, int &position) {
  int blindRequest = (position * maxSteps) / 100;
  int moveSteps = blindRequest - motorPosition;

  int speed = (moveSteps < 0) ? 30 : 150; // Set speed based on the sign of moveSteps
    
  myStepper.setSpeed(speed);
  myStepper.step(-moveSteps);

  updateMotorPositionToBackend(blindRequest);

  motorPosition = blindRequest;
  
  Serial.printf("Device %s set position to %d\r\n", deviceId.c_str(), position);

  motor_Off();
  return true; // request handled properly
}

bool onAdjustRangeValue(const String &deviceId, int &positionDelta) {
  blindsPosition += positionDelta;
  Serial.printf("Device %s position changed about %i to %d\r\n", deviceId.c_str(), positionDelta, blindsPosition);
  positionDelta = blindsPosition; // calculate and return absolute position
  return true; // request handled properly
}


void motor_Off(){
  digitalWrite(12,LOW);
  digitalWrite(13,LOW);
  digitalWrite(18,LOW);
  digitalWrite(19,LOW);
}

// setup function for WiFi connection
void setupWiFi() {
  Serial.printf("\r\n[Wifi]: Connecting");

  #if defined(ESP8266)
    WiFi.setSleepMode(WIFI_NONE_SLEEP); 
    WiFi.setAutoReconnect(true);
  #elif defined(ESP32)
    WiFi.setSleep(false); 
    WiFi.setAutoReconnect(true);
  #endif

  WiFi.begin(WIFI_SSID, WIFI_PASS);

  while (WiFi.status() != WL_CONNECTED) {
    Serial.printf(".");
    delay(250);
  }
  IPAddress localIP = WiFi.localIP();
  Serial.printf("connected!\r\n[WiFi]: IP-Address is %d.%d.%d.%d\r\n", localIP[0], localIP[1], localIP[2], localIP[3]);
}

void setupSinricPro() {
  // get a new Blinds device from SinricPro
  SinricProBlinds &myBlinds = SinricPro[BLINDS_ID];
  myBlinds.onPowerState(onPowerState);
  myBlinds.onRangeValue(onRangeValue);
  myBlinds.onAdjustRangeValue(onAdjustRangeValue);

  // setup SinricPro
  SinricPro.onConnected([](){ Serial.printf("Connected to SinricPro\r\n"); }); 
  SinricPro.onDisconnected([](){ Serial.printf("Disconnected from SinricPro\r\n"); });
  SinricPro.begin(APP_KEY, APP_SECRET);
}

// main setup function
void setup() {
  Serial.begin(BAUD_RATE); 
  Serial.printf("\r\n\r\n");

  setupWiFi();

  motorPosition = getMotorPositionFromBackend(); // Get motor position from backend

  setupSinricPro();
}

void loop() {
  SinricPro.handle();
}