#include <Arduino.h>

void setup() {
    pinMode(PIN7, INPUT_PULLUP);  // enable internal pull-up
    Serial.begin(115200);
}

int buttonStatus = 0;
void loop() {
  int pinValue = digitalRead(PIN7);
  delay(10); // quick and dirty debounce filter
  if (buttonStatus != pinValue) {
    buttonStatus = pinValue;
    Serial.println(buttonStatus);
  }
}