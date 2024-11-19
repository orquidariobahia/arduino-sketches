// Definitions

#define MY_RADIO_RF24
#define MY_DEBUG
// #define MY_PASSIVE_NODE
#define MY_REPEATER_FEATURE
#define MY_NODE_ID 71
#define MY_TRANSPORT_WAIT_READY_MS 1
// #define SETUP

#define SHT31_ADDRESS 0x44

// Relay setup

#define RELAY_PIN 4
#define RELAY_ON 1
#define RELAY_OFF 0

// Sensors ID (0-39)

#define TEMP_ID 1
#define HUM_ID 2
// #define CO2_ID 3
// #define TVOC_ID 4
// #define AQI_ID 5

// Actuators ID (60-99)

#define RELAY_ID 60

// Configurations (120-159)

#define AUTONOMOUS_ID 120
#define UPDATE_FREQUENCY_ID 121
#define TEMPERATURE_ADJUST_ID 122
#define HUMIDITY_ADJUST_ID 123
#define MIN_HUMIDITY_ID 124
#define MAX_HUMIDITY_ID 125
// #define MIN_CO2_ID 126
// #define MAX_CO2_ID 127

// Libraries

#include "Wire.h"
#include "SHT31.h"
#include <MySensors.h>

// Global variables

SHT31 sht;
bool autonomous = false;
int relayStatus = RELAY_OFF;
float maxHumidity = 999.0;
float minHumidity = -999.0;
uint32_t updateFrequency = 10000;
float tempAdjust = 0.0;
float humAdjust = 0.0;
bool justOn = true;

// Messages

MyMessage relay(RELAY_ID, V_STATUS);
MyMessage temp(TEMP_ID, V_TEMP);
MyMessage hum(HUM_ID, V_HUM);


void setup() {
  send(relay.set(relayStatus == RELAY_ON));
  send(MyMessage(AUTONOMOUS_ID, V_STATUS).set(autonomous));
  send(MyMessage(UPDATE_FREQUENCY_ID, V_TEXT).set(updateFrequency));
  send(MyMessage(TEMPERATURE_ADJUST_ID, V_TEMP).set((float)tempAdjust, 1));
  send(MyMessage(HUMIDITY_ADJUST_ID, V_HUM).set((float)humAdjust, 1));
  send(MyMessage(MAX_HUMIDITY_ID, V_HUM).set((float)maxHumidity, 1));
  send(MyMessage(MIN_HUMIDITY_ID, V_HUM).set((float)minHumidity, 1));
}

void before() {
  pinMode(RELAY_PIN, OUTPUT);

  Serial.begin(115200);
  Wire.begin();
  Wire.setClock(100000);
  sht.begin();

  uint16_t stat = sht.readStatus();

  digitalWrite(RELAY_PIN, RELAY_OFF);

#ifdef SETUP
  storeEeprom_int32(MAX_HUMIDITY_ID, toUint32(93.0f));
  storeEeprom_int32(MIN_HUMIDITY_ID, toUint32(80.0f));
  storeEeprom(AUTONOMOUS_ID, true);
  storeEeprom_int32(UPDATE_FREQUENCY_ID, 60000L);
  storeEeprom_int32(TEMPERATURE_ADJUST_ID, toUint32(0.0f));
  storeEeprom_int32(HUMIDITY_ADJUST_ID, toUint32(0.0f));
#endif

  autonomous = readEeprom(AUTONOMOUS_ID);
  maxHumidity = fromUint32(readEeprom_int32(MAX_HUMIDITY_ID));
  minHumidity = fromUint32(readEeprom_int32(MIN_HUMIDITY_ID));
  updateFrequency = readEeprom_int32(UPDATE_FREQUENCY_ID);
  tempAdjust = fromUint32(readEeprom_int32(TEMPERATURE_ADJUST_ID));
  humAdjust = fromUint32(readEeprom_int32(HUMIDITY_ADJUST_ID));
}

void presentation() {
  // Send the sketch version information to the gateway and controller
  sendSketchInfo("SHT31_and_Relay", "2.0");

  // Register all sensors to gw (they will be created as child devices)
  present(RELAY_ID, S_BINARY, "Relay Status");
  present(TEMP_ID, S_TEMP, "Temperature");
  present(HUM_ID, S_HUM, "Humidity");
  present(AUTONOMOUS_ID, S_BINARY, "Autonomous");
  present(MAX_HUMIDITY_ID, S_HUM, "Maximum Humidity");
  present(MIN_HUMIDITY_ID, S_HUM, "Minimum Humidity");
  present(UPDATE_FREQUENCY_ID, S_INFO, "Update Frequency (s)");
  present(TEMPERATURE_ADJUST_ID, S_TEMP, "Temperature Adjust (+/- C)");
  present(HUMIDITY_ADJUST_ID, S_HUM, "Humidity Adjust (+/- %)");
}

void loop() {
  /* TODO
  Changes are needed
  call -> bool isConnected() 
  call -> int getError() -> https://github.com/RobTillaart/SHT31?tab=readme-ov-file#error-interface
  */
  sht.read(false);
  float t = sht.getTemperature() + tempAdjust;
  float h = sht.getHumidity() + humAdjust;
  if (autonomous) {
    if ((h < minHumidity && relayStatus == RELAY_OFF) || (justOn && h < maxHumidity)) {
#ifdef MY_DEBUG
      Serial.println("Turning on");
      Serial.println(minHumidity);
#endif
      justOn = false;
      relayStatus = RELAY_ON;
      digitalWrite(RELAY_PIN, relayStatus);
      send(relay.set(relayStatus == RELAY_ON));
    } else if (h > maxHumidity && relayStatus == RELAY_ON) {
#ifdef MY_DEBUG
      Serial.println("Turning off");
      Serial.println(maxHumidity);
#endif
      relayStatus = RELAY_OFF;
      digitalWrite(RELAY_PIN, relayStatus);
      send(relay.set(relayStatus == RELAY_ON));
    }
  }
#ifdef MY_DEBUG
  Serial.print(t);
  Serial.print(" - ");
  Serial.println(h);
  Serial.println(updateFrequency);
#endif
  send(temp.set(t, 1));
  send(hum.set(h, 1));
  wait(updateFrequency);
}

void receive(const MyMessage &message) {
// We only expect one type of message from controller. But we better check anyway.
#ifdef MY_DEBUG
  Serial.println(autonomous);
  Serial.print("Incoming message for sensor:");
  Serial.print(message.getSensor());
  Serial.print(" - ");
  Serial.println(message.getType());
#endif

  switch (message.getSensor()) {
    case RELAY_ID:
      relayStatus = message.getBool() ? RELAY_ON : RELAY_OFF;
      digitalWrite(RELAY_PIN, relayStatus);
      break;
    case AUTONOMOUS_ID:
      autonomous = message.getBool();
      storeEeprom(AUTONOMOUS_ID, autonomous);
      break;
    case MIN_HUMIDITY_ID:
      minHumidity = message.getFloat();
      storeEeprom_int32(MIN_HUMIDITY_ID, toUint32(minHumidity));
      break;
    case MAX_HUMIDITY_ID:
      maxHumidity = message.getFloat();
      storeEeprom_int32(MAX_HUMIDITY_ID, toUint32(maxHumidity));
      break;
    case UPDATE_FREQUENCY_ID:
      updateFrequency = message.getULong();
      storeEeprom_int32(UPDATE_FREQUENCY_ID, updateFrequency);
      break;
    case HUMIDITY_ADJUST_ID:
      humAdjust = message.getFloat();
      storeEeprom_int32(HUMIDITY_ADJUST_ID, toUint32(humAdjust));
      break;
    case TEMPERATURE_ADJUST_ID:
      tempAdjust = message.getFloat();
      storeEeprom_int32(TEMPERATURE_ADJUST_ID, toUint32(tempAdjust));
      break;
  }
}
