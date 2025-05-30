// Definitions

#define MY_NODE_ID 50
#define SKETCH_NAME "Flame"
#define SKETCH_VERSION "v0.1"
#define SKETCH_DESCRIPTION "Tonel"

#define MY_RADIO_RF24
#define MY_DEBUG
// #define MY_REPEATER_FEATURE
// #define MY_TRANSPORT_WAIT_READY_MS 1
#define MY_PASSIVE_NODE

#define INT_MINUMUM_VALUE -32768
#define UINT8_MAX_VALUE 255

// Relay setup

#define RELAY_ON 0   // GPIO value to write to turn on attached relay
#define RELAY_OFF 1  // GPIO value to write to turn off attached relay
#define RELAY_PIN 6  // Arduino Digital I/O pin number for first relay (second on pin+1 etc)

// Sensors ID (0-39)

// #define TEMP_ID 1
// #define HUM_ID 2
// #define CO2_ID 3
// #define TVOC_ID 4
// #define AQI_ID 5
// #define TEMP_ID_ADDITIONAL 6
// #define HUM_ID_ADDITIONAL 7
#define FLAME_ID 8

// Actuators ID (60-99)

// #define RELAY_ID 60

// Configurations (120-159)

// #define AUTONOMOUS_ID 120
#define UPDATE_FREQUENCY_ID 121
// #define TEMPERATURE_ADJUST_ID 122
// #define HUMIDITY_ADJUST_ID 123
// #define MIN_HUMIDITY_ID 124
// #define MAX_HUMIDITY_ID 125
// #define MIN_CO2_ID 126
// #define MAX_CO2_ID 127

// Libraries

#include "Serialization.cpp"
#include <MySensors.h>

// Global variables

uint32_t updateFrequency = 60000;
int lastIrRead;

// Messages

// MyMessage temp(TEMP_ID, V_TEMP);

void setup() {
  send(MyMessage(UPDATE_FREQUENCY_ID, V_CUSTOM).set(updateFrequency));
}

void before() {
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, RELAY_OFF);

  Serial.print(F("Node ID: "));
  Serial.println(MY_NODE_ID);
  Serial.print(F("Desription: "));
  Serial.println(SKETCH_DESCRIPTION);
  Serial.print(F("Sketch Identification: "));
  Serial.print(SKETCH_NAME);
  Serial.print(F(" - "));
  Serial.println(SKETCH_VERSION);
  Serial.println(F(" - by Shirkit @ https://github.com/orquidariobahia/arduino-sketches"));
}

void presentation() {
  sendSketchInfo(SKETCH_NAME, SKETCH_VERSION);

  // present(TEMP_ID, S_TEMP, "[s] Temperature (°C)");

  present(UPDATE_FREQUENCY_ID, S_CUSTOM, "[c] Updt Freq (uint32 ms)");
}

void loop() {  

  lastIrRead = analogRead(A7);
  Serial.println(lastIrRead);
  if (lastIrRead > 1000) {
    digitalWrite(RELAY_PIN, RELAY_OFF);
    Serial.println("No Flame");
  } else {
    Serial.println("FIRE!");
    digitalWrite(RELAY_PIN, RELAY_ON  );
  }
  // wait(updateFrequency);
  wait(100);
}

void receive(const MyMessage &message) {
// We only expect one type of message from controller. But we better check anyway.
#ifdef MY_DEBUG
  Serial.print(F("Incoming message for sensor:"));
  Serial.print(message.getSensor());
  Serial.print(F(" - "));
  Serial.println(message.getType());
#endif

  switch (message.getSensor()) {
    case UPDATE_FREQUENCY_ID:
      updateFrequency = message.getULong();
      storeEeprom_int32(UPDATE_FREQUENCY_ID, updateFrequency);
      break;
  }
}