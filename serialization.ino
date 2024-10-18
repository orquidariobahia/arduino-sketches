#define INT_MINUMUM_VALUE -32768
#define UINT8_MAX_VALUE 255

void storeEeprom(uint8_t pos, uint8_t value) {
  if (pos < 120 || pos > 159) {
    Serial.println("Out of bounds attempt to save");
    return;
  }

  int actualPos = (pos % 40) * 4;

  saveState(actualPos, (value));
}

void storeEeprom_int(uint8_t pos, int value) {
  if (pos < 120 || pos > 159) {
    Serial.println("Out of bounds attempt to save");
    Serial.print(EEPROM_LOCAL_CONFIG_ADDRESS);
    return;
  }

  int actualPos = (pos % 40) * 4;

  saveState(actualPos, (value >> 8));
  saveState(actualPos + 1, (value & 0xff));
}

void storeEeprom_int32(uint8_t pos, uint32_t value) {
  if (pos < 120 || pos > 159) {
    Serial.println("Out of bounds attempt to save");
    return;
  }

  int actualPos = (pos % 40) * 4;

  saveState(actualPos, (value >> 24));
  saveState(actualPos + 1, (value >> 16));
  saveState(actualPos + 2, (value >> 8));
  saveState(actualPos + 3, (value));
}

uint32_t readEeprom_int32(uint8_t pos) {
  if (pos < 120 || pos > 159) {
    Serial.println("Out of bounds attempt to save");
    return INT_MINUMUM_VALUE;
  } else {
    int actualPos = (pos % 40) * 4;
    uint32_t h1, h2, l1, l2;

    h1 = loadState(actualPos);
    h2 = loadState(actualPos + 1);
    l1 = loadState(actualPos + 2);
    l2 = loadState(actualPos + 3);
    h1 = h1 << 24;
    h2 = h2 << 16;
    l1 = l1 << 8;
    return (h1 | h2 | l1 | l2);
  }
}

int readEeprom_int(uint8_t pos) {
  if (pos < 120 || pos > 159) {
    Serial.println("Out of bounds attempt to save");
    return INT_MINUMUM_VALUE;
  } else {
    int actualPos = (pos % 40) * 4;
    int high, low;

    high = loadState(actualPos) << 8;
    low = loadState(actualPos + 1);
    return (high | low);
  }
}

uint8_t readEeprom(uint8_t pos) {
  if (pos < 120 || pos > 159) {
    Serial.println("Out of bounds attempt to save");
    return UINT8_MAX_VALUE;
  } else {
    int actualPos = (pos % 40) * 4;
    int high;

    high = loadState(actualPos);
    return (high);
  }
}

uint32_t toUint32(float f) {
  uint32_t u;
  memcpy(&u, &f, sizeof(f));
  return u;
}

float fromUint32(uint32_t i) {
  float f;
  memcpy(&f, &i, sizeof(i));
  return f;
}

// void storeState(const uint8_t pos, const uint8_t value) {
//   hwWriteConfig(EEPROM_LOCAL_CONFIG_ADDRESS + pos, value);
// }
// uint8_t readState(const uint8_t pos) {
//   return hwReadConfig(EEPROM_LOCAL_CONFIG_ADDRESS + pos);
// }
