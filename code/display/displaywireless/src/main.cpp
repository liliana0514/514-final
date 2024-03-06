#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <Stepper.h>

// ILI9341 TFT LCD引腳定義
#define TFT_CS     43
#define TFT_RST    6
#define TFT_DC     5
#define TFT_MOSI   9
#define TFT_CLK    7
#define TFT_LED    -1  // 如果直接連接到3V3，則不需要控制

// 步進電機引腳定義
#define MOTOR_PIN1 3
#define MOTOR_PIN2 4
#define MOTOR_PIN3 2
#define MOTOR_PIN4 1

// 定義步進電機每圈的步數
#define STEPS_PER_REV 600

// 初始化步進電機和顯示器
Stepper stepper(STEPS_PER_REV, 1, 2, 3, 4);
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_MOSI, TFT_CLK, TFT_RST);

void setup() {
  Serial.begin(115200);
  
  // 初始化显示器
  tft.begin();
  if (TFT_LED > -1) {
    pinMode(TFT_LED, OUTPUT);
    digitalWrite(TFT_LED, HIGH);
  }

  // 设置步进电机的速度
  stepper.setSpeed(60); // 从10 RPM开始，根据需要调整

  // 显示"Hello, World!"并旋转180度
  tft.fillScreen(ILI9341_BLACK);
  tft.setRotation(2); // 旋转180度
  tft.setCursor(0, 0);
  tft.println("Hello, World!");
}

void loop() {
  // 计算旋转150度所需的步数
  float stepsPerDegree = STEPS_PER_REV / 315.0;
  int stepsToRotate = (int)(stepsPerDegree * 150
  );

  // 从原点旋转150度
  stepper.step(stepsToRotate);
  delay(500);
  stepper.step(-stepsToRotate);
  delay(500);


}
