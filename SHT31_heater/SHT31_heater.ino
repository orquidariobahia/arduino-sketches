//
//    FILE: SHT31_heater.ino
//  AUTHOR: Rob Tillaart
// PURPOSE: demo heater functions
//     URL: https://github.com/RobTillaart/SHT31


#include "Wire.h"
#include "SHT31.h"

#define SHT31_ADDRESS 0x44
#define TIMEOUT 150
#define REST 300
#define CYCLES 20

SHT31 sht(SHT31_ADDRESS);
uint16_t status;


void setup() {
  Serial.begin(115200);
  Serial.println(__FILE__);
  Serial.print("SHT31_LIB_VERSION: \t");
  Serial.println(SHT31_LIB_VERSION);

  Wire.begin();
  Wire.setClock(100000);
  sht.begin();

  status = sht.readStatus();
  printHeaterStatus(status);

  for (int i = 0; i < CYCLES; i++) {
    sht.setHeatTimeout(TIMEOUT);  //  heater timeout 30 seconds, just for demo.

    sht.heatOn();

    for (int k = 0; k < TIMEOUT && sht.isHeaterOn(); k = k + 10) {
      status = sht.readStatus();
      printHeaterStatus(status);
      sht.read();
      Serial.print("\t");
      Serial.print(sht.getTemperature());
      Serial.println(" C");
      delay(10000);
    }

    if (status & SHT31_STATUS_HEATER_ON) {
      sht.heatOff();
      Serial.println("Turning off heater");
    } else {
      sht.heatOff();
      Serial.println("Error?");
    }

    Serial.print("Cooling off");
    for (int i = 0; i < REST; i = i + 10) {
      Serial.print(".");
      delay(10000);
    }

    Serial.println("Device cooled off");
  }
  Serial.println("Cycle complete");
}

void loop() {
  //  forced switch off
}


void printHeaterStatus(uint16_t status) {
  Serial.print("\tHEATER: ");
  if (status & SHT31_STATUS_HEATER_ON) {
    Serial.print("ON");
  } else {
    Serial.print("OFF");
  }
}


//  -- END OF FILE --
