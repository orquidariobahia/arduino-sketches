// Definitions

#define MY_RADIO_RF24
#define MY_DEBUG
#define MY_REPEATER_FEATURE
#define MY_NODE_ID 61

#define INT_MINUMUM_VALUE -32768
#define UINT8_MAX_VALUE 255

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

#include <AHTxx.h>
#include <ScioSense_ENS160.h>
#include <MySensors.h>

// Global variables

AHTxx aht20(AHTXX_ADDRESS_X38, AHT2x_SENSOR);  //sensor address, sensor type
ScioSense_ENS160 ens160(ENS160_I2CADDR_1);

uint32_t updateFrequency = 10000;
float tempAdjust = 0.0;
float humAdjust = 0.0;

// Messages

MyMessage temp(TEMP_ID, V_TEMP);
MyMessage hum(HUM_ID, V_HUM);
MyMessage tvoc(TVOC_ID, V_LEVEL);
MyMessage eco2(CO2_ID, V_LEVEL);
MyMessage aqi(AQI_ID, V_LEVEL);

void setup() {
  send(MyMessage(UPDATE_FREQUENCY_ID, V_CUSTOM).set(updateFrequency));
  send(MyMessage(TEMPERATURE_ADJUST_ID, V_TEMP).set(tempAdjust, 1));
  send(MyMessage(HUMIDITY_ADJUST_ID, V_HUM).set(humAdjust, 1));
}

void before() {
  Serial.begin(115200);
  Wire.begin();

  if (aht20.begin()) {
#ifdef MY_DEBUG
    Serial.println("AHT21 ok");
  } else {
    printStatus();
#endif
  }

  ens160.begin();
  if (ens160.available()) {
    bool r = ens160.setMode(ENS160_OPMODE_STD);
#ifdef MY_DEBUG
    Serial.println(r ? "done." : "failed!");
    // Print ENS160 versions
    Serial.print("\tRev: ");
    Serial.print(ens160.getMajorRev());
    Serial.print(".");
    Serial.print(ens160.getMinorRev());
    Serial.print(".");
    Serial.println(ens160.getBuild());

    Serial.print("\tStandard mode ");
#endif
  }

  updateFrequency = readEeprom_int32(UPDATE_FREQUENCY_ID);
  tempAdjust = readEeprom_int32(TEMPERATURE_ADJUST_ID);
  humAdjust = readEeprom_int32(HUMIDITY_ADJUST_ID);
}

void presentation() {
  sendSketchInfo("ENS160_and_radio", "1.0");

  present(TEMP_ID, S_TEMP, "[s] Temperature (°C)");
  present(HUM_ID, S_HUM, "[s] Humidity (%)");
  present(CO2_ID, S_AIR_QUALITY, "[s] CO2 (ppm)");
  present(TVOC_ID, S_AIR_QUALITY, "[s] TVOC (ppb)");
  present(AQI_ID, S_AIR_QUALITY, "[s] Air Quality Index - AQI (1-5)");

  present(UPDATE_FREQUENCY_ID, S_CUSTOM, "[c] Update Frequency (uint32 ms)");
  present(TEMPERATURE_ADJUST_ID, S_TEMP, "[c] Temperature Adjust (float °C)");
  present(HUMIDITY_ADJUST_ID, S_HUM, "[c] Humidity Adjust (float %)");
}

void loop() {

  float readTemp = aht20.readTemperature();

  if (readTemp != AHTXX_ERROR) {
    send(temp.set(readTemp, 1));
#ifdef MY_DEBUG
    Serial.print(F("Temperature: "));
    Serial.print(readTemp);
    Serial.println(F(" +-0.3C"));
  } else {
    Serial.print(F("Temperature: "));
    printStatus();
#endif
  }


  float readHum = aht20.readHumidity(AHTXX_USE_READ_DATA);

  if (readHum != AHTXX_ERROR) {
    send(hum.set(readHum, 1));
#ifdef MY_DEBUG
    Serial.print(F("Humidity: "));
    Serial.print(readHum);
    Serial.println(F(" +-2%"));
  } else {
    Serial.print(F("Humidity: "));
    printStatus();
#endif
  }

  if (ens160.available()) {
    ens160.set_envdata(readTemp, readHum);

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

  wait(10000);
}

void storeEeprom(uint8_t pos, uint8_t value) {
  if (pos < 120 || pos > 159) {
    Serial.println("Out of bounds attempt to save");
    return;
  }

  int actualPos = (pos % 40) * 4;

  saveState(actualPos, (value));
}

void storeEeprom_int(uint8_t pos, int value) {
  if (pos < 120 || pos > 159) {
    Serial.println("Out of bounds attempt to save");
    return;
  }

  int actualPos = (pos % 40) * 4;

  saveState(actualPos, (value >> 8));
  saveState(actualPos + 1, (value & 0xff));
}

void storeEeprom_int32(uint8_t pos, uint32_t value) {
  if (pos < 120 || pos > 159) {
    Serial.println("Out of bounds attempt to save");
    return;
  }

  int actualPos = (pos % 40) * 4;

  saveState(actualPos, (value >> 24));
  saveState(actualPos + 1, (value >> 16));
  saveState(actualPos + 2, (value >> 8));
  saveState(actualPos + 3, (value));
}

uint32_t readEeprom_int32(uint8_t pos) {
  if (pos < 120 || pos > 159) {
    Serial.println("Out of bounds attempt to save");
    return INT_MINUMUM_VALUE;
  } else {
    int actualPos = (pos % 40) * 4;
    uint32_t h1, h2, l1, l2;

    h1 = loadState(actualPos);
    h2 = loadState(actualPos + 1);
    l1 = loadState(actualPos + 2);
    l2 = loadState(actualPos + 3);
    h1 = h1 << 24;
    h2 = h2 << 16;
    l1 = l1 << 8;
    return (h1 | h2 | l1 | l2);
  }
}

int readEeprom_int(uint8_t pos) {
  if (pos < 120 || pos > 159) {
    Serial.println("Out of bounds attempt to save");
    return INT_MINUMUM_VALUE;
  } else {
    int actualPos = (pos % 40) * 4;
    int high, low;

    high = loadState(actualPos) << 8;
    low = loadState(actualPos + 1);
    return (high | low);
  }
}

uint8_t readEeprom(uint8_t pos) {
  if (pos < 120 || pos > 159) {
    Serial.println("Out of bounds attempt to save");
    return UINT8_MAX_VALUE;
  } else {
    int actualPos = (pos % 40) * 4;
    int high;

    high = loadState(actualPos);
    return (high);
  }
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

#ifdef MY_DEBUG
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
#endif