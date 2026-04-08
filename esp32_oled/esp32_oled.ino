#include <Wire.h>
#include "SSD1306.h"
#include "DS18B20.h"
#include <Preferences.h>
#include <WiFi.h>      // ESP32 WiFi库
#include <PubSubClient.h>       // MQTT客户端库

Preferences preferences;

SSD1306 display(0x3c, 21, 22);

// WiFi配置
const char* ssid = "jingda830";      // WiFi名称
const char* password = "jd717718"; // WiFi密码

// OneNet MQTT配置
const char* mqttServer = "mqtts.heclouds.com"; // OneNet MQTT服务器地址
const int mqttPort = 1883;                // OneNet MQTT端口
const char* mqttClientId = "esp32_oled"; // 设备ID
const char* mqttUser = "dU5jVg1L9b";     // 产品ID
const char* mqttPassword = "version=2018-10-31&res=products%2FdU5jVg1L9b%2Fdevices%2Fesp32_oled&et=2810377042&method=md5&sign=1wRzfPZDjJ6ztNqWYY9lIg%3D%3D"; // API密钥

// WiFi和MQTT客户端
WiFiClient espClient;
PubSubClient client(espClient);

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

// 函数声明
void setupWiFi();         // WiFi连接设置
void connectMQTT();       // 连接到MQTT服务器
void mqttCallback(char* topic, byte* payload, unsigned int length); // MQTT消息回调
void sendSensorData();    // 发送传感器数据到OneNet

void setup() {
  Serial.begin(115200);
  
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
  
  // 设置WiFi
  setupWiFi();
  
  // 配置MQTT客户端
  client.setServer(mqttServer, mqttPort);
  client.setCallback(mqttCallback);
  
  // 连接到MQTT服务器
  connectMQTT();
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
  // 保持MQTT连接
  if (!client.connected()) {
    connectMQTT();
  }
  client.loop();
  
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
  
  // 定期发送传感器数据到OneNet
  static unsigned long lastSend = 0;
  if (millis() - lastSend > 2000) {
    sendSensorData();
    lastSend = millis();
  }
  
  delay(100); // 增加刷新频率
}

/**
 * WiFi连接设置
 * 连接到指定的WiFi网络
 */
void setupWiFi() {
  Serial.print("Connecting to ");
  Serial.println(ssid);
  
  WiFi.begin(ssid, password); // 开始连接WiFi
  
  // 等待WiFi连接成功
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP()); // 打印IP地址
}

/**
 * 连接到MQTT服务器
 * 建立与OneNet MQTT服务的连接
 */
void connectMQTT() {
  while (!client.connected()) {
    Serial.print("Connecting to MQTT...");
    if (client.connect(mqttClientId, mqttUser, mqttPassword)) {
      Serial.println("connected");
      // 订阅命令主题
      String cmdTopic = "$sys/" + String(mqttUser) + "/" + String(mqttClientId) + "/cmd/request/#";
      client.subscribe(cmdTopic.c_str());
      Serial.print("Subscribed to: ");
      Serial.println(cmdTopic);
      
      // 连接成功后立即发送传感器数据
      sendSensorData();
      Serial.println("Initial sensor data sent after MQTT connection");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" retrying in 5 seconds");
      delay(5000);
    }
  }
}

/**
 * MQTT消息回调函数
 * 处理从OneNet接收到的命令
 */
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("]: ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
  
  // 解析JSON命令
  String message = "";
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  
  // 这里可以添加命令处理逻辑
  // 例如：切换模式、调整阈值等
}

/**
 * 发送传感器数据到OneNet云平台
 * 使用MQTT协议上传传感器数据
 */
void sendSensorData() {
  // 读取传感器数据
  float temperature = readTemperature();
  int turbidity = readTurbidity();
  int phValue = readPH();
  
  // 构建JSON字符串
  String json = "{\"id\":\"123\",\"params\":{";
  json += "\"temperature\":{\"value\":" + String(temperature) + "},";
  json += "\"turbidity\":{\"value\":" + String(turbidity) + "},";
  json += "\"ph\":{\"value\":" + String(phValue) + "}";
  json += "}}";
  
  // 发布数据到OneNet设备接入平台
  String topic = "$sys/" + String(mqttUser) + "/" + String(mqttClientId) + "/thing/property/post";
  
  if (client.publish(topic.c_str(), json.c_str())) {
    Serial.println("Data sent to OneNet successfully");
    Serial.print("Topic: ");
    Serial.println(topic);
    Serial.print("Payload: ");
    Serial.println(json);
  } else {
    Serial.println("Failed to send data to OneNet");
  }
}
