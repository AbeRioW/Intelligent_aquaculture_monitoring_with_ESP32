#ifndef DS18B20_H
#define DS18B20_H

#include <Arduino.h>

#define DQ_PIN 4

// 精确的1us延时函数
inline void delayDQ(unsigned int us) {
  delayMicroseconds(us);
}

// 初始化DS18B20
// 返回0：初始化成功
unsigned char Init_18B20() {
  unsigned char dat;
  pinMode(DQ_PIN, OUTPUT);
  digitalWrite(DQ_PIN, 1);
  delayDQ(10);    // 恢复时间：>1us
  digitalWrite(DQ_PIN, 0);
  delay(2);       // 单片机拉低总线:>960us (延长1倍，确保大于480us)
  digitalWrite(DQ_PIN, 1);
  delayDQ(60);    // 释放总线，在中间位置读取是否有器件响应
  pinMode(DQ_PIN, INPUT);
  dat = digitalRead(DQ_PIN);
  pinMode(DQ_PIN, OUTPUT);
  digitalWrite(DQ_PIN, 1);
  delay(2);       // 确保单片机释放总线后的全部时间：>960us (延长1倍，确保大于480us)
  return dat;
}

// 读一位数据
unsigned char ReadOneBit() {
  unsigned char dat = 0;
  pinMode(DQ_PIN, OUTPUT);
  digitalWrite(DQ_PIN, 0);
  delayDQ(1);     // 下拉总线：>1us
  digitalWrite(DQ_PIN, 1);
  delayDQ(4);     // 释放总线，进行采样，确保从下拉开始的总共时间：<15us (缩短为原来的一半)
  pinMode(DQ_PIN, INPUT);
  dat = digitalRead(DQ_PIN);
  pinMode(DQ_PIN, OUTPUT);
  digitalWrite(DQ_PIN, 1);
  delayDQ(100);   // 读取DQ，此延时不能少，确保从下拉开始的总共时间：>120us (延长1倍，确保大于60us)
  return(dat);
}

// 从18B20读一个字节数据
unsigned char ReadOneChar() {
  unsigned char i=0;
  unsigned char dat = 0;
  for (i=8; i>0; i--) {
    dat >>= 1;              // 让从总线上读到的位数据，依次从高位移动到低位。
    if(ReadOneBit())        // 控制器进行采样
      dat |= 0x80;          // 存储读取的位
  }
  pinMode(DQ_PIN, OUTPUT);
  digitalWrite(DQ_PIN, 1);
  return(dat);
}

// 写一位数据
void WriteOneBit(unsigned char dat) {
  pinMode(DQ_PIN, OUTPUT);
  digitalWrite(DQ_PIN, 0);
  delayDQ(1);     // 下拉总线：>1us
  digitalWrite(DQ_PIN, dat);
  delayDQ(118);   // 确保从下拉开始的总共时间：>120us (延长1倍，确保大于60us)
  digitalWrite(DQ_PIN, 1);
  delayDQ(4);     // 恢复时间：>1us
}

// 向18B20写入一个字节数据
void WriteOneChar(unsigned char dat) {
  unsigned char i=0;
  for (i=8; i>0; i--) {
    WriteOneBit(dat & 0x01);
    dat >>= 1;
  }
  pinMode(DQ_PIN, OUTPUT);
  digitalWrite(DQ_PIN, 1);
}

// 读取温度值
float ds18b20_read_temperature() {
  unsigned char tempL, tempH;
  int16_t temp;
  
  if (Init_18B20() != 0) {
    return -127.0; // 传感器未响应
  }
  
  WriteOneChar(0xCC); // 跳过ROM
  WriteOneChar(0x44); // 启动温度转换
  
  // 等待转换完成
  delay(750);
  
  if (Init_18B20() != 0) {
    return -127.0; // 传感器未响应
  }
  
  WriteOneChar(0xCC); // 跳过ROM
  WriteOneChar(0xBE); // 读取温度数据
  
  tempL = ReadOneChar(); // 读取低字节
  tempH = ReadOneChar(); // 读取高字节
  
  temp = (tempH << 8) | tempL;
  
  return (float)temp / 16.0;
}

#endif // DS18B20_H