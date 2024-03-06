#include <Arduino.h>
#include <Wire.h>
#include "Adafruit_VEML7700.h"

Adafruit_VEML7700 veml = Adafruit_VEML7700();

// TCS230 引脚定义
const int S0 = 1;
const int S1 = 2;
const int S2 = 3;
const int S3 = 4;
const int OUT = 7;

// 按钮引脚定义
const int BUTTON_PIN = 9;

// 校准阶段 Enum
enum CalibrationStage {
    WAITING,
    CALIBRATING_RED,
    CALIBRATING_ORANGE,
    CALIBRATING_YELLOW,
    CALIBRATING_GREEN,
    CALIBRATING_LIGHT_BLUE,
    CALIBRATING_BLUE,
    CALIBRATING_PURPLE,
    CALIBRATION_DONE
};

CalibrationStage calibrationStage = WAITING;

// 颜色数据结构
struct ColorCalibrationData {
    unsigned long redFrequency;
    unsigned long greenFrequency;
    unsigned long blueFrequency;
};

// 定义每种颜色的校准数据变量
ColorCalibrationData calibrationData[7]; // 对应七种颜色

// 函数声明
void performCalibration();
void readVEML7700();
void readTCS230();

void setup() {
    Serial.begin(115200);
    Serial.println("Setup complete");

    Wire.begin();

    if (!veml.begin()) {
        Serial.println("Failed to find VEML7700 chip");
        while (1);
    }
    Serial.println("VEML7700 Found!");

    pinMode(S0, OUTPUT);
    pinMode(S1, OUTPUT);
    pinMode(S2, OUTPUT);
    pinMode(S3, OUTPUT);
    pinMode(OUT, INPUT);

    digitalWrite(S0, LOW);  // Set frequency scaling to 20%
    digitalWrite(S1, HIGH);

    pinMode(BUTTON_PIN, INPUT_PULLUP);

    calibrationStage = CALIBRATING_RED;
    Serial.println("Please align TCS230 with red color sample and press the button to calibrate.");
}

void loop() {
    static bool lastButtonState = HIGH;
    bool currentButtonState = digitalRead(BUTTON_PIN);

    if (currentButtonState == LOW && lastButtonState == HIGH) {
        delay(300);  // 防抖延时

        if (calibrationStage != CALIBRATION_DONE) {
            performCalibration();  // 继续校准
        } else {
            // 校准完成后的操作，比如颜色检测
            readVEML7700();
            readTCS230();
        }
    }

    lastButtonState = currentButtonState;
}

void performCalibration() {
    Serial.println("Starting calibration...");

    unsigned long redFrequency = 0, greenFrequency = 0, blueFrequency = 0;

    for (int i = 0; i < 5; i++) {
        // 测量红色频率
        digitalWrite(S2, LOW);
        digitalWrite(S3, LOW);
        redFrequency = pulseIn(OUT, LOW);
        calibrationData[calibrationStage - 1].redFrequency += redFrequency;

        // 测量绿色频率
        digitalWrite(S2, HIGH);
        digitalWrite(S3, HIGH);
        greenFrequency = pulseIn(OUT, LOW);
        calibrationData[calibrationStage - 1].greenFrequency += greenFrequency;

        // 测量蓝色频率
        digitalWrite(S2, LOW);
        digitalWrite(S3, HIGH);
        blueFrequency = pulseIn(OUT, LOW);
        calibrationData[calibrationStage - 1].blueFrequency += blueFrequency;
    }

    // 计算平均频率
    calibrationData[calibrationStage - 1].redFrequency /= 5;
    calibrationData[calibrationStage - 1].greenFrequency /= 5;
    calibrationData[calibrationStage - 1].blueFrequency /= 5;

    // 显示平均频率信息
    Serial.println("------------------------------------------------");
    Serial.print("Average Red Frequency: "); Serial.println(calibrationData[calibrationStage - 1].redFrequency);
    Serial.print("Average Green Frequency: "); Serial.println(calibrationData[calibrationStage - 1].greenFrequency);
    Serial.print("Average Blue Frequency: "); Serial.println(calibrationData[calibrationStage - 1].blueFrequency);
    Serial.println("------------------------------------------------");

    // 更新校准阶段并立即显示下一阶段的提示信息
    if (calibrationStage < CALIBRATING_PURPLE) {
        calibrationStage = static_cast<CalibrationStage>(calibrationStage + 1);
        const char* nextColorMessage[] = {"ORANGE", "YELLOW", "GREEN", "LIGHT BLUE", "BLUE", "PURPLE"};
        Serial.print("Please align TCS230 with ");
        Serial.print(nextColorMessage[calibrationStage - CALIBRATING_ORANGE]);
        Serial.println(" color sample and press the button to calibrate.");
    } else if (calibrationStage == CALIBRATING_PURPLE) {
        calibrationStage = CALIBRATION_DONE;
        Serial.println("Calibration done. You can now start detecting colors.");
    }
}


void readVEML7700() {
    // 示例实现：读取VEML7700光照强度传感器的数据并打印
    float lux = veml.readLux();
    Serial.print("Lux: ");
    Serial.println(lux);
}

void readTCS230() {
    Serial.println("Detecting color...");

    // 测量当前颜色频率
    digitalWrite(S2, LOW); digitalWrite(S3, LOW); // Select red filter
    unsigned long currentRedFrequency = pulseIn(OUT, LOW, 1000000);
    digitalWrite(S2, HIGH); digitalWrite(S3, HIGH); // Select green filter
    unsigned long currentGreenFrequency = pulseIn(OUT, LOW, 1000000);
    digitalWrite(S2, LOW); digitalWrite(S3, HIGH); // Select blue filter
    unsigned long currentBlueFrequency = pulseIn(OUT, LOW, 1000000);

    // 打印当前测量值
    Serial.print("Current Red Frequency: "); Serial.println(currentRedFrequency);
    Serial.print("Current Green Frequency: "); Serial.println(currentGreenFrequency);
    Serial.print("Current Blue Frequency: "); Serial.println(currentBlueFrequency);

    unsigned long minDifference = ULONG_MAX;
    int closestColor = -1;
    for (int i = 0; i < 7; i++) {
        unsigned long difference = abs(static_cast<long>(currentRedFrequency - calibrationData[i].redFrequency)) +
                                   abs(static_cast<long>(currentGreenFrequency - calibrationData[i].greenFrequency)) +
                                   abs(static_cast<long>(currentBlueFrequency - calibrationData[i].blueFrequency));

        if (difference < minDifference) {
            minDifference = difference;
            closestColor = i;
        }
    }

    // 根据closestColor值显示最接近的颜色
    const char* colorNames[] = {"Red", "Orange", "Yellow", "Green", "Light Blue", "Blue", "Purple"};
    if (closestColor != -1) {
        Serial.print("最接近的颜色是: ");
        Serial.println(colorNames[closestColor]);
    } else {
        Serial.println("Color unclear");
    }
}

