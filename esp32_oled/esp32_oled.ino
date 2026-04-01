
#include <Wire.h>
#include "SSD1306.h"
#include "DS18B20.h"

SSD1306 display(0x3c, 21, 22);

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
  return ds18b20_read_temperature();
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
}
