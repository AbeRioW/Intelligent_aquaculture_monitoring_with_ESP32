
#include <Wire.h>
#include "SSD1306.h"
#include <OneWire.h>

SSD1306 display(0x3c, 21, 22);

#define ONE_WIRE_BUS 4
OneWire ds(ONE_WIRE_BUS);

#define TURBIDITY_PIN 35

void setup() {
  display.init();
  display.flipScreenVertically();
  
  display.clear();
  display.setFont(ArialMT_Plain_10);
  display.drawString(0, 0, "Initializing sensors...");
  display.display();
  delay(2000);
}

float readTemperature() {
  byte i;
  byte present = 0;
  byte data[12];
  byte addr[8];
  
  if (!ds.search(addr)) {
    ds.reset_search();
    return -127.0;
  }
  
  if (OneWire::crc8(addr, 7) != addr[7]) {
    return -127.0;
  }
  
  ds.reset();
  ds.select(addr);
  ds.write(0x44, 1);
  
  delay(1000);
  
  present = ds.reset();
  ds.select(addr);
  ds.write(0xBE);
  
  for (i = 0; i < 9; i++) {
    data[i] = ds.read();
  }
  
  int16_t raw = (data[1] << 8) | data[0];
  byte cfg = (data[4] & 0x60);
  if (cfg == 0x00) raw = raw << 3;
  else if (cfg == 0x20) raw = raw << 2;
  else if (cfg == 0x40) raw = raw << 1;
  
  return (float)raw / 16.0;
}

int readTurbidity() {
  return analogRead(TURBIDITY_PIN);
}

void loop() {
  float temperature = readTemperature();
  int turbidity = readTurbidity();
  
  display.clear();
  display.setFont(ArialMT_Plain_10);
  display.drawString(0, 0, "Temperature:");
  
  if (temperature == -127.0) {
    display.setFont(ArialMT_Plain_10);
    display.drawString(0, 12, "Sensor not found");
  } else {
    display.setFont(ArialMT_Plain_16);
    display.drawString(0, 12, String(temperature) + " °C");
  }
  
  display.setFont(ArialMT_Plain_10);
  display.drawString(0, 28, "Turbidity:");
  display.setFont(ArialMT_Plain_16);
  display.drawString(0, 40, String(turbidity) + " ADC");
  
  display.display();
  delay(1000);
  
  ds.reset_search();
}
