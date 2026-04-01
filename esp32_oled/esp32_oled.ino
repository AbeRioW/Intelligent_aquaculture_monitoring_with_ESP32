
#include <Wire.h>
#include "SSD1306.h"
#include "DS18B20.h"
#include <Preferences.h>

Preferences preferences;

SSD1306 display(0x3c, 21, 22);

#define TURBIDITY_PIN 35
#define PH_PIN 34
#define KEY1_PIN 25
#define KEY2_PIN 26
#define KEY3_PIN 27

// 阈值变量
int tempThreshold = 25;
int turbidityThreshold = 2000;
int phThreshold = 2000;

// 设置模式标志
bool settingMode = false;
int settingIndex = 0; // 0: 温度, 1: 浑浊度, 2: PH值

// 按键状态变量
bool key1State = HIGH;
bool key1LastState = HIGH;
bool key2State = HIGH;
bool key2LastState = HIGH;
bool key3State = HIGH;
bool key3LastState = HIGH;

void setup() {
  display.init();
  display.flipScreenVertically();
  
  // 初始化按键引脚
  pinMode(KEY1_PIN, INPUT_PULLUP);
  pinMode(KEY2_PIN, INPUT_PULLUP);
  pinMode(KEY3_PIN, INPUT_PULLUP);
  
  // 初始化Preferences
  preferences.begin("aquaculture", false);
  
  // 读取保存的阈值
  tempThreshold = preferences.getInt("tempThreshold", 25);
  turbidityThreshold = preferences.getInt("turbidityThreshold", 2000);
  phThreshold = preferences.getInt("phThreshold", 2000);
  
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
  // 读取按键状态
  key1State = digitalRead(KEY1_PIN);
  key2State = digitalRead(KEY2_PIN);
  key3State = digitalRead(KEY3_PIN);
  
  // 检测KEY1下降沿
  if (key1LastState == HIGH && key1State == LOW) {
    if (settingMode) {
      // 在设置模式下，KEY1用于切换设置项
      settingIndex++;
      if (settingIndex > 2) {
        // 切换回主页面，保存设置
        preferences.putInt("tempThreshold", tempThreshold);
        preferences.putInt("turbidityThreshold", turbidityThreshold);
        preferences.putInt("phThreshold", phThreshold);
        settingMode = false;
        settingIndex = 0;
      }
    } else {
      // 在正常模式下，KEY1进入设置模式
      settingMode = true;
      settingIndex = 0;
    }
    delay(50); // 消抖
  }
  
  // 检测KEY2下降沿，增加当前设置值
  if (key2LastState == HIGH && key2State == LOW) {
    if (settingMode) {
      switch (settingIndex) {
        case 0: // 温度
          if (tempThreshold < 30) {
            tempThreshold += 1;
          }
          break;
        case 1: // 浑浊度
          if (turbidityThreshold < 4200) {
            turbidityThreshold += 100;
          }
          break;
        case 2: // PH值
          if (phThreshold < 4200) {
            phThreshold += 100;
          }
          break;
      }
      delay(50); // 消抖
    }
  }
  
  // 检测KEY3下降沿，减少当前设置值
  if (key3LastState == HIGH && key3State == LOW) {
    if (settingMode) {
      switch (settingIndex) {
        case 0: // 温度
          if (tempThreshold > 0) {
            tempThreshold -= 1;
          }
          break;
        case 1: // 浑浊度
          if (turbidityThreshold > 0) {
            turbidityThreshold -= 100;
          }
          break;
        case 2: // PH值
          if (phThreshold > 0) {
            phThreshold -= 100;
          }
          break;
      }
      delay(50); // 消抖
    }
  }
  
  // 更新按键状态
  key1LastState = key1State;
  key2LastState = key2State;
  key3LastState = key3State;
  
  // 读取传感器数据
  float temperature = readTemperature();
  int turbidity = readTurbidity();
  int phValue = readPH();
  
  // 显示界面
  display.clear();
  display.setFont(ArialMT_Plain_10);
  
  if (settingMode) {
    // 设置界面
    display.drawString(0, 0, "Setting Mode");
    
    switch (settingIndex) {
      case 0:
        display.drawString(0, 16, "Temperature: " + String(tempThreshold) + " °C *");
        display.drawString(0, 32, "Turbidity: " + String(turbidityThreshold) + " ADC");
        display.drawString(0, 48, "PH Value: " + String(phThreshold) + " ADC");
        break;
      case 1:
        display.drawString(0, 16, "Temperature: " + String(tempThreshold) + " °C");
        display.drawString(0, 32, "Turbidity: " + String(turbidityThreshold) + " ADC *");
        display.drawString(0, 48, "PH Value: " + String(phThreshold) + " ADC");
        break;
      case 2:
        display.drawString(0, 16, "Temperature: " + String(tempThreshold) + " °C");
        display.drawString(0, 32, "Turbidity: " + String(turbidityThreshold) + " ADC");
        display.drawString(0, 48, "PH Value: " + String(phThreshold) + " ADC *");
        break;
    }
  } else {
    // 正常显示界面
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
  }
  
  display.display();
  delay(100); // 增加刷新频率
}
