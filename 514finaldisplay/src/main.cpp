#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <Stepper.h>
#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

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

// Define the UUIDs for BLE service and characteristic
static BLEUUID serviceUUID("db0e37aa-c7ff-4584-936d-e39622848d33");
static BLEUUID charUUID("0fd8fa9f-34da-40bb-8cb7-afc7d0174389");

static boolean doConnect = false;
static boolean connected = false;
static boolean doScan = false;
static BLERemoteCharacteristic* pRemoteCharacteristic;
static BLEAdvertisedDevice* myDevice;

// 用于存储光照度和颜色信息的全局变量
static String currentLux = "Waiting...";
static String currentColor = "Waiting...";

void displayInfo(String color, String lux) {
    // 完全清除屏幕，而不是尝试基于信息长度来清除
    tft.fillScreen(ILI9341_BLACK);
    
    tft.setTextColor(ILI9341_WHITE);  
    tft.setTextSize(2);
    tft.setCursor(0, 0);

    // 显示光照度信息
    tft.println("Lux Detected: " + lux);

    // 显示颜色信息
    tft.println("Color Detected: " + color);

    delay(100);
}
// 全局变量，用于跟踪步进电机的当前位置（以步数为单位）
int currentStepPosition = 0;

void moveStepperToColor(String color) {
    float stepsPerDegree = STEPS_PER_REV / 315.0;
    int targetSteps = 0; // 目标位置的步数

    // 根据颜色计算目标位置的步数
    if(color == "Red") targetSteps = stepsPerDegree * 23;
    else if(color == "Orange") targetSteps = stepsPerDegree * 46;
    else if(color == "Yellow") targetSteps = stepsPerDegree * 69;
    else if(color == "Green") targetSteps = stepsPerDegree * 92;
    else if(color == "Light Blue") targetSteps = stepsPerDegree * 115;
    else if(color == "Blue") targetSteps = stepsPerDegree * 138;
    else if(color == "Purple") targetSteps = stepsPerDegree * 161;

    // 计算从当前位置到目标位置需要移动的步数
    int stepsToMove = targetSteps - currentStepPosition;

    // 移动步进电机到目标位置
    stepper.step(stepsToMove);

    // 更新当前步进电机的位置
    currentStepPosition = targetSteps;
}



void notifyCallback(
  BLERemoteCharacteristic* pBLERemoteCharacteristic,
  uint8_t* pData,
  size_t length,
  bool isNotify) {
    // 创建一个足够大的缓冲区来容纳数据加上null终结符
    char buf[length + 1];
    memcpy(buf, pData, length);
    buf[length] = '\0'; // 添加null终结符

    String message = String(buf); // 使用已终止的字符串创建String对象

    // 在串行监视器上打印接收到的消息
    Serial.print("Received message: ");
    Serial.println(message);

    // 根据接收到的消息类型更新全局变量并打印更新
    if (message.startsWith("Lux: ")) {
        currentLux = message.substring(5); // 更新光照度信息
        Serial.print("Updated Lux: ");
        Serial.println(currentLux);
    } else if (message.startsWith("Color Detected: ")) {
        currentColor = message.substring(16); // 更新颜色信息
        Serial.print("Updated Color: ");
        Serial.println(currentColor);

        // 在这里调用moveStepperToColor函数，以依据颜色移动步进电机
        moveStepperToColor(currentColor);
    }

    // 无论是收到光照度还是颜色信息，都刷新显示
    displayInfo(currentColor, currentLux);
}

class MyClientCallback : public BLEClientCallbacks {
    void onConnect(BLEClient* pclient) {
    }

    void onDisconnect(BLEClient* pclient) {
        connected = false;
        Serial.println("onDisconnect");
    }
};

bool connectToServer() {
    Serial.print("Forming a connection to ");
    Serial.println(myDevice->getAddress().toString().c_str());

    BLEClient* pClient = BLEDevice::createClient();
    Serial.println(" - Created client");

    pClient->setClientCallbacks(new MyClientCallback());
    pClient->connect(myDevice);
    Serial.println(" - Connected to server");

    // Increase the MTU size if necessary
    pClient->setMTU(517);

    BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
    if (pRemoteService == nullptr) {
      Serial.print("Failed to find our service UUID: ");
      Serial.println(serviceUUID.toString().c_str());
      pClient->disconnect();
      return false;
    }
    Serial.println(" - Found our service");

    pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID);
    if (pRemoteCharacteristic == nullptr) {
      Serial.print("Failed to find our characteristic UUID: ");
      Serial.println(charUUID.toString().c_str());
      pClient->disconnect();
      return false;
    }
    Serial.println(" - Found our characteristic");

    if(pRemoteCharacteristic->canRead()) {
      std::string value = pRemoteCharacteristic->readValue();
      Serial.print("The characteristic value was: ");
      Serial.println(value.c_str());
    }

    if(pRemoteCharacteristic->canNotify())
      pRemoteCharacteristic->registerForNotify(notifyCallback);

    connected = true;
    return true;
}

class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice advertisedDevice) {
        Serial.print("BLE Advertised Device found: ");
        Serial.println(advertisedDevice.toString().c_str());

        if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(serviceUUID)) {
            BLEDevice::getScan()->stop();
            myDevice = new BLEAdvertisedDevice(advertisedDevice);
            doConnect = true;
            doScan = true;
        }
    }
};

void setup() {
    Serial.begin(115200);
    tft.begin();
    if (TFT_LED > -1) {
        pinMode(TFT_LED, OUTPUT);
        digitalWrite(TFT_LED, HIGH); // 如果您的屏幕需要背光控制
    }
    tft.fillScreen(ILI9341_BLACK);
    tft.setRotation(3);
    tft.setTextSize(2);
    tft.setTextColor(ILI9341_WHITE);
    tft.setCursor(0, 0);
    tft.println("Connecting...");

    BLEDevice::init("");
    BLEScan* pBLEScan = BLEDevice::getScan();
    pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
    pBLEScan->setActiveScan(true);
    pBLEScan->start(30);

    // 设置步进电机速度
    stepper.setSpeed(60);  // 设置为60转/分钟
}

void loop() {
  if (doConnect == true) {
    if (connectToServer()) {
      Serial.println("We are now connected to the BLE Server.");
    } else {
      Serial.println("We have failed to connect to the server; there is nothing more we will do.");
    }
    doConnect = false;
  }

  if (connected) {
    // No need to write anything to the characteristic; comment out
    // delay(1000); // Delay to keep the loop sane
  } else if (doScan) {
    BLEDevice::getScan()->start(0);
  }
}