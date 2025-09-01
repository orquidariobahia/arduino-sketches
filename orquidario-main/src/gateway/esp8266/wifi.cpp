#define SKETCH_NAME "ESP8266 Wi-Fi Gateway"
#define SKETCH_VERSION "v1.0"
#define SKETCH_DESCRIPTION "Laboratorio 2"

// Enable debug prints to serial monitor
// #define MY_DEBUG

// Use a bit lower baudrate for serial prints on ESP8266 than default in MyConfig.h
#define MY_BAUD_RATE 9600

// Enables and select radio type (if attached)
#define MY_RADIO_RF24
#define MY_RF24_DATARATE RF24_1MBPS
//#define MY_RADIO_RFM69
//#define MY_RADIO_RFM95

#define MY_GATEWAY_ESP8266

#define MY_WIFI_SSID "NovoOrquidario"
#define MY_WIFI_PASSWORD "orquidea"

// Enable UDP communication
//#define MY_USE_UDP  // If using UDP you need to set MY_CONTROLLER_IP_ADDRESS or MY_CONTROLLER_URL_ADDRESS below

// Set the hostname for the WiFi Client. This is the hostname
// it will pass to the DHCP server if not static.
#define MY_HOSTNAME "ESP8266_GW_Laboratorio"

// Enable MY_IP_ADDRESS here if you want a static ip address (no DHCP)
// #define MY_IP_ADDRESS 192,168,15,248

// If using static ip you can define Gateway and Subnet address as well
// #define MY_IP_GATEWAY_ADDRESS 192,168,15,1
// #define MY_IP_SUBNET_ADDRESS 255,255,255,0

// The port to keep open on node server mode
#define MY_PORT 5003

// How many clients should be able to connect to this gateway (default 1)
#define MY_GATEWAY_MAX_CLIENTS 2

// Controller ip address. Enables client mode (default is "server" mode).
// Also enable this if MY_USE_UDP is used and you want sensor data sent somewhere.
//#define MY_CONTROLLER_IP_ADDRESS 192, 168, 178, 68
//#define MY_CONTROLLER_URL_ADDRESS "my.controller.org"

// Enable inclusion mode
//#define MY_INCLUSION_MODE_FEATURE

// Enable Inclusion mode button on gateway
//#define MY_INCLUSION_BUTTON_FEATURE
// Set inclusion mode duration (in seconds)
//#define MY_INCLUSION_MODE_DURATION 60
// Digital pin used for inclusion mode button
//#define MY_INCLUSION_MODE_BUTTON_PIN D1

// Set blinking period
#define MY_DEFAULT_LED_BLINK_PERIOD 300

// Flash leds on rx/tx/err
// Led pins used if blinking feature is enabled above
#define MY_DEFAULT_ERR_LED_PIN LED_BUILTIN  // Error led pin
#define MY_DEFAULT_RX_LED_PIN  LED_BUILTIN  // Receive led pin
#define MY_DEFAULT_TX_LED_PIN  LED_BUILTIN  // the PCB, on board LED

#include <Arduino.h>
#include <MySensors.h>
#include <IPAddress.h>
// ESP8266WiFi.h should be automatically included by MySensors

// Global variables for connection monitoring
unsigned long lastControllerHeartbeat = 0;
unsigned long lastMessageReceived = 0;
unsigned long connectionCheckInterval = 90000; // Check every 90 seconds
bool controllerConnected = false;

bool hasConnectedClients() {
    return (millis() - lastMessageReceived) < 60000; // Activity in last minute
}

bool isControllerConnected() {
    bool recentActivity = (millis() - lastMessageReceived) < 300000; // Activity in last 5 minutes
    
    return /*WiFi.isNetworkConnected() &&*/ isTransportReady() & (hasConnectedClients() || recentActivity);
}

#ifdef MY_DEBUG
void printConnectionStatus() {
    Serial.println(F("=== Connection Status ==="));    
    Serial.print(F("Last Message: "));
    Serial.print((millis() - lastMessageReceived) / 1000);
    Serial.println(F(" seconds ago"));
    
    Serial.print(F("Controller Connected: "));
    Serial.println(isControllerConnected() ? F("YES") : F("NO"));
    Serial.println(F("========================"));
}
#endif

void before() {
#ifndef MY_DEBUG
	Serial.begin(MY_BAUD_RATE);
#endif
	wait(250);
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

	Serial.println();
	Serial.println(F("== IP Configuration =="));
	Serial.println();

	#ifdef MY_IP_ADDRESS
	Serial.print(F("Static IP Address: "));
	IPAddress(MY_IP_ADDRESS).printTo(Serial);
	Serial.println();

	Serial.print(F("Gateway IP Address: "));
	IPAddress(MY_IP_GATEWAY_ADDRESS).printTo(Serial);
	Serial.println();

	Serial.print(F("Subnet Mask: "));
	IPAddress(MY_IP_SUBNET_ADDRESS).printTo(Serial);
	Serial.println();
	#else
	Serial.print(F("DHCP IP Address: "));
	IPAddress ip = WiFi.localIP();
	ip.printTo(Serial);
	Serial.println();

	Serial.print(F("Gateway IP Address: "));
	IPAddress gw = WiFi.gatewayIP();
	gw.printTo(Serial);
	Serial.println();

	Serial.print(F("Subnet Mask: "));
	IPAddress subnet = WiFi.subnetMask();
	subnet.printTo(Serial);
	Serial.println();	
	#endif

	Serial.print(F("Port: "));
	Serial.println(MY_PORT);

	Serial.println();
	Serial.println(F("== Configuration =="));
	Serial.println();
	
	Serial.print(F("Hostname: "));
	Serial.println(MY_HOSTNAME);

	Serial.print(F("MAC Address: "));
	Serial.println(WiFi.macAddress());
	
	Serial.print(F("Gateway Max Clients: "));
	Serial.println(MY_GATEWAY_MAX_CLIENTS);

	Serial.println();
	Serial.println(F("== WiFi Configuration =="));
	Serial.println();
	Serial.print(F("SSID: "));
	Serial.println(MY_WIFI_SSID);
	Serial.print(F("Password: "));
	Serial.println(MY_WIFI_PASSWORD);

	Serial.println(F("-----------------------------------------------"));

	pinMode(LED_BUILTIN, OUTPUT);

}

void setup()
{
	lastMessageReceived = millis();
}

void loop()
{
	static unsigned long lastConnectionCheck = 0;
	
	if (millis() - lastConnectionCheck > connectionCheckInterval) {
		lastConnectionCheck = millis();
		
		#ifdef MY_DEBUG
		printConnectionStatus();
		#endif
		
		if (millis() - lastMessageReceived > 600000) {
			// trigger device restart
			ESP.restart();
		}
	}
}

void receive(const MyMessage &message) {
	lastMessageReceived = millis();
}
