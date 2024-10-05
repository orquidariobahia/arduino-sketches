
#define MY_RADIO_RF24
#define MY_DEBUG
// #define MY_PASSIVE_NODE
#define MY_REPEATER_FEATURE
#define MY_NODE_ID 76
//#define MY_TRANSPORT_WAIT_READY_MS 1

#include <MySensors.h>

#define RELAY_PIN 4  // Arduino Digital I/O pin number for first relay (second on pin+1 etc)
#define RELAY_ON 1   // GPIO value to write to turn on attached relay
#define RELAY_OFF 0  // GPIO value to write to turn off attached relay

#define RELAY_ID 0
#define TIMER_ON_ID 1
#define TIMER_OFF_ID 2
#define AUTONOMOUS_ID 3

MyMessage relay(RELAY_ID, V_STATUS);

bool autonomous = false;
bool justOn = true;
int relayStatus = RELAY_OFF;
int timerOn = -1;
int timerOff = -1;

void setup() {

  digitalWrite(RELAY_PIN, RELAY_OFF);

  send(relay.set(relayStatus == RELAY_ON));
  send(MyMessage(AUTONOMOUS_ID, V_STATUS).set(autonomous));
  send(MyMessage(TIMER_ON_ID, V_CUSTOM).set(timerOn));
  send(MyMessage(TIMER_OFF_ID, V_CUSTOM).set(timerOff));
}

void before() {
  pinMode(RELAY_PIN, OUTPUT);

  Serial.begin(115200);

  digitalWrite(RELAY_PIN, RELAY_OFF);

  autonomous = loadState(AUTONOMOUS_ID);
  timerOn = loadState(TIMER_ON_ID);
  timerOff = loadState(TIMER_OFF_ID);
}

void presentation() {
  // Send the sketch version information to the gateway and controller
  sendSketchInfo("Relay_and_Timer", "1.0");

  // Register all sensors to gw (they will be created as child devices)
  present(RELAY_ID, S_BINARY, "Relay Status");
  present(TIMER_ON_ID, S_CUSTOM, "Timer On (minutes < 256)");
  present(TIMER_OFF_ID, S_CUSTOM, "Timer Off (minutes< 256)");
  present(AUTONOMOUS_ID, S_BINARY, "Autonomous");
}

void loop() {
  if (justOn) {
    justOn = false;
    wait(10000);
  } else {
    if (autonomous) {
      if (relayStatus == RELAY_OFF) {
  #ifdef MY_DEBUG
        Serial.print("Turning on - ");
        Serial.println(timerOn);
  #endif
        relayStatus = RELAY_ON;
        digitalWrite(RELAY_PIN, relayStatus);
        send(relay.set(relayStatus == RELAY_ON));
        uint32_t t = timerOn * 60UL * 1000UL;
        Serial.println(t);
        wait(t);
      } else if (relayStatus == RELAY_ON) {
  #ifdef MY_DEBUG
        Serial.print("Turning off - ");
        Serial.println(timerOff);
  #endif
        relayStatus = RELAY_OFF;
        digitalWrite(RELAY_PIN, relayStatus);
        send(relay.set(relayStatus == RELAY_ON));
        int32_t t = timerOff * 60UL * 1000UL;
        Serial.println(t);
        wait(t);
      }
    }
  }
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
    case TIMER_ON_ID:
      timerOn = message.getInt();
      saveState(TIMER_ON_ID, timerOn);
      break;
    case TIMER_OFF_ID:
      timerOff = message.getInt();
      saveState(TIMER_OFF_ID, timerOff);
      break;
  }
}
