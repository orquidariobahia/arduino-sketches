#include "Wire.h"
#include "SHT31.h"
#define SHT31_ADDRESS 0x44
SHT31 sht;


#define MY_RADIO_RF24
#define MY_DEBUG
// #define MY_PASSIVE_NODE
#define MY_REPEATER_FEATURE
//#define MY_NODE_ID 71
#define MY_TRANSPORT_WAIT_READY_MS 1

#include <MySensors.h>

#define RELAY_PIN 4  // Arduino Digital I/O pin number for first relay (second on pin+1 etc)
#define RELAY_ON 1   // GPIO value to write to turn on attached relay
#define RELAY_OFF 0  // GPIO value to write to turn off attached relay

#define RELAY_ID 0
#define TEMP_ID 1
#define HUM_ID 2
#define AUTONOMOUS_ID 3
#define UPDATE_FREQUENCY_ID 4
#define MIN_HUMIDITY_ID 6
#define MAX_HUMIDITY_ID 7
#define TEMPERATURE_ADJUST_ID 8
#define HUMIDITY_ADJUST_ID 9

MyMessage relay(RELAY_ID, V_STATUS);
MyMessage temp(TEMP_ID, V_TEMP);
MyMessage hum(HUM_ID, V_HUM);

bool autonomous = false;
int relayStatus = RELAY_OFF;
float maxHumidity = 999.0;
float minHumidity = -999.0;
uint32_t updateFrequency = 10000;
float tempAdjust = 0.0;
float humAdjust = 0.0;

void setup() {
  /*send(relay.set(relayStatus == RELAY_ON));
  send(MyMessage(AUTONOMOUS_ID, V_STATUS).set(autonomous));
  send(MyMessage(UPDATE_FREQUENCY_ID, V_CUSTOM).set(updateFrequency));
  send(MyMessage(TEMPERATURE_ADJUST_ID, V_TEMP).set((float)tempAdjust, 1));
  send(MyMessage(HUMIDITY_ADJUST_ID, V_HUM).set((float)humAdjust, 1));
  send(MyMessage(MAX_HUMIDITY_ID, V_HUM).set((float)maxHumidity, 1));
  send(MyMessage(MIN_HUMIDITY_ID, V_HUM).set((float)minHumidity, 1));*/
  
  send(relay.set(relayStatus == RELAY_ON));
  send(MyMessage(AUTONOMOUS_ID, V_STATUS).set(autonomous));
  send(MyMessage(UPDATE_FREQUENCY_ID, V_CUSTOM).set(updateFrequency));
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

  autonomous = loadState(AUTONOMOUS_ID);
  maxHumidity = loadState(MAX_HUMIDITY_ID);
  minHumidity = loadState(MIN_HUMIDITY_ID);
  updateFrequency = loadState(UPDATE_FREQUENCY_ID);
  updateFrequency *= 1000;
  tempAdjust = loadState(TEMPERATURE_ADJUST_ID);
  humAdjust = loadState(HUMIDITY_ADJUST_ID);
}

void presentation() {
  // Send the sketch version information to the gateway and controller
  sendSketchInfo("SHT31_and_Relay", "1.2");

  // Register all sensors to gw (they will be created as child devices)
  present(RELAY_ID, S_BINARY, "Relay Status");
  present(TEMP_ID, S_TEMP, "Temperature");
  present(HUM_ID, S_HUM, "Humidity");
  present(AUTONOMOUS_ID, S_BINARY, "Autonomous");
  present(MAX_HUMIDITY_ID, S_HUM, "Maximum Humidity");
  present(MIN_HUMIDITY_ID, S_HUM, "Minimum Humidity");
  present(UPDATE_FREQUENCY_ID, S_CUSTOM, "Update Frequency (s)");
  present(TEMPERATURE_ADJUST_ID, S_TEMP, "Temperature Adjust (+/- C)");
  present(HUMIDITY_ADJUST_ID, S_HUM, "Humidity Adjust (+/- %)");
}

void loop() {
  /*
  Changes are needed
  call -> bool isConnected() 
  call -> int getError() -> https://github.com/RobTillaart/SHT31?tab=readme-ov-file#error-interface
  */
  sht.read(false);
  float t = sht.getTemperature() + tempAdjust;
  float h = sht.getHumidity() + humAdjust;
  if (autonomous) {
    if (h < minHumidity && relayStatus == RELAY_OFF) {
#ifdef MY_DEBUG
      Serial.println("Turning on");
      Serial.println(minHumidity);
#endif
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
      saveState(AUTONOMOUS_ID, message.getBool());
      autonomous = message.getBool();
      break;
    case MIN_HUMIDITY_ID:
      minHumidity = message.getFloat();
      saveState(MIN_HUMIDITY_ID, minHumidity);
      break;
    case MAX_HUMIDITY_ID:
      maxHumidity = message.getFloat();
      saveState(MAX_HUMIDITY_ID, maxHumidity);
      break;
    case UPDATE_FREQUENCY_ID:
      updateFrequency = message.getInt();
      updateFrequency *= 1000;
      saveState(UPDATE_FREQUENCY_ID, message.getInt());
      break;
    case HUMIDITY_ADJUST_ID:
      humAdjust = message.getFloat();
      saveState(HUMIDITY_ADJUST_ID, humAdjust);
      break;
    case TEMPERATURE_ADJUST_ID:
      tempAdjust = message.getFloat();
      saveState(TEMPERATURE_ADJUST_ID, tempAdjust);
      break;
  }
}
