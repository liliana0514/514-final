#include <Arduino.h>
#include <Wire.h>
#include "Adafruit_VEML7700.h"
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <stdlib.h>


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

// BLE Server
BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;
bool deviceConnected = false;
bool oldDeviceConnected = false;
unsigned long previousMillis = 0;
const long interval = 1000;

#define SERVICE_UUID "db0e37aa-c7ff-4584-936d-e39622848d33"
#define CHARACTERISTIC_UUID "0fd8fa9f-34da-40bb-8cb7-afc7d0174389"

class MyServerCallbacks : public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
        deviceConnected = true;
    };

    void onDisconnect(BLEServer* pServer) {
        deviceConnected = false;
    }
};

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

    // 初始化BLE设备
    BLEDevice::init("ESP32_Color_Detector");
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());
    BLEService *pService = pServer->createService(SERVICE_UUID);
    pCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID,
        BLECharacteristic::PROPERTY_READ |
        BLECharacteristic::PROPERTY_WRITE |
        BLECharacteristic::PROPERTY_NOTIFY
    );
    pCharacteristic->addDescriptor(new BLE2902());

    pService->start();
    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    pAdvertising->setMinPreferred(0x06);
    pAdvertising->setMinPreferred(0x12);
    BLEDevice::startAdvertising();

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
            if (deviceConnected) {
                readVEML7700();
                readTCS230();
            }
        }
    }


    lastButtonState = currentButtonState;

    // Handle BLE connection status
    if (!deviceConnected && oldDeviceConnected) {
        delay(500); // 稍微延迟，等待可能的重连
        pServer->startAdvertising(); // 重新开始广告，以便新的客户端可以连接
        Serial.println("We are now connected to the BLE Client.");
        oldDeviceConnected = deviceConnected;
    } else if (deviceConnected && !oldDeviceConnected) {
        // 更新设备连接状态
        oldDeviceConnected = deviceConnected;
    }
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
    float lux = veml.readLux();
    String message = "Lux: " + String(lux);
    Serial.println(message);
    if (deviceConnected) {
        pCharacteristic->setValue(message.c_str());
        pCharacteristic->notify();
    }
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
        String colorMessage = "Color Detected: " + String(colorNames[closestColor]);
        Serial.println(colorMessage);
        if (deviceConnected) {
            pCharacteristic->setValue(colorMessage.c_str());
            pCharacteristic->notify();
        }
    } else {
        String unclearMessage = "Color unclear";
        Serial.println(unclearMessage);
        if (deviceConnected) {
            pCharacteristic->setValue(unclearMessage.c_str());
            pCharacteristic->notify();
        }
    }
}

