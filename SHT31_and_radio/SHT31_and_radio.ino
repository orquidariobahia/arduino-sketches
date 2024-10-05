#include "Wire.h"
#include "SHT31.h"
#define SHT31_ADDRESS 0x44
SHT31 sht;

#define MY_RADIO_RF24
#define MY_DEBUG
// #define MY_PASSIVE_NODE
// #define MY_REPEATER_FEATURE
#define MY_NODE_ID 70

#include <MySensors.h>

#define TEMP_ID 0
#define HUM_ID 1

MyMessage temp(TEMP_ID, V_TEMP);
MyMessage hum(HUM_ID, V_HUM);

void setup() {
}

void before() {
  Serial.begin(115200);
  Wire.begin();
  Wire.setClock(100000);
  sht.begin();

  uint16_t stat = sht.readStatus();

  updateFrequency = loadState(UPDATE_FREQUENCY_ID);
  updateFrequency *= 1000;
}

void presentation() {
  // Send the sketch version information to the gateway and controller
  sendSketchInfo("SHT31_and_Radio", "1.1");

  // Register all sensors to gw (they will be created as child devices)
  present(TEMP_ID, S_TEMP, "Temperature");
  present(HUM_ID, S_HUM, "Humidity");
  present(UPDATE_FREQUENCY_ID, S_CUSTOM, "Update Frequency (s)");
}

void loop() {
  sht.read(false);
  float t = sht.getTemperature();
  float h = sht.getHumidity();

  Serial.print(t);
  Serial.print(" - ");
  Serial.println(h);
  send(temp.set(t, 1));
  send(hum.set(h, 1));
  wait(5000);
}