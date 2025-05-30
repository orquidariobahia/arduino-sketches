#define MY_NODE_ID 52
#define SKETCH_NAME "DS18B20 / Relay / Screen"
#define SKETCH_VERSION "v1.0"
#define SKETCH_DESCRIPTION "Monitoramento da Pasteurização"

#define MY_RADIO_RF24
#define MY_RF24_DATARATE RF24_1MBPS
// #define MY_DEBUG
// #define MY_REPEATER_FEATURE
// #define MY_TRANSPORT_WAIT_READY_MS 10000
// #define MY_PASSIVE_NODE
// #define MY_TRANSPORT_UPLINK_CHECK_DISABLED
// #define MY_TRANSPORT_MAX_TSM_FAILURES 100
// #define MY_TRANSPORT_TIMEOUT_FAILURE_STATE_MS		(10*1000ul) // 10 seconds

// DS18B20 Setup

#define COMPARE_TEMP 1 // Send temperature only if changed? 1 = Yes 0 = No
#define ONE_WIRE_BUS 3 // Pin where dallase sensor is connected 
#define MAX_ATTACHED_DS18B20 4

// LCD Setup

#define COLUMS           16   //LCD columns
#define ROWS             2    //LCD rows
#define LCD_SPACE_SYMBOL 0x20 //space symbol from LCD ROM, see p.9 of GDM2004D datasheet
// #define LCD_SCROLL_SPEED 350  //scroll speed in milliseconds
#define UPDATE_SCREEN 10000
#define DEGREE      0xDF

// Relay setup

#define RELAY_PIN 7  // Arduino Digital I/O pin number for first relay (second on pin+1 etc)
#define RELAY_ON 0   // GPIO value to write to turn on attached relay
#define RELAY_OFF 1  // GPIO value to write to turn off attached relay

// Sensors ID (0-39)

#define TEMP_ID 1

// Actuators ID (60-99)

#define RELAY_ID 60

// Configurations (120-159)

#define AUTONOMOUS_ID 120
#define UPDATE_FREQUENCY_ID 121
#define TEMPERATURE_ADJUST_ID 122

// Libraries

#include <MySensors.h>  
#include <DallasTemperature.h>
#include <OneWire.h>
#include <Serialization.h>
#include <LiquidCrystal_I2C.h>

// Global variables

LiquidCrystal_I2C lcd(PCF8574_ADDR_A21_A11_A01, 4, 5, 6, 16, 11, 12, 13, 14, POSITIVE);
OneWire oneWire(ONE_WIRE_BUS); // Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
DallasTemperature sensors(&oneWire); // Pass the oneWire reference to Dallas Temperature. 

uint32_t updateFrequency = 60000;
float tempAdjust = 0.0;
uint8_t loopCount = 0;
bool autonomous = false;
int relayStatus = RELAY_OFF;
bool justOn = true;
float lastTemperature[MAX_ATTACHED_DS18B20];
int numSensors = 0;

// Messages

MyMessage msg(TEMP_ID, V_TEMP);
MyMessage relay(RELAY_ID, V_STATUS);

void setup() {
    lcd.clear();
    lcd.home();
    lcd.print(F("Setting up"));
    Serial.println(F("Setting up"));
    sensors.setWaitForConversion(false);
    send(relay.set(relayStatus == RELAY_ON));
    send(MyMessage(AUTONOMOUS_ID, V_STATUS).set(autonomous));
    send(MyMessage(UPDATE_FREQUENCY_ID, V_TEXT).set(updateFrequency));
    send(MyMessage(TEMPERATURE_ADJUST_ID, V_TEMP).set((float)tempAdjust, 1));
}

void before() {
    if (lcd.begin(COLUMS, ROWS, LCD_5x8DOTS) != 1) //colums, rows, characters size
    {
        Serial.println(F("PCF8574 is not connected or lcd pins declaration is wrong. Only pins numbers: 4,5,6,16,11,12,13,14 are legal."));
    }
    lcd.clear();
    lcd.home();
    lcd.leftToRight();
    lcd.print(F("Booting up"));

    pinMode(RELAY_PIN, OUTPUT);
    digitalWrite(RELAY_PIN, RELAY_OFF);

    sensors.begin();

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

    storeEeprom_int32(UPDATE_FREQUENCY_ID, 60000);
    storeEeprom_int32(TEMPERATURE_ADJUST_ID, 0);
    storeEeprom(AUTONOMOUS_ID, true);

    updateFrequency = readEeprom_int32(UPDATE_FREQUENCY_ID);
    tempAdjust = fromUint32(readEeprom_int32(TEMPERATURE_ADJUST_ID));
    autonomous = readEeprom(AUTONOMOUS_ID);

    numSensors = sensors.getDeviceCount();

    lcd.clear();
    lcd.home();
    lcd.print(F("Init Transport"));
    Serial.println(F("Init Transport"));

}

void presentation() {
    lcd.clear();
    lcd.home();
    lcd.print(F("Presenting"));
    Serial.println(F("Presenting"));
    sendSketchInfo(SKETCH_NAME, SKETCH_VERSION);

    present(RELAY_ID, S_BINARY, "[s] Relay Status (bool)");
    wait(250);

    present(UPDATE_FREQUENCY_ID, S_CUSTOM, "[c] Updt Freq (uint32 ms)");
    wait(250);
    present(TEMPERATURE_ADJUST_ID, S_TEMP, "[c] Temp Adj (float °C)");
    wait(250);

    present(AUTONOMOUS_ID, S_BINARY, "[c] Autonomous");
    wait(250);

    // Present all sensors to controller
    for (int i = 0; i < numSensors && i < MAX_ATTACHED_DS18B20; i++) {
        present(i, S_TEMP, "[s] Temperature (°C)");
        wait(250);
    }
}

void doSensors(bool sendUpdate) {
    // Fetch temperatures from Dallas sensors
    sensors.requestTemperatures();

    // query conversion time and sleep until conversion completed
    int16_t conversionTime = sensors.millisToWaitForConversion(sensors.getResolution());
    // sleep() call can be replaced by wait() call if node need to process incoming messages (or if node is repeater)
    wait(conversionTime);

    lcd.clear();
    lcd.home();

    // Read temperatures and send them to controller 
    for (int i = 0; i < numSensors && i < MAX_ATTACHED_DS18B20; i++) {
        // Fetch and round temperature to one decimal
        float temperature = static_cast<float>(static_cast<int>((getControllerConfig().isMetric ? sensors.getTempCByIndex(i) : sensors.getTempFByIndex(i)) * 10.)) / 10.;

        switch (i)
        {
        case 0:
            lcd.setCursor(0, 0);
            break;
        case 1:
            lcd.setCursor(9, 0);
            break;
        case 2:
            lcd.setCursor(0, 1);
            break;
        case 3:
            lcd.setCursor(9, 1);
            break;
        }
        lcd.print(i+1);
        lcd.print(F(":"));
        lcd.print(temperature, 1);
        lcd.write(DEGREE);

        // Only send data if temperature has changed and no error
#if COMPARE_TEMP == 1
        if (sendUpdate && lastTemperature[i] != temperature && temperature != -127.00 && temperature != 85.00)
#else
        if (sendUpdate && temperature != -127.00 && temperature != 85.00)
#endif
        {
            // Send in the new temperature
            send(msg.setSensor(i).set(temperature, 1));
            // Save new temperatures for next compare
            lastTemperature[i] = temperature;
        }
    }
}

void loop()
{
#ifdef LCD_SCROLL_SPEED
    if (justOn || loopCount >= (updateFrequency / LCD_SCROLL_SPEED)) {
        loopCount = 0;
        doSensors(true);
    }
    loopCount++;;
    lcd.scrollDisplayLeft();
    wait(LCD_SCROLL_SPEED);
#elif UPDATE_SCREEN
    if (justOn || loopCount >= (updateFrequency / UPDATE_SCREEN)) {
        loopCount = 0;
        justOn = false;
        doSensors(true);
    }
    else {
        Serial.print(loopCount);
        Serial.print(F(">="));
        Serial.println(updateFrequency / UPDATE_SCREEN);
        doSensors(false);
    }
    loopCount++;
    wait(UPDATE_SCREEN);
#else
    doSensors(true);
    wait(updateFrequency);
#endif
}