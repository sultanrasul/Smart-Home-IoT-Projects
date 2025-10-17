/*
  Arduino IoT Cloud Variables description

  The following variables are automatically generated and updated when changes are made to the Thing

  int current_Moisture;
  int trigger_Level;
  bool pump_Status;
  bool push_Button;
  bool switch_Variable;

  Variables which are marked as READ/WRITE in the Cloud Thing will also have functions
  which are called when their values are changed from the Dashboard.
  These functions are generated with the Thing and added at the end of this sketch.
*/

#include "thingProperties.h"

// Analog input port
#define SENSOR_IN 34

#define RELAY_OUT 16

// Constant for dry sensor
int DryValue = 2650;

// Constant for wet sensor
int WetValue = 1800;

int soilMoistureValue;
int soilMoisturePercent;

// Variable for pump trigger
int pump_trigger = 30;

void setup() {
  // Initialize serial and wait for port to open:
  Serial.begin(9600);
  // This delay gives the chance to wait for a Serial Monitor without blocking if none is found
  delay(1500);

  // Defined in thingProperties.h
  initProperties();

  // Connect to Arduino IoT Cloud
  ArduinoCloud.begin(ArduinoIoTPreferredConnection);

  /*
     The following function allows you to obtain more information
     related to the state of network and IoT Cloud connection and errors
     the higher number the more granular information you’ll get.
     The default is 0 (only errors).
     Maximum is 4
  */
  setDebugMessageLevel(2);
  ArduinoCloud.printDebugInfo();

  // Set Relay as Output
  pinMode(RELAY_OUT, OUTPUT);

  // Turn off relay
  digitalWrite(RELAY_OUT, LOW);

  // Set Pump Status to Off
  pump_Status = false;
}

void loop() {
  ArduinoCloud.update();

  // Get soil mositure value
  soilMoistureValue = analogRead(SENSOR_IN);

  // Determine soil moisture percentage value
  soilMoisturePercent = map(soilMoistureValue, DryValue, WetValue, 0, 100);

  // Keep values between 0 and 100
  soilMoisturePercent = constrain(soilMoisturePercent, 0, 100);

  // Print raw value to serial monitor for sensor calibration
  Serial.println(soilMoistureValue);
  Serial.println(soilMoisturePercent);

  // Pass soil moisture to cloud variable
  current_Moisture = soilMoisturePercent;
  
  if(switch_Variable == true){
    Serial.println("Mode: Auto");
    if (soilMoisturePercent <= pump_trigger) {
    // Turn pump on
    pumpOn();
    } else {
    // Turn pump off
    pumpOff();
    }
  }else{
    Serial.println("Mode: Manual");
    pumpOff();
    if (push_Button == true) {
        // Turn pump on
        pumpOn();
       } else {
          // Turn pump off
        pumpOff();
      }
  }
  
  

}

void pumpOn() {
  // Turn pump on
  digitalWrite(RELAY_OUT, HIGH);
  pump_Status = true;


}

void pumpOff() {
  // Turn pump off
  digitalWrite(RELAY_OUT, LOW);
  pump_Status = false;

}






/*
  Since TriggerLevel is READ_WRITE variable, onTriggerLevelChange() is
  executed every time a new value is received from IoT Cloud.
*/
void onTriggerLevelChange()  {
  // Add your code here to act upon TriggerLevel change
  pump_trigger = trigger_Level;

}


/*
  Since SwitchVariable is READ_WRITE variable, onSwitchVariableChange() is
  executed every time a new value is received from IoT Cloud.
*/
void onSwitchVariableChange()  {

}



/*
  Since PushButton is READ_WRITE variable, onPushButtonChange() is
  executed every time a new value is received from IoT Cloud.
*/
void onPushButtonChange()  {
  // Add your code here to act upon PushButton change

}

