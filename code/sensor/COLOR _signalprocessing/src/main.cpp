#include <Wire.h>
#include "Adafruit_VEML7700.h"

Adafruit_VEML7700 veml = Adafruit_VEML7700();

void setup() {
  Serial.begin(115200);
  // 等待串口连接
  delay(1000);
  Serial.println("Initializing VEML7700...");

  if (!veml.begin()) {
    Serial.println("Failed to find VEML7700 chip");
    while (1); // 在这里无限循环
  }
  Serial.println("VEML7700 Found!");
}

void loop() {
  // 您的代码逻辑
}
