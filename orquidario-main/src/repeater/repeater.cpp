#define MY_NODE_ID 10
#define SKETCH_NAME "Repeater Node"
#define SKETCH_VERSION "v1.0"
#define SKETCH_DESCRIPTION "Em cima do container"

// Enable debug prints to serial monitor
#define MY_DEBUG

// Enable and select radio type attached
#define MY_RADIO_RF24
// #define MY_DEBUG
#define MY_REPEATER_FEATURE
// #define MY_TRANSPORT_WAIT_READY_MS 10000
// #define MY_PASSIVE_NODE
// #define MY_TRANSPORT_UPLINK_CHECK_DISABLED
#define MY_TRANSPORT_MAX_TSM_FAILURES 100
// #define MY_TRANSPORT_TIMEOUT_FAILURE_STATE_MS		(10*1000ul) // 10 seconds
#define MY_RF24_DATARATE RF24_1MBPS

#include <MySensors.h>

void before() {
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
  Serial.println(F("-----------------------------------------------"));}

void setup()
{

}

void presentation()
{
	//Send the sensor node sketch version information to the gateway
	sendSketchInfo(SKETCH_NAME, SKETCH_VERSION);
}

void loop()
{
}
