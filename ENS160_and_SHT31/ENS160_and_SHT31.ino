// Definitions

#define MY_NODE_ID 67
#define SKETCH_NAME "ENS160 and SHT31"
#define SKETCH_VERSION "v1.2"
#define SKETCH_DESCRIPTION "Container próximo da porta"

#define MY_RADIO_RF24
// #define MY_DEBUG
// #define MY_REPEATER_FEATURE
// #define MY_TRANSPORT_WAIT_READY_MS 1
// #define MY_PASSIVE_NODE

#define INT_MINUMUM_VALUE -32768
#define UINT8_MAX_VALUE 255

#define SHT31_ADDRESS 0x44

// Relay setup

// #define RELAY_PIN 4  // Arduino Digital I/O pin number for first relay (second on pin+1 etc)
// #define RELAY_ON 1   // GPIO value to write to turn on attached relay
// #define RELAY_OFF 0  // GPIO value to write to turn off attached relay

// Sensors ID (0-39)

#define TEMP_ID 1
#define HUM_ID 2
#define CO2_ID 3
#define TVOC_ID 4
#define AQI_ID 5
#define TEMP_ID_ADDITIONAL 6
#define HUM_ID_ADDITIONAL 7

// Actuators ID (60-99)

// #define RELAY_ID 60

// Configurations (120-159)

// #define AUTONOMOUS_ID 120
#define UPDATE_FREQUENCY_ID 121
#define TEMPERATURE_ADJUST_ID 122
#define HUMIDITY_ADJUST_ID 123
// #define MIN_HUMIDITY_ID 124
// #define MAX_HUMIDITY_ID 125
// #define MIN_CO2_ID 126
// #define MAX_CO2_ID 127

// Libraries

#include "Wire.h"
#include "SHT31.h"
#include <AHTxx.h>
#include <ScioSense_ENS160.h>
#include <MySensors.h>

// Global variables

AHTxx aht20(AHTXX_ADDRESS_X38, AHT2x_SENSOR);  //sensor address, sensor type
ScioSense_ENS160 ens160(ENS160_I2CADDR_1);
SHT31 sht;

uint32_t updateFrequency = 60000;
float tempAdjust = 0.0;
float humAdjust = 0.0;
uint8_t loopCount = 1;

// Messages

MyMessage temp(TEMP_ID, V_TEMP);
MyMessage hum(HUM_ID, V_HUM);
MyMessage temp2(TEMP_ID, V_TEMP);
MyMessage hum2(HUM_ID, V_HUM);
MyMessage tvoc(TVOC_ID, V_LEVEL);
MyMessage eco2(CO2_ID, V_LEVEL);
MyMessage aqi(AQI_ID, V_LEVEL);

void setup() {
  send(MyMessage(UPDATE_FREQUENCY_ID, V_CUSTOM).set(updateFrequency));
  send(MyMessage(TEMPERATURE_ADJUST_ID, V_TEMP).set(tempAdjust, 1));
  send(MyMessage(HUMIDITY_ADJUST_ID, V_HUM).set(humAdjust, 1));
}

void before() {
  Wire.begin();
  Wire.setClock(100000);

  Serial.print("Node ID: ");
  Serial.println(MY_NODE_ID);
  Serial.println(SKETCH_DESCRIPTION);
  Serial.print("Sketch Identification: ");
  Serial.print(SKETCH_NAME);
  Serial.print(" - ");
  Serial.println(SKETCH_VERSION);
  Serial.println(" - by Shirkit @ https://github.com/orquidariobahia/arduino-sketches");

  bool beg = sht.begin();
  uint16_t stat = sht.readStatus();
  // TODO process the stat
  sht.read();
  int err = sht.getError();
  Serial.println(beg ? "SHT begin" : "SHT not begin");
  Serial.println(sht.isConnected() ? "SHT Connected" : "SHT not connected");
  Serial.print("SHT Stat: ");
  Serial.print(stat);
  Serial.print(" SHT Stat HEX: ");
  Serial.print(stat, HEX);
  Serial.print(" SHT err: ");
  Serial.print(err);
  Serial.print(" SHT err HEX: ");
  Serial.println(err, HEX);

  if (aht20.begin()) {
    Serial.println("AHT21 ok");
  } else {
    printStatus();
  }

  ens160.begin(true);
  if (ens160.available()) {
    bool r = ens160.setMode(ENS160_OPMODE_STD);
    Serial.println(r ? "done." : "failed!");
    // Print ENS160 versions
    Serial.print("\tRev: ");
    Serial.print(ens160.getMajorRev());
    Serial.print(".");
    Serial.print(ens160.getMinorRev());
    Serial.print(".");
    Serial.println(ens160.getBuild());

    Serial.print("\tStandard mode ");
  } else {
    bool res = ens160.setMode(ENS160_OPMODE_STD);
    Serial.println("Could not initialize ENS160");
    Serial.println(res);
  }

  updateFrequency = readEeprom_int32(UPDATE_FREQUENCY_ID);
  tempAdjust = readEeprom_int32(TEMPERATURE_ADJUST_ID);
  humAdjust = readEeprom_int32(HUMIDITY_ADJUST_ID);
}

void presentation() {
  sendSketchInfo(SKETCH_NAME, SKETCH_VERSION);

  present(TEMP_ID, S_TEMP, "[s] Temperature (°C)");
  present(HUM_ID, S_HUM, "[s] Humidity (%)");
  present(CO2_ID, S_AIR_QUALITY, "[s] CO2 (ppm)");
  present(TVOC_ID, S_AIR_QUALITY, "[s] TVOC (ppb)");
  present(AQI_ID, S_AIR_QUALITY, "[s] Air Quali - AQI (1-5)");
  present(TEMP_ID, S_TEMP, "[s] Temp Addi (°C)");
  present(HUM_ID, S_HUM, "[s] Hum Addi (%)");

  present(UPDATE_FREQUENCY_ID, S_CUSTOM, "[c] Updt Freq (uint32 ms)");
  present(TEMPERATURE_ADJUST_ID, S_TEMP, "[c] Temp Adj (float °C)");
  present(HUMIDITY_ADJUST_ID, S_HUM, "[c] Hum Adj (float %)");
}

void loop() {  
  sht.read(false);
  float t = sht.getTemperature() + tempAdjust;
  float h = sht.getHumidity() + humAdjust;

#ifdef MY_DEBUG
  Serial.print("SHT: ");
  Serial.print(t);
  Serial.print("ºC - ");
  Serial.print(h);
  Serial.println("%");
#endif
  send(temp.set(t, 1));
  send(hum.set(h, 1));

  float readTemp = aht20.readTemperature() + tempAdjust;

  if (readTemp != AHTXX_ERROR) {
    send(temp2.set(readTemp, 1));
#ifdef MY_DEBUG
    Serial.print(F("Temperature: "));
    Serial.print(readTemp);
    Serial.println(F(" +-0.3C"));
  } else {
    Serial.print(F("Temperature: "));
    printStatus();
#endif
  }


  float readHum = aht20.readHumidity(AHTXX_USE_READ_DATA) + humAdjust;

  if (readHum != AHTXX_ERROR) {
    send(hum2.set(readHum, 1));
#ifdef MY_DEBUG
    Serial.print(F("Humidity: "));
    Serial.print(readHum);
    Serial.println(F(" +-2%"));
  } else {
    Serial.print(F("Humidity: "));
    printStatus();
#endif
  }

  uint32_t time = updateFrequency * ((uint32_t) loopCount);
  loopCount++;

  if (ens160.available()) {
    if (time > 900000 || loopCount == 0) {
      ens160.set_envdata(readTemp, readHum);
      loopCount = 1;
    }

    ens160.measure(true);
    ens160.measureRaw(true);

    send(eco2.set(ens160.geteCO2()));
    send(tvoc.set(ens160.getTVOC()));
    send(aqi.set(ens160.getAQI()));

#ifdef MY_DEBUG
    Serial.print("AQI: ");
    Serial.print(ens160.getAQI());
    Serial.print("\t");
    Serial.print("TVOC: ");
    Serial.print(ens160.getTVOC());
    Serial.print("ppb\t");
    Serial.print("eCO2: ");
    Serial.print(ens160.geteCO2());
    Serial.println("ppm\t");
#endif
  }

  wait(updateFrequency);
}

void receive(const MyMessage &message) {
// We only expect one type of message from controller. But we better check anyway.
#ifdef MY_DEBUG
  Serial.print("Incoming message for sensor:");
  Serial.print(message.getSensor());
  Serial.print(" - ");
  Serial.println(message.getType());
#endif

  switch (message.getSensor()) {
    case UPDATE_FREQUENCY_ID:
      updateFrequency = message.getULong();
      storeEeprom_int32(UPDATE_FREQUENCY_ID, updateFrequency);
      break;
    case HUMIDITY_ADJUST_ID:
      humAdjust = message.getFloat();
      storeEeprom_int32(HUMIDITY_ADJUST_ID, humAdjust);
      break;
    case TEMPERATURE_ADJUST_ID:
      tempAdjust = message.getFloat();
      storeEeprom_int32(TEMPERATURE_ADJUST_ID, tempAdjust);
      break;
  }
}

void printStatus() {
  switch (aht20.getStatus()) {
    case AHTXX_NO_ERROR:
      Serial.println(F("no error"));
      break;

    case AHTXX_BUSY_ERROR:
      Serial.println(F("sensor busy, increase polling time"));
      break;

    case AHTXX_ACK_ERROR:
      Serial.println(F("sensor didn't return ACK, not connected, broken, long wires (reduce speed), bus locked by slave (increase stretch limit)"));
      break;

    case AHTXX_DATA_ERROR:
      Serial.println(F("received data smaller than expected, not connected, broken, long wires (reduce speed), bus locked by slave (increase stretch limit)"));
      break;

    case AHTXX_CRC8_ERROR:
      Serial.println(F("computed CRC8 not match received CRC8, this feature supported only by AHT2x sensors"));
      break;

    default:
      Serial.println(F("unknown status"));
      break;
  }
}