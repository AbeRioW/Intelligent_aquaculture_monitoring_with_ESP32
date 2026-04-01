
#include <Wire.h>
#include "SSD1306.h"
#include "DS18B20.h"

SSD1306 display(0x3c, 21, 22);

#define TURBIDITY_PIN 35
#define PH_PIN 34

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

int readPH() {
  return analogRead(PH_PIN);
}

void loop() {
  float temperature = readTemperature();
  int turbidity = readTurbidity();
  int phValue = readPH();
  
  display.clear();
  display.setFont(ArialMT_Plain_10);
  display.drawString(0, 0, "Temperature: ");
  if (temperature == -127.0) {
    display.drawString(80, 0, "Sensor not found");
  } else {
    display.drawString(80, 0, String(temperature) + " °C");
  }
  
  display.drawString(0, 16, "Turbidity: ");
  display.drawString(80, 16, String(turbidity) + " ADC");
  
  display.drawString(0, 32, "PH Value: ");
  display.drawString(80, 32, String(phValue) + " ADC");
  
  display.display();
  delay(1000);
}
