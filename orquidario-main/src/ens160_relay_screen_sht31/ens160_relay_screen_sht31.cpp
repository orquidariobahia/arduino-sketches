// Definitions

#define MY_NODE_ID 60
#define SKETCH_NAME "ENS160 / Relay / Screen / SHT31"
#define SKETCH_VERSION "v1.0"
#define SKETCH_DESCRIPTION "Monitorar Laboratório (sem relay)"

#define MY_RADIO_RF24
// #define MY_DEBUG
// #define MY_REPEATER_FEATURE
#define MY_TRANSPORT_WAIT_READY_MS 10000
// #define MY_PASSIVE_NODE
// #define MY_TRANSPORT_UPLINK_CHECK_DISABLED
// #define MY_TRANSPORT_MAX_TSM_FAILURES 100
// #define MY_TRANSPORT_TIMEOUT_FAILURE_STATE_MS		(10*1000ul) // 10 seconds
#define MY_RF24_DATARATE RF24_1MBPS


// Functionality

#define USE_LCD
// #define USE_BUTTON
// #define USE_RELAY

#define SHT31_ADDRESS 0x44

// LCD Setup

#ifdef USE_LCD
#define COLUMS           16   //LCD columns
#define ROWS             2    //LCD rows
#define LCD_SPACE_SYMBOL 0x20 //space symbol from LCD ROM, see p.9 of GDM2004D datasheet
// #define LCD_SCROLL_SPEED 350  //scroll speed in milliseconds
#define UPDATE_SCREEN 10000
#endif

// Relay setup

#ifdef USE_RELAY
#define RELAY_PIN 6  // Arduino Digital I/O pin number for first relay (second on pin+1 etc)
#define RELAY_ON 0   // GPIO value to write to turn on attached relay
#define RELAY_OFF 1  // GPIO value to write to turn off attached relay
#endif

// Sensors ID (0-39)

#define TEMP_ID 1
#define HUM_ID 2
#define CO2_ID 3
#define TVOC_ID 4
#define AQI_ID 5
#define TEMP_ID_ADDITIONAL 6
#define HUM_ID_ADDITIONAL 7

// Actuators ID (60-99)

#ifdef USE_RELAY
#define RELAY_ID 60
#endif

// Configurations (120-159)

#ifdef USE_RELAY
#define AUTONOMOUS_ID 120
#define MIN_HUMIDITY_ID 124
#define MAX_HUMIDITY_ID 125
#define MIN_CO2_ID 126
#define MAX_CO2_ID 127
#endif
#define UPDATE_FREQUENCY_ID 121
#define TEMPERATURE_ADJUST_ID 122
#define HUMIDITY_ADJUST_ID 123

// Libraries

#include "Wire.h"
#include <SHT31.h>
#include <AHTxx.h>
#include <ScioSense_ENS160.h>
#include <MySensors.h>
#include <Serialization.h>
#include <LiquidCrystal_I2C.h>

// Global variables

#ifdef USE_LCD
LiquidCrystal_I2C lcd(PCF8574_ADDR_A21_A11_A01, 4, 5, 6, 16, 11, 12, 13, 14, POSITIVE);
unsigned long updateScreenTime = 0;
#endif
AHTxx aht20(AHTXX_ADDRESS_X38, AHT2x_SENSOR);  //sensor address, sensor type
ScioSense_ENS160 ens160(ENS160_I2CADDR_1);
SHT31 sht;

uint32_t updateFrequency = 60000;
float tempAdjust = 0.0;
float humAdjust = 0.0;
bool justOn = true;
float maxHumidity = 0;
float minHumidity = 0;
uint16_t maxCo2 = 0;
uint16_t minCo2 = 0;
unsigned long updateSensorsTime = 0;
#ifdef USE_RELAY
bool autonomous = false;
int relayStatus = RELAY_OFF;
#endif

// Messages

MyMessage temp(TEMP_ID, V_TEMP);
MyMessage hum(HUM_ID, V_HUM);
MyMessage temp2(TEMP_ID_ADDITIONAL, V_TEMP);
MyMessage hum2(HUM_ID_ADDITIONAL, V_HUM);
MyMessage tvoc(TVOC_ID, V_LEVEL);
MyMessage eco2(CO2_ID, V_LEVEL);
MyMessage aqi(AQI_ID, V_LEVEL);
#ifdef USE_RELAY
MyMessage relay(RELAY_ID, V_STATUS);
#endif

String getShtError() {
  switch (sht.getError()) {
    case SHT31_OK:
      return F("no error");

    case SHT31_ERR_WRITECMD:
      return F("I2C write failed");

    case SHT31_ERR_READBYTES:
      return F("I2C read failed");

    case SHT31_ERR_HEATER_OFF:
      return F("Could not switch off heater");

    case SHT31_ERR_NOT_CONNECT:
      return F("Could not connect");

    case SHT31_ERR_CRC_TEMP:
      return F("CRC error in temperature");

    case SHT31_ERR_CRC_HUM:
      return F("CRC error in humidity");

    case SHT31_ERR_CRC_STATUS:
      return F("CRC error in status field");

    case SHT31_ERR_HEATER_COOLDOWN:
      return F("Heater need to cool down");

    case SHT31_ERR_HEATER_ON:
      return F("Could not switch on heater");

    default:
      return F("unknown error");
  }
}

String getAhtStatus() {
  switch (aht20.getStatus()) {
    case AHTXX_NO_ERROR:
      return F("no error");

    case AHTXX_BUSY_ERROR:
      return F("sensor busy, increase polling time");

    case AHTXX_ACK_ERROR:
      return F("sensor didn't return ACK, not connected, broken, long wires (reduce speed), bus locked by slave (increase stretch limit)");

    case AHTXX_DATA_ERROR:
      return F("received data smaller than expected, not connected, broken, long wires (reduce speed), bus locked by slave (increase stretch limit)");

    case AHTXX_CRC8_ERROR:
      return F("computed CRC8 not match received CRC8, this feature supported only by AHT2x sensors");

    default:
      return F("unknown status");
  }
}

void setup() {
  #ifdef USE_LCD
  lcd.clear();
  lcd.home();
  lcd.print(F("Setting up"));
  #endif
  Serial.println(F("Setting up"));
  #ifdef USE_RELAY
  send(relay.set(relayStatus == RELAY_ON));
  wait(250);
  send(MyMessage(AUTONOMOUS_ID, V_STATUS).set(autonomous));
  wait(250);
  send(MyMessage(MAX_HUMIDITY_ID, V_HUM).set((float)maxHumidity, 1));
  wait(250);
  send(MyMessage(MIN_HUMIDITY_ID, V_HUM).set((float)minHumidity, 1));
  wait(250);
  #endif
  send(MyMessage(UPDATE_FREQUENCY_ID, V_TEXT).set(updateFrequency));
  wait(250);
  send(MyMessage(TEMPERATURE_ADJUST_ID, V_TEMP).set((float)tempAdjust, 1));
  wait(250);
  send(MyMessage(HUMIDITY_ADJUST_ID, V_HUM).set((float)humAdjust, 1));
  wait(250);
}

void before() {
  Wire.begin();
  Wire.setClock(100000);

  #ifdef USE_LCD
  if (lcd.begin(COLUMS, ROWS, LCD_5x8DOTS) != 1) //colums, rows, characters size
  {
    Serial.println(F("PCF8574 is not connected or lcd pins declaration is wrong. Only pins numbers: 4,5,6,16,11,12,13,14 are legal."));
  }
  lcd.clear();
  lcd.home();
  lcd.leftToRight();
  lcd.print(F("Botting up"));
  #endif

  #ifdef USE_RELAY
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, RELAY_OFF);
  #endif

  Serial.println(F("-----------------------------------------------"));
  Serial.print(F("Sketch Identification: "));
  Serial.print(SKETCH_NAME);
  Serial.print(F(" - "));
  Serial.println(SKETCH_VERSION);
  Serial.print(F("Desription: "));
  Serial.println(SKETCH_DESCRIPTION);
  Serial.print(F("Node ID: "));
  Serial.println(MY_NODE_ID);
  Serial.println(F(" - by Shirkit @ https://github.com/orquidariobahia/arduino-sketches"));
  Serial.println(F("-----------------------------------------------"));

  bool beg = sht.begin();
  // TODO process the stat
  sht.read();
  Serial.print(F("SHT begin: "));
  Serial.println(beg ? F("true") : F("false"));
  Serial.print(F("SHT connected: "));
  Serial.println(sht.isConnected() ? F("true") : F("false"));
  Serial.print(F("SHT error: "));
  Serial.println(getShtError());

  #ifdef USE_LCD
  int err = sht.getError();
  if (!beg || err != SHT31_OK) {
    lcd.home();
    lcd.clear();
    lcd.print(F("SHT31 begin: "));
    lcd.print(beg ? F("true") : F("false"));
    lcd.setCursor(0, 1);
    lcd.print(getShtError());
    wait(2000);
  }
  #endif

  aht20.begin();
  Serial.print(F("AHTXXX status: "));
  Serial.println(getAhtStatus());
  #ifdef USE_LCD
  if (aht20.getStatus() != AHTXX_NO_ERROR) {
    lcd.home();
    lcd.clear();
    lcd.print(F("AHT20 status: "));
    lcd.setCursor(0, 1);
    lcd.print(getAhtStatus());
    wait(2000);
  }
  #endif

  ens160.begin(false);
  if (ens160.available()) {
    bool r = ens160.setMode(ENS160_OPMODE_STD);
    Serial.print(F("ENS160 init: "));
    Serial.println(r ? F("done.") : F("failed!"));
    // Print ENS160 versions
    Serial.print(F("\tRev: "));
    Serial.print(ens160.getMajorRev());
    Serial.print(F("."));
    Serial.print(ens160.getMinorRev());
    Serial.print(F("."));
    Serial.println(ens160.getBuild());

    Serial.println(F("\tStandard mode "));
  } else {
    bool res = ens160.setMode(ENS160_OPMODE_STD);
    Serial.println(F("Could not initialize ENS160"));
    Serial.println(res);
    #ifdef USE_LCD
    lcd.home();
    lcd.clear();
    lcd.print(F("ENS160 error!"));
    wait(2000);
    #endif
  }

  storeEeprom_int32(UPDATE_FREQUENCY_ID, 60000);
  storeEeprom_int32(TEMPERATURE_ADJUST_ID, 0);
  storeEeprom_int32(HUMIDITY_ADJUST_ID, 0);
  // storeEeprom(AUTONOMOUS_ID, true);
  // storeEeprom_int32(MAX_HUMIDITY_ID, toUint32(0.0f));
  // storeEeprom_int32(MIN_HUMIDITY_ID, toUint32(0.0f));
  // storeEeprom_int32(MAX_CO2_ID, 800);
  // storeEeprom_int32(MIN_CO2_ID, 500);

  updateFrequency = readEeprom_int32(UPDATE_FREQUENCY_ID);
  tempAdjust = fromUint32(readEeprom_int32(TEMPERATURE_ADJUST_ID));
  humAdjust = fromUint32(readEeprom_int32(HUMIDITY_ADJUST_ID));
  #ifdef USE_RELAY
  autonomous = readEeprom(AUTONOMOUS_ID);
  maxHumidity = fromUint32(readEeprom_int32(MAX_HUMIDITY_ID));
  minHumidity = fromUint32(readEeprom_int32(MIN_HUMIDITY_ID));
  maxCo2 = readEeprom_int32(MAX_CO2_ID);
  minCo2 = readEeprom_int32(MIN_CO2_ID);
  #endif

  #ifdef USE_LCD
  lcd.clear();
  lcd.home();
  lcd.print(F("Init Transport"));  
  #endif
}

void presentation() {
  #ifdef USE_LCD
  lcd.clear();
  lcd.home();
  lcd.print(F("Presenting"));
  #endif
  Serial.println(F("Presenting"));
  sendSketchInfo(SKETCH_NAME, SKETCH_VERSION);
  wait(250);

  #ifdef USE_RELAY
  present(RELAY_ID, S_BINARY, "[s] Relay Status (bool)");
  wait(250);
  #endif
  present(TEMP_ID, S_TEMP, "[s] Temperature (°C)");
  wait(250);
  present(HUM_ID, S_HUM, "[s] Humidity (%)");
  wait(250);
  present(CO2_ID, S_AIR_QUALITY, "[s] CO2 (ppm)");
  wait(250);
  present(TVOC_ID, S_AIR_QUALITY, "[s] TVOC (ppb)");
  wait(250);
  present(AQI_ID, S_AIR_QUALITY, "[s] Air Quali - AQI (1-5)");
  wait(250);
  present(TEMP_ID_ADDITIONAL, S_TEMP, "[s] Temp Addi (°C)");
  wait(250);
  present(HUM_ID_ADDITIONAL, S_HUM, "[s] Hum Addi (%)");
  wait(250);
  
  
  present(UPDATE_FREQUENCY_ID, S_CUSTOM, "[c] Updt Freq (uint32 ms)");
  wait(250);
  present(TEMPERATURE_ADJUST_ID, S_TEMP, "[c] Temp Adj (float °C)");
  wait(250);
  present(HUMIDITY_ADJUST_ID, S_HUM, "[c] Hum Adj (float %)");
  wait(250);

  #ifdef USE_RELAY
  present(AUTONOMOUS_ID, S_BINARY, "[c] Autonomous");
  wait(250);
  present(MAX_HUMIDITY_ID, S_HUM, "[c] Max Humid (float °C)");
  wait(250);
  present(MIN_HUMIDITY_ID, S_HUM, "[c] Min Humid (float °C)");
  wait(250);
  present(MAX_CO2_ID, S_AIR_QUALITY, "[c] Max CO2 (ppm)");
  wait(250);
  present(MIN_CO2_ID, S_AIR_QUALITY, "[c] Min CO2 (ppm)");
  wait(250);
  #endif

}

void doSensors(bool sendUpdate) {  
  sht.read(false);
  float t = sht.getTemperature() + tempAdjust;
  float h = sht.getHumidity() + humAdjust;

#ifdef MY_DEBUG
  Serial.print(F("SHT: "));
  Serial.print(t);
  Serial.print(F("ºC - "));
  Serial.print(h);
  Serial.println(F("%"));
#endif
  if (sendUpdate) {
    send(temp.set(t, 1));
    wait(250);
    send(hum.set(h, 1));
    wait(250);
  }
  #ifdef USE_LCD
  lcd.clear();
  lcd.home();
  lcd.print(t, 1);
  lcd.print(F(" C"));
  lcd.setCursor(10, 0);
  lcd.print(h, 1);
  lcd.print(F(" %"));

  if (!isTransportReady()) {
    lcd.setCursor(11, 1);
    lcd.print(F("T Err"));
  }
  #endif

  float readTemp = aht20.readTemperature() + tempAdjust;

  if (readTemp != AHTXX_ERROR) {
    if (sendUpdate) {
      send(temp2.set(readTemp, 1));
      wait(250);
    }
#ifdef MY_DEBUG
    Serial.print(F("Temperature: "));
    Serial.print(readTemp);
    Serial.println(F(" +-0.3C"));
  } else {
    Serial.print(F("Temperature: "));
    Serial.println(getAhtStatus());
#endif
  }

  float readHum = aht20.readHumidity(AHTXX_USE_READ_DATA) + humAdjust;

  if (readHum != AHTXX_ERROR) {
    if (sendUpdate) {
      send(hum2.set(readHum, 1));
      wait(250);
    }
#ifdef MY_DEBUG
    Serial.print(F("Humidity: "));
    Serial.print(readHum);
    Serial.println(F(" +-2%"));
  } else {
    Serial.print(F("Humidity: "));
    Serial.println(getAhtStatus());
#endif
  }

  if (ens160.available()) {
    // if (time > 900000 || loopCount == 0) {
    //   ens160.set_envdata(t, h);
    //   loopCount = 1;
    // }

    ens160.measure(true);
    ens160.measureRaw(true);

    if (sendUpdate) {
      send(eco2.set(ens160.geteCO2()));
      wait(250);
      send(tvoc.set(ens160.getTVOC()));
      wait(250);
      send(aqi.set(ens160.getAQI()));
      wait(250);
    }

    #ifdef USE_LCD
    lcd.setCursor(0, 1);
    // lcd.print(F("CO2: "));
    lcd.print(ens160.geteCO2());
    lcd.print(F(" ppm"));
    // lcd.print(F(" | TVOC: "));
    // lcd.print(ens160.getTVOC());
    // lcd.print(F(" ppb"));
    // lcd.print(F(" | AQI: "));
    // lcd.print(ens160.getAQI());
    #endif

#ifdef MY_DEBUG
    Serial.print(F("AQI: "));
    Serial.print(ens160.getAQI());
    Serial.print(F("\t"));
    Serial.print(F("TVOC: "));
    Serial.print(ens160.getTVOC());
    Serial.print(F("ppb\t"));
    Serial.print(F("eCO2: "));
    Serial.print(ens160.geteCO2());
    Serial.println(F("ppm\t"));
#endif
  }

  #ifdef USE_RELAY
  if (sendUpdate && autonomous) {
    if (maxHumidity > 0 && minHumidity > 0) {
      if ((h < minHumidity && relayStatus == RELAY_OFF) || (justOn && h < maxHumidity)) {
        #ifdef MY_DEBUG
        Serial.println(F("Turning on"));
        Serial.println(minHumidity);
  #endif
        justOn = false;
        relayStatus = RELAY_ON;
        digitalWrite(RELAY_PIN, relayStatus);
        send(relay.set(relayStatus == RELAY_ON));
      } else if (h > maxHumidity && relayStatus == RELAY_ON) {
        #ifdef MY_DEBUG
        Serial.println(F("Turning off"));
        Serial.println(maxHumidity);
        #endif
        relayStatus = RELAY_OFF;
        digitalWrite(RELAY_PIN, relayStatus);
        send(relay.set(relayStatus == RELAY_ON));
      }
    } else if (minCo2 > 0 && maxCo2 > 0) {
      if ((ens160.geteCO2() > maxCo2 && relayStatus == RELAY_OFF) || (justOn && ens160.geteCO2() > minCo2)) {
  #ifdef MY_DEBUG
        Serial.println(F("Turning on"));
        Serial.println(maxCo2);
        #endif
        justOn = false;
        relayStatus = RELAY_ON;
        digitalWrite(RELAY_PIN, relayStatus);
        send(relay.set(relayStatus == RELAY_ON));
      } else if (ens160.geteCO2() < minCo2 && relayStatus == RELAY_ON) {
        #ifdef MY_DEBUG
        Serial.println(F("Turning off"));
        Serial.println(minCo2);
        #endif
        relayStatus = RELAY_OFF;
        digitalWrite(RELAY_PIN, relayStatus);
        send(relay.set(relayStatus == RELAY_ON));
      }
    }
  }

  #ifdef USE_LCD
  if (relayStatus == RELAY_ON) {
    lcd.setCursor(14, 1);
    lcd.print(F("On"));
  } else {
    lcd.setCursor(13, 1);
    lcd.print(F("Off"));
  }
  #endif
  #endif
  justOn = false;
}

void loop()
{
  unsigned long now = millis();
  if (justOn || now - updateSensorsTime >= updateFrequency) {
      doSensors(true);
      justOn = false;
      updateSensorsTime = now;
#ifdef USE_LCD
      updateScreenTime = now;
#endif
  }
  else {
#ifdef USE_LCD
  #ifdef LCD_SCROLL_SPEED
      if (loopCount * BUTTON_DELAY % LCD_SCROLL_SPEED == 0) {
          lcd.scrollDisplayLeft();
      }
  #elif UPDATE_SCREEN
      if (now - updateScreenTime >= UPDATE_SCREEN) {
          updateScreenTime = now;
          doSensors(false);
      }
  #endif
#endif
  }

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
    #ifdef USE_RELAY
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
    case MIN_CO2_ID:
      minCo2 = message.getUInt();
      storeEeprom_int32(MIN_CO2_ID, minCo2);
      break;
    case MAX_CO2_ID:
      maxCo2 = message.getUInt();
      storeEeprom_int32(MAX_CO2_ID, maxCo2);
      break;
    #endif
    case HUMIDITY_ADJUST_ID:
      humAdjust = message.getFloat();
      storeEeprom_int32(HUMIDITY_ADJUST_ID, humAdjust);
      break;
    case TEMPERATURE_ADJUST_ID:
      tempAdjust = message.getFloat();
      storeEeprom_int32(TEMPERATURE_ADJUST_ID, tempAdjust);
      break;
    case CO2_ID:
        #ifdef MY_DEBUG
        Serial.println("Sending CO2");
        #endif
        ens160.measure(true);
        ens160.measureRaw(true);

        eco2.setDestination(message.getSender());
        send(eco2.set(ens160.geteCO2()));
        eco2.setDestination(0);
      break;
    case HUM_ID:
        #ifdef MY_DEBUG
        Serial.println("Sending Humidity");
        #endif
        sht.read(false);
        float h = sht.getHumidity() + humAdjust;

        hum.setDestination(message.getSender());
        send(hum.set(h, 1));
        hum.setDestination(0);
      break;
    case TEMP_ID:
        #ifdef MY_DEBUG
        Serial.println("Sending Temperature");
        #endif
        sht.read(false);
        float t = sht.getTemperature() + tempAdjust;

        temp.setDestination(message.getSender());
        send(temp.set(t, 1));
        temp.setDestination(0);
      break;
  }
}