#define MY_NODE_ID 46
#define SKETCH_NAME "SCD40 / SHT31 / SSD1306"
#define SKETCH_IDENTIFICATION "mysensors_46_co2_temp_hum_screen"
#define SKETCH_VERSION "v1.0"
#define SKETCH_DESCRIPTION "Monitoramento de Do Orquidário 3"

#define MY_RADIO_RF24
// #define MY_DEBUG
// #define MY_REPEATER_FEATURE
// #define FIRST_CONFIGURE
#define MY_TRANSPORT_WAIT_READY_MS 1000
#define MY_PASSIVE_NODE
// #define MY_TRANSPORT_UPLINK_CHECK_DISABLED
// #define MY_TRANSPORT_MAX_TSM_FAILURES 100
// #define MY_TRANSPORT_TIMEOUT_FAILURE_STATE_MS		(10*1000ul) // 10 seconds
#define MY_RF24_DATARATE RF24_1MBPS

// Functionality

#define USE_LCD
#define USE_SHT31
#define USE_SCD40
// #define USE_RELAY

// DS18B20 Setup

#define COMPARE_TEMP // Send temperature only if changed? 1 = Yes 0 = No

// LCD Setup

#ifdef USE_LCD
#define DEGREE      0xDF
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels
#define UPDATE_SCREEN 10000
#define SSD1306_NO_SPLASH
#endif

// Relay setup

#ifdef USE_RELAY
#define RELAY_PIN 7  // Arduino Digital I/O pin number for first relay (second on pin+1 etc)
#define RELAY_ON 0   // GPIO value to write to turn on attached relay
#define RELAY_OFF 1  // GPIO value to write to turn off attached relay
#endif

// SCD40 Setup

#ifdef USE_SCD40
#define NO_ERROR 0
#endif

// Button

// Sensors ID (0-39)

#ifdef USE_SHT31
#define TEMP_ID 1
#define HUM_ID 2
#endif
#ifdef USE_SCD40
#define CO2_ID 3
#endif

// Actuators ID (60-99)

#ifdef USE_RELAY
#define RELAY_ID 60
#endif

// Configurations (120-159)

#ifdef USE_RELAY
#define AUTONOMOUS_ID 120
#ifdef USE_SHT31
#define MIN_HUMIDITY_ID 125
#define MAX_HUMIDITY_ID 126
#endif
#ifdef USE_SCD40
#define MIN_CO2_ID 127
#define MAX_CO2_ID 128
#endif
#endif
#define UPDATE_FREQUENCY_ID 121
#ifdef USE_SHT31
#define TEMPERATURE_ADJUST_ID 122
#define HUMIDITY_ADJUST_ID 123
#endif
#ifdef USE_SCD40
#define CO2_ADJUST_ID 124
#endif

// Libraries

#include <MySensors.h>  
#include <Serialization.h>

#include <Wire.h>
#ifdef USE_LCD
#include <SPI.h>
#include <Adafruit_SSD1306.h>
#endif
#ifdef USE_SHT31
#include <SHT31.h>
#endif
#ifdef USE_SCD40
#include <SensirionI2cScd4x.h>
#endif

// Global variables

#ifdef USE_LCD
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
unsigned long updateScreenTime = 0;
#endif

#ifdef USE_SHT31
SHT31 sht;
#endif

#ifdef USE_SCD40
SensirionI2cScd4x scd4x;
// static char errorMessage[64];
static int16_t error;
#endif

#ifdef USE_RELAY
int relayStatus = RELAY_OFF;
#endif

uint32_t updateFrequency = 60000;
float tempAdjust = 0.0;
float humAdjust = 0.0;
int16_t co2Adjust = 0;
#ifdef USE_RELAY
float maxHumidity = 0;
float minHumidity = 0;
uint16_t maxCo2 = 0;
uint16_t minCo2 = 0;
bool autonomous = false;
int relayStatus = RELAY_OFF;
#endif
bool justOn = true;
unsigned long updateSensorsTime = 0;

// Messages

MyMessage msgTemp(0, V_TEMP);
MyMessage msgHum(0, V_HUM);
MyMessage msgCO2(0, V_LEVEL);
#ifdef USE_RELAY
MyMessage relay(RELAY_ID, V_STATUS);
#endif

#ifdef USE_SHT31
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
#endif

#ifdef USE_SCD40
void PrintUint64(uint64_t& value) {
    Serial.print("0x");
    Serial.print((uint32_t)(value >> 32), HEX);
    Serial.print((uint32_t)(value & 0xFFFFFFFF), HEX);
}
#endif

void setup() {
#ifdef USE_LCD
    display.println(F("Setting up"));
    display.display();
#endif
    Serial.println(F("Setting up"));
#ifdef USE_RELAY
    send(relay.set(relayStatus == RELAY_ON));
    wait(250);
    send(MyMessage(AUTONOMOUS_ID, V_STATUS).set(autonomous));
    wait(250);
#ifdef USE_SHT31
    send(MyMessage(MAX_HUMIDITY_ID, V_HUM).set((float)maxHumidity, 1));
    wait(250);
    send(MyMessage(MIN_HUMIDITY_ID, V_HUM).set((float)minHumidity, 1));
    wait(250);
#endif
#ifdef USE_SCD40
    send(MyMessage(MAX_CO2_ID, V_AIR_QUALITY).set(maxCo2));
    wait(250);
    send(MyMessage(MIN_CO2_ID, V_AIR_QUALITY).set(minCo2));
    wait(250);
#endif
#endif    
    send(MyMessage(UPDATE_FREQUENCY_ID, V_VAR1).set(updateFrequency));
    wait(250);
#ifdef USE_SHT31
    send(MyMessage(TEMPERATURE_ADJUST_ID, V_TEMP).set((float)tempAdjust, 1));
    wait(250);
    send(MyMessage(HUMIDITY_ADJUST_ID, V_HUM).set((float)humAdjust, 1));
    wait(250);
#endif
#ifdef USE_SCD40
    send(MyMessage(CO2_ADJUST_ID, V_LEVEL).set(co2Adjust));
    wait(250);
    send(MyMessage(CO2_ID, V_UNIT_PREFIX).set(F("ppm")));
    wait(250);
#endif
}

void before() {
    Wire.begin();
    Wire.setClock(100000);
    
    #ifdef USE_LCD
    wait(250);
    if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
        Serial.println(F("Erro na Tela"));
    }
    display.clearDisplay();
    display.setTextSize(1);      // Normal 1:1 pixel scale
    display.setTextColor(SSD1306_WHITE); // Draw white text
    display.setCursor(0, 0);     // Start at top-left corner
    display.cp437(true);         // Use full 256 char 'Code Page 437' font
    display.println(F("Booting up"));
    display.display();
#endif
    Serial.println(F("Booting up"));

#ifdef USE_RELAY
    pinMode(RELAY_PIN, OUTPUT);
    digitalWrite(RELAY_PIN, RELAY_OFF);
#endif

    Serial.println(F("-----------------------------------------------"));
    Serial.print(F("Sketch Name: "));
    Serial.print(SKETCH_NAME);
    Serial.print(F(" - "));
    Serial.println(SKETCH_VERSION);
    Serial.print(F("Desription: "));
    Serial.println(SKETCH_DESCRIPTION);
    Serial.print(F("Node ID: "));
    Serial.println(MY_NODE_ID);
    Serial.print(F("Sketch Identification: "));
    Serial.println(SKETCH_IDENTIFICATION);
    Serial.println(F(" - by Shirkit @ https://github.com/orquidariobahia/arduino-sketches"));
    Serial.println(F("-----------------------------------------------"));

#ifdef USE_SHT31
    bool beg = sht.begin();
    // TODO process the stat
    sht.read();
    Serial.print(F("SHT begin: "));
    Serial.println(beg ? F("true") : F("false"));
    Serial.print(F("SHT connected: "));
    Serial.println(sht.isConnected() ? F("true") : F("false"));
    Serial.print(F("SHT error: "));
    Serial.println(getShtError());
#endif

#ifdef USE_SCD40
    scd4x.begin(Wire, SCD40_I2C_ADDR_62);

    delay(250);
    scd4x.wakeUp();
    scd4x.startPeriodicMeasurement();
#endif

#ifdef FIRST_CONFIGURE
    storeEeprom_int32(UPDATE_FREQUENCY_ID, 60000);
#ifdef USE_SHT31
    storeEeprom_int32(TEMPERATURE_ADJUST_ID, 0);
    storeEeprom_int32(HUMIDITY_ADJUST_ID, 0);
#endif
#ifdef USE_SCD40
    storeEeprom_int32(CO2_ADJUST_ID, 0);
#endif
#ifdef USE_RELAY
    storeEeprom(AUTONOMOUS_ID, true);
#ifdef USE_SHT31
    storeEeprom_int32(MAX_HUMIDITY_ID, toUint32(0.0f));
    storeEeprom_int32(MIN_HUMIDITY_ID, toUint32(0.0f));
#endif
#ifdef USE_SCD40
    storeEeprom_int32(MAX_CO2_ID, 800);
    storeEeprom_int32(MIN_CO2_ID, 500);
#endif
#endif
#endif

    updateFrequency = readEeprom_int32(UPDATE_FREQUENCY_ID);
#ifdef USE_SHT31
    tempAdjust = fromUint32(readEeprom_int32(TEMPERATURE_ADJUST_ID));
    humAdjust = fromUint32(readEeprom_int32(HUMIDITY_ADJUST_ID));
#endif
#ifdef USE_SCD40
    co2Adjust = readEeprom_int32(CO2_ADJUST_ID);
#endif
#ifdef USE_RELAY
    autonomous = readEeprom(AUTONOMOUS_ID);
#ifdef USE_SHT31
    maxHumidity = fromUint32(readEeprom_int32(MAX_HUMIDITY_ID));
    minHumidity = fromUint32(readEeprom_int32(MIN_HUMIDITY_ID));
#endif
#ifdef USE_SCD40
    maxCo2 = readEeprom_int32(MAX_CO2_ID);
    minCo2 = readEeprom_int32(MIN_CO2_ID);
#endif
#endif

    Serial.println(F("Init Transport"));
#ifdef USE_LCD
    display.println(F("Init Transport"));
    display.display();
#endif
}

void presentation() {
#ifdef USE_LCD
    display.println(F("Presenting"));
    display.display();
#endif
    Serial.println(F("Presenting"));
    sendSketchInfo(SKETCH_IDENTIFICATION, SKETCH_VERSION);

    present(UPDATE_FREQUENCY_ID, S_CUSTOM, F("[c] Updt Freq (uint32 ms)"));
    wait(250);
    
#ifdef USE_SHT31
    present(TEMP_ID, S_TEMP, F("[s] Temperature (°C)"));
    wait(250);
    present(HUM_ID, S_HUM, F("[s] Humidity (%)"));
    wait(250);
    present(TEMPERATURE_ADJUST_ID, S_TEMP, F("[c] Temp Adj (float °C)"));
    wait(250);
    present(HUMIDITY_ADJUST_ID, S_HUM, F("[c] Hum Adj (float %)"));
    wait(250);
#endif

#ifdef USE_SCD40
    present(CO2_ADJUST_ID, S_CUSTOM, F("[c] CO2 Adj (int ppm)"));
    wait(250);
    present(CO2_ID, S_AIR_QUALITY, F("[s] CO2 (ppm)"));
    wait(250);
#endif

#ifdef USE_RELAY
    present(RELAY_ID, S_BINARY, F("[s] Relay Status (bool)"));
    wait(250);
    present(AUTONOMOUS_ID, S_BINARY, F("[c] Autonomous"));
    wait(250);
#ifdef USE_SHT31
    present(MAX_HUMIDITY_ID, S_HUM, F("[c] Max Humid (float °C)"));
    wait(250);
    present(MIN_HUMIDITY_ID, S_HUM, F("[c] Min Humid (float °C)"));
    wait(250);
#endif
#ifdef USE_SCD40
    present(MAX_CO2_ID, S_AIR_QUALITY, F("[c] Max CO2 (ppm)"));
    wait(250);
    present(MIN_CO2_ID, S_AIR_QUALITY, F("[c] Min CO2 (ppm)"));
    wait(250);
#endif
#endif

}

void doSensors(bool sendUpdate) {

#ifdef USE_LCD
    display.clearDisplay();
    display.setCursor(0, 0);
#endif

#ifdef USE_SHT31
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
#ifdef USE_LCD
    display.print(t, 1);
    display.println(F(" C"));
    display.print(h, 1);
    display.println(F(" %"));
#endif
    if (sendUpdate) {
        send(msgTemp.set(t, 1));
        wait(250);
        send(msgHum.set(h, 1));
        wait(250);
    }
#endif

#ifdef USE_SCD40
    bool dataReady = false;
    uint16_t co2Concentration = 0;
    float temperature = 0.0f;
    float relativeHumidity = 0.0f;

    error = scd4x.getDataReadyStatus(dataReady);
    while (error == NO_ERROR && !dataReady) {
        wait(100);
        scd4x.getDataReadyStatus(dataReady);
    }
    error = scd4x.readMeasurement(co2Concentration, temperature, relativeHumidity);
    if (error == NO_ERROR) {
        co2Concentration += co2Adjust;
        display.print((co2Concentration));
        display.println(F(" ppm"));

        if (sendUpdate) {
            send(msgCO2.set(co2Concentration));
            wait(250);
        }
    }

#endif

// #ifdef USE_RELAY
//   if (sendUpdate && autonomous) {
//     if (maxHumidity > 0 && minHumidity > 0) {
//       if ((h < minHumidity && relayStatus == RELAY_OFF) || (justOn && h < maxHumidity)) {
//       #ifdef MY_DEBUG
//         Serial.println(F("Turning on"));
//         Serial.println(minHumidity);
// #endif
//         justOn = false;
//         relayStatus = RELAY_ON;
//         digitalWrite(RELAY_PIN, relayStatus);
//         send(relay.set(relayStatus == RELAY_ON));
//       } else if (h > maxHumidity && relayStatus == RELAY_ON) {
//       #ifdef MY_DEBUG
//         Serial.println(F("Turning off"));
//         Serial.println(maxHumidity);
//       #endif
//         relayStatus = RELAY_OFF;
//         digitalWrite(RELAY_PIN, relayStatus);
//         send(relay.set(relayStatus == RELAY_ON));
//       }
//     } else if (minCo2 > 0 && maxCo2 > 0) {
//     #ifdef USE_ENS160
//       if ((ens160.geteCO2() > maxCo2 && relayStatus == RELAY_OFF) || (justOn && ens160.geteCO2() > minCo2)) {
// #ifdef MY_DEBUG
//         Serial.println(F("Turning on"));
//         Serial.println(maxCo2);
//       #endif
//         justOn = false;
//         relayStatus = RELAY_ON;
//         digitalWrite(RELAY_PIN, relayStatus);
//         send(relay.set(relayStatus == RELAY_ON));
//       } else if (ens160.geteCO2() < minCo2 && relayStatus == RELAY_ON) {
//       #ifdef MY_DEBUG
//         Serial.println(F("Turning off"));
//         Serial.println(minCo2);
//       #endif
//         relayStatus = RELAY_OFF;
//         digitalWrite(RELAY_PIN, relayStatus);
//         send(relay.set(relayStatus == RELAY_ON));
//       }
//     #endif
//     }
//   }

// #ifdef USE_LCD
//   if (relayStatus == RELAY_ON) {
//     lcd.setCursor(14, 1);
//     lcd.print(F("On"));
//   } else {
//     lcd.setCursor(13, 1);
//     lcd.print(F("Off"));
//   }
// #endif
// #endif

#ifdef USE_LCD
    if (!isTransportReady()) {
        // wait(UPDATE_SCREEN / 2);
        // display.clearDisplay();
        // display.setCursor(0, 0);
        display.println(F("Transport error"));
    }
    display.display();
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
    wait(10);
}