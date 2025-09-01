#define MY_NODE_ID 56
#define SKETCH_NAME "Button / DS18B20 / Relay / Screen"
#define SKETCH_IDENTIFICATION "mysensors_56_temp_freezer"
#define SKETCH_VERSION "v1.2"
#define SKETCH_DESCRIPTION "Monitoramento de Todos os Freezers"

#define MY_RADIO_RF24
#define MY_DEBUG
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

// DS18B20 Setup

#define COMPARE_TEMP // Send temperature only if changed? 1 = Yes 0 = No
#define ONE_WIRE_BUS 3 // Pin where dallase sensor is connected 
#define MAX_ATTACHED_DS18B20 4

// LCD Setup

#ifdef USE_LCD
#define COLUMS           16   //LCD columns
#define ROWS             2    //LCD rows
#define LCD_SPACE_SYMBOL 0x20 //space symbol from LCD ROM, see p.9 of GDM2004D datasheet
// #define LCD_SCROLL_SPEED 350  //scroll speed in milliseconds
#define UPDATE_SCREEN 10000
#define DEGREE      0xDF
#endif

// Relay setup

#ifdef USE_RELAY
#define RELAY_PIN 7  // Arduino Digital I/O pin number for first relay (second on pin+1 etc)
#define RELAY_ON 0   // GPIO value to write to turn on attached relay
#define RELAY_OFF 1  // GPIO value to write to turn off attached relay
#endif

// Button

#define BUTTON_DELAY 10
#ifdef USE_BUTTON
#define BUTTON_PIN 6
#endif

// Sensors ID (0-39)

// #define TEMP_ID 1
// First x sensors will be also occupied by the temp sensors

// Actuators ID (60-99)

#ifdef USE_RELAY
#define RELAY_ID 60
#endif

// Configurations (120-159)

#ifdef USE_RELAY
#define AUTONOMOUS_ID 120
#endif
#define UPDATE_FREQUENCY_ID 121
#define TEMPERATURE_ADJUST_ID 122

// Libraries

#include <MySensors.h>  
#include <Serialization.h>

#include <DallasTemperature.h>
#include <OneWire.h>

#ifdef USE_LCD
#include <LiquidCrystal_I2C.h>
#endif

// Global variables

#ifdef USE_LCD
LiquidCrystal_I2C lcd(PCF8574_ADDR_A21_A11_A01, 4, 5, 6, 16, 11, 12, 13, 14, POSITIVE);
unsigned long updateScreenTime = 0;
#endif

#ifdef USE_RELAY
int relayStatus = RELAY_OFF;
#endif

OneWire oneWire(ONE_WIRE_BUS); // Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
DallasTemperature sensors(&oneWire); // Pass the oneWire reference to Dallas Temperature. 

uint32_t updateFrequency = 60000;
float tempAdjust = 0.0;
#ifdef USE_RELAY
bool autonomous = false;
#endif
bool justOn = true;
float lastTemperature[MAX_ATTACHED_DS18B20];
int numSensors = 0;
unsigned long updateSensorsTime = 0;

// Messages

MyMessage msg(0, V_TEMP);
#ifdef USE_RELAY
MyMessage relay(RELAY_ID, V_STATUS);
#endif

void setup() {
#ifdef USE_LCD
    lcd.clear();
    lcd.home();
    lcd.print(F("Setting up"));
#endif
    Serial.println(F("Setting up"));
    sensors.setWaitForConversion(false);
#ifdef USE_RELAY
    send(relay.set(relayStatus == RELAY_ON));
    wait(250);
    send(MyMessage(AUTONOMOUS_ID, V_STATUS).set(autonomous));
    wait(250);
#endif    
    send(MyMessage(UPDATE_FREQUENCY_ID, V_VAR1).set(updateFrequency));
    wait(250);
    send(MyMessage(TEMPERATURE_ADJUST_ID, V_TEMP).set((float)tempAdjust, 1));
    wait(250);
}

void before() {
#ifdef USE_LCD
    if (lcd.begin(COLUMS, ROWS, LCD_5x8DOTS) != 1) //colums, rows, characters size
    {
        Serial.println(F("PCF8574 is not connected or lcd pins declaration is wrong. Only pins numbers: 4,5,6,16,11,12,13,14 are legal."));
    }
    lcd.clear();
    lcd.home();
    lcd.leftToRight();
    lcd.print(F("Booting up"));
#endif

#ifdef USE_RELAY
    pinMode(RELAY_PIN, OUTPUT);
    digitalWrite(RELAY_PIN, RELAY_OFF);
#endif

#ifdef USE_BUTTON
    pinMode(BUTTON_PIN, INPUT_PULLUP);
#endif

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

//     storeEeprom_int32(UPDATE_FREQUENCY_ID, 60000);
//    storeEeprom_int32(TEMPERATURE_ADJUST_ID, 0);
// #ifdef USE_RELAY
//     storeEeprom(AUTONOMOUS_ID, true);
// #endif

    updateFrequency = readEeprom_int32(UPDATE_FREQUENCY_ID);
    tempAdjust = fromUint32(readEeprom_int32(TEMPERATURE_ADJUST_ID));
#ifdef USE_RELAY
    autonomous = readEeprom(AUTONOMOUS_ID);
#endif

    numSensors = sensors.getDeviceCount();
#ifdef MY_DEBUG
    Serial.print(F("Found "));
    Serial.print(numSensors);
    Serial.println(F(" DS18B20 sensors."));
#endif

#ifdef USE_LCD
    lcd.clear();
    lcd.home();
    lcd.print(F("Init Transport"));
    Serial.println(F("Init Transport"));
#endif
}

void presentation() {
#ifdef USE_LCD
    lcd.clear();
    lcd.home();
    lcd.print(F("Presenting"));
#endif
    Serial.println(F("Presenting"));
    sendSketchInfo(SKETCH_IDENTIFICATION, SKETCH_VERSION);

#ifdef USE_RELAY
    present(RELAY_ID, S_BINARY, "[s] Relay Status (bool)");
    wait(250);
#endif

    present(UPDATE_FREQUENCY_ID, S_CUSTOM, "[c] Updt Freq (uint32 ms)");
    wait(250);
    present(TEMPERATURE_ADJUST_ID, S_TEMP, "[c] Temp Adj (float °C)");
    wait(250);

#ifdef USE_RELAY
    present(AUTONOMOUS_ID, S_BINARY, "[c] Autonomous");
    wait(250);
#endif


// Present all sensors to controller
for (int i = 0; i < numSensors && i < MAX_ATTACHED_DS18B20; i++) {
        String sname = String("[s] Temperature ");
        sname.concat(i + 1);
        sname.concat(" °C");
        present(i, S_TEMP, sname.c_str());
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

#ifdef USE_LCD
    lcd.clear();
    lcd.home();
#endif

    // Read temperatures and send them to controller 
    for (int i = 0; i < numSensors && i < MAX_ATTACHED_DS18B20; i++) {
        // Fetch and round temperature to one decimal
        float temperature = static_cast<float>(static_cast<int>((getControllerConfig().isMetric || true? sensors.getTempCByIndex(i) : sensors.getTempFByIndex(i)) * 10.)) / 10.;

#ifdef USE_LCD
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

        lcd.print(i + 1);
        lcd.print(F(":"));
        lcd.print(temperature, 1);
        lcd.write(DEGREE);
#endif

        // Only send data if temperature has changed and no error
#ifdef COMPARE_TEMP
        if (sendUpdate && lastTemperature[i] != temperature && temperature != -127.00 && temperature != 85.00)
#else
        if (sendUpdate && temperature != -127.00 && temperature != 85.00)
#endif
        {
            // Send in the new temperature
            send(msg.setSensor(i).set(temperature, 1));
            // Save new temperatures for next compare
            lastTemperature[i] = temperature;

#ifdef MY_DEBUG
            Serial.print(F("Sensor "));
            Serial.print(i + 1);
            Serial.print(F(": "));
            Serial.print(temperature);
            Serial.println(F("C"));
#endif
        }
    }

#ifdef USE_LCD
    if (!isTransportReady()) {
        wait(UPDATE_SCREEN / 2);
        lcd.clear();
        lcd.print(F("Transport error"));
    }
#endif
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

#ifdef USE_BUTTON
    if (digitalRead(BUTTON_PIN) == LOW) {
        // Button Pressed
        // relayStatus = RELAY_ON;
        // digitalWrite(RELAY_PIN, RELAY_ON);
    }
    else {
        // Button Released
        // relayStatus = RELAY_OFF;
        // digitalWrite(RELAY_PIN, RELAY_OFF);
    }
#endif
    wait(BUTTON_DELAY);
}