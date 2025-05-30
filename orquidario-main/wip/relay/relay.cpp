// Definitions

#define MY_NODE_ID 53
#define SKETCH_NAME "Relay"
#define SKETCH_VERSION "v0.1"
#define SKETCH_DESCRIPTION "Exaustor do Container"
#define CO2_WINDOW_SIZE 6

#define MY_RADIO_RF24
// #define MY_DEBUG
// #define MY_REPEATER_FEATURE
#define MY_TRANSPORT_WAIT_READY_MS 60000
// #define MY_PASSIVE_NODE

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

#define RELAY_ID 60

// Configurations (120-159)

#define AUTONOMOUS_ID 120
#define UPDATE_FREQUENCY_ID 121
// #define TEMPERATURE_ADJUST_ID 122
// #define HUMIDITY_ADJUST_ID 123
// #define MIN_HUMIDITY_ID 124
// #define MAX_HUMIDITY_ID 125
#define MIN_CO2_ID 126
#define MAX_CO2_ID 127
#define REMOTE_NODE_ID 128
#define REMOTE_CO2_ID 129
#define REMOTE_NODE_ID_2 130
#define REMOTE_CO2_ID_2 131

// Libraries

#include <Serialization.h>
#include <MySensors.h>

// Global variables

uint32_t updateFrequency = 60000;
int relayStatus = RELAY_OFF;
uint8_t remoteNodeId = 0;
uint8_t remoteCo2Id = 0;
uint8_t remoteNodeId2 = 0;
uint8_t remoteCo2Id2 = 0;
uint32_t minCo2 = 0;
uint32_t maxCo2 = 0;
bool autonomous = true;
int loopCount = 0;
uint16_t co2List[CO2_WINDOW_SIZE];
int nextIndex = 0;

// Messages

MyMessage relay(RELAY_ID, V_STATUS);

void setup() {
    send(relay.set(relayStatus == RELAY_ON));
    send(MyMessage(UPDATE_FREQUENCY_ID, V_CUSTOM).set(updateFrequency));
    send(MyMessage(REMOTE_NODE_ID, V_CUSTOM).set(remoteNodeId));
    send(MyMessage(REMOTE_CO2_ID_2, V_CUSTOM).set(remoteCo2Id2));
    send(MyMessage(REMOTE_NODE_ID_2, V_CUSTOM).set(remoteNodeId2));
    send(MyMessage(REMOTE_CO2_ID, V_CUSTOM).set(remoteCo2Id));
    send(MyMessage(MIN_CO2_ID, V_LEVEL).set(minCo2));
    send(MyMessage(MAX_CO2_ID, V_LEVEL).set(maxCo2));
    send(MyMessage(AUTONOMOUS_ID, V_STATUS).set(autonomous));
}

void before() {
    pinMode(RELAY_PIN, OUTPUT);
    digitalWrite(RELAY_PIN, RELAY_OFF);

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

    // storeEeprom_int32(UPDATE_FREQUENCY_ID, 30000);
    // storeEeprom(REMOTE_NODE_ID, 67);
    // storeEeprom_int32(MIN_CO2_ID, 500);
    // storeEeprom_int32(MAX_CO2_ID, 800);
    // storeEeprom(AUTONOMOUS_ID, true);
    // storeEeprom(REMOTE_CO2_ID, 3);
    // storeEeprom(REMOTE_NODE_ID, 52);
    // storeEeprom(REMOTE_CO2_ID_2, 3);
    // storeEeprom(REMOTE_NODE_ID_2, 54);

    updateFrequency = readEeprom_int32(UPDATE_FREQUENCY_ID);
    remoteNodeId = readEeprom(REMOTE_NODE_ID);
    remoteCo2Id = readEeprom(REMOTE_CO2_ID);
    remoteNodeId2 = readEeprom(REMOTE_NODE_ID_2);
    remoteCo2Id2 = readEeprom(REMOTE_CO2_ID_2);
    minCo2 = readEeprom_int32(MIN_CO2_ID);
    maxCo2 = readEeprom_int32(MAX_CO2_ID);
    autonomous = readEeprom(AUTONOMOUS_ID);

    for(int i = 0; i < CO2_WINDOW_SIZE; i++) {
        co2List[i] = 0;
    }
}

void presentation() {
    sendSketchInfo(SKETCH_NAME, SKETCH_VERSION);

    present(RELAY_ID, S_BINARY, "[s] Relay Status (bool)");
    present(UPDATE_FREQUENCY_ID, S_CUSTOM, "[c] Updt Freq (uint32 ms)");
    present(REMOTE_NODE_ID, S_CUSTOM, "[c] Remote Node ID (uint8)");
    present(MIN_CO2_ID, S_AIR_QUALITY, "[c] Min CO2 (int)");
    present(MAX_CO2_ID, S_AIR_QUALITY, "[c] Max CO2 (int)");
    present(AUTONOMOUS_ID, S_BINARY, "[c] Autonomous");
}

void handleMessage(uint16_t co2) {
    if (co2 == 0) {
        return;
    }
    co2List[nextIndex] = co2;
    nextIndex = (nextIndex + 1) % CO2_WINDOW_SIZE;
    uint32_t average = 0;
    byte avgAmount = 0;
    for(int i = 0; i < CO2_WINDOW_SIZE; i++) {
        if (co2List[i] != 0) {
            average += co2List[i];
            avgAmount++;
        }
    }
    if (avgAmount > 0) {
        average = average / avgAmount;
    }
    else {
        average = co2;
    }
    if ((average > maxCo2 && relayStatus == RELAY_OFF)) {
#ifdef MY_DEBUG
        Serial.println("Turning on");
        Serial.println(co2);
#endif
        relayStatus = RELAY_ON;
        digitalWrite(RELAY_PIN, relayStatus);
        send(relay.set(relayStatus == RELAY_ON));
    }
    else if (average < minCo2 && relayStatus == RELAY_ON) {
#ifdef MY_DEBUG
        Serial.println("Turning off");
        Serial.println(co2);
#endif
        relayStatus = RELAY_OFF;
        digitalWrite(RELAY_PIN, relayStatus);
        send(relay.set(relayStatus == RELAY_ON));
    }
}

void loop() {

    if (autonomous) {
        if (loopCount == 0) {
            request(remoteCo2Id, V_LEVEL, remoteNodeId);
        }
        else {
            if (remoteNodeId2 != 0) {
                request(remoteCo2Id2, V_LEVEL, remoteNodeId2);
            }
            loopCount = -1;
        }
    }

    loopCount++;

    wait(updateFrequency);
    // wait(5000);
}

void receive(const MyMessage& message) {
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
    case RELAY_ID:
        relayStatus = message.getBool() ? RELAY_ON : RELAY_OFF;
        digitalWrite(RELAY_PIN, relayStatus);
        break;
    case AUTONOMOUS_ID:
        autonomous = message.getBool();
        storeEeprom(AUTONOMOUS_ID, autonomous);
        break;
    case MIN_CO2_ID:
        minCo2 = message.getInt();
        storeEeprom_int32(MIN_CO2_ID, minCo2);
        break;
    case MAX_CO2_ID:
        maxCo2 = message.getInt();
        storeEeprom_int32(MAX_CO2_ID, maxCo2);
        break;
    case REMOTE_NODE_ID:
        remoteNodeId = message.getByte();
        storeEeprom(REMOTE_NODE_ID, remoteNodeId);
        break;
    }

    if (message.getDestination() == MY_NODE_ID && message.getSender() == remoteNodeId && message.getType() == V_LEVEL && message.getSensor() == remoteCo2Id) {
        handleMessage(message.getUInt());
    }
}