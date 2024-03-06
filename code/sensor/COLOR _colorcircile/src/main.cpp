#include <Arduino.h>

#define S0 1
#define S1 2
#define S2 3
#define S3 4
#define sensorOut 7
#define buttonPin 9

int redFrequency = 0;
int greenFrequency = 0;
int blueFrequency = 0;
bool pressed = false;
int buttonPressCount = 0;

// 存儲校正後的顏色數值
int mappedRedMin = 0;
int mappedRedMax = 255;
int mappedGreenMin = 0;
int mappedGreenMax = 255;
int mappedBlueMin = 0;
int mappedBlueMax = 255;

void calibrateColor(const char* color, int& frequency); // 声明 calibrateColor 函数
void detectNewColor(); // 声明 detectNewColor 函数

void setup() {
  Serial.begin(115200);
  while (!Serial) {
        delay(10);
  }
  pinMode(S0, OUTPUT);
  pinMode(S1, OUTPUT);
  pinMode(S2, OUTPUT);
  pinMode(S3, OUTPUT);
  pinMode(sensorOut, INPUT);
  pinMode(buttonPin, INPUT_PULLUP); // 使用內部上拉電阻

  digitalWrite(S0, HIGH);
  digitalWrite(S1, LOW);

  Serial.println("Please align TCS230 with red color sample and press the button to calibrate.");
}

void loop() {
  if (!pressed && digitalRead(buttonPin) == LOW) { // 如果按鈕按下且未顯示提示
    pressed = true; // 設置按鈕已按下的標誌
    buttonPressCount++;
    Serial.println("Button pressed.");
  }

  if (pressed) { // 如果按鈕已按下
    switch (buttonPressCount) {
      case 1: // 第一次按下，校準紅色
        if (digitalRead(buttonPin) == LOW) {
          calibrateColor("RED", redFrequency);
          Serial.println("Please align TCS230 with green color sample and press the button to calibrate.");
          buttonPressCount++;
        }
        break;
      case 2: // 第二次按下，校準綠色
        if (digitalRead(buttonPin) == LOW) {
          calibrateColor("GREEN", greenFrequency);
          Serial.println("Please align TCS230 with blue color sample and press the button to calibrate.");
          buttonPressCount++;
        }
        break;
      case 3: // 第三次按下，校準藍色
        if (digitalRead(buttonPin) == LOW) {
          calibrateColor("BLUE", blueFrequency);
          Serial.println("Calibration Done!");
          pressed = false; // 校正完成後停止偵測新的顏色
        }
        break;
      default:
        break;
    }
  }

  if (!pressed && digitalRead(buttonPin) == HIGH) { // 如果按鈕釋放，重置按鈕按下的標誌
    buttonPressCount = 0;
    Serial.println("Button released.");
  }

  // 只有在按鈕被按下並且校正完成後，才持續偵測新的顏色
  if (!pressed && buttonPressCount > 3) {
    detectNewColor();  
  }
}



void calibrateColor(const char* color, int& frequency) {
  Serial.print("Calibrating ");
  Serial.println(color);

  int minFrequency = INT_MAX; // 初始化最小值為整型最大值
  int maxFrequency = INT_MIN; // 初始化最大值為整型最小值

  for (int i = 0; i < 10; i++) { // 偵測十次
    frequency = 0; // 重置 frequency

    digitalWrite(S2, LOW); // 紅色
    digitalWrite(S3, LOW);
    frequency = pulseIn(sensorOut, LOW);
    Serial.print(color);
    Serial.print(" = ");
    Serial.println(frequency);
    
    if (frequency < minFrequency) minFrequency = frequency;
    if (frequency > maxFrequency) maxFrequency = frequency;
    
    delay(100);
  }

  Serial.print(color);
  Serial.print(" Min: ");
  Serial.print(minFrequency);
  Serial.print(" Max: ");
  Serial.println(maxFrequency);

  // 映射到範圍為 0 到 255
  int mappedMinFrequency = map(minFrequency, minFrequency, maxFrequency, 0, 255);
  int mappedMaxFrequency = map(maxFrequency, minFrequency, maxFrequency, 0, 255);

  Serial.print(color);
  Serial.print(" Mapped Min: ");
  Serial.print(mappedMinFrequency);
  Serial.print(" Mapped Max: ");
  Serial.println(mappedMaxFrequency);

  // 將校正後的數值存儲到全局變量中
  if (strcmp(color, "RED") == 0) {
    mappedRedMin = mappedMinFrequency;
    mappedRedMax = mappedMaxFrequency;
  } else if (strcmp(color, "GREEN") == 0) {
    mappedGreenMin = mappedMinFrequency;
    mappedGreenMax = mappedMaxFrequency;
  } else if (strcmp(color, "BLUE") == 0) {
    mappedBlueMin = mappedMinFrequency;
    mappedBlueMax = mappedMaxFrequency;
  }

  delay(1000); // 避免按鈕彈起時連續執行
}

void detectNewColor() {
  int newRedFrequency = 0;
  int newGreenFrequency = 0;
  int newBlueFrequency = 0;

  // 偵測新顏色的紅色成分
  digitalWrite(S2, LOW); // 紅色
  digitalWrite(S3, LOW);
  newRedFrequency = pulseIn(sensorOut, LOW);
  
  // 將紅色成分映射到 0 到 255 的範圍
  int mappedNewRedFrequency = map(newRedFrequency, mappedRedMin, mappedRedMax, 0, 255);

  // 偵測新顏色的綠色成分
  digitalWrite(S2, HIGH); // 綠色
  digitalWrite(S3, HIGH);
  newGreenFrequency = pulseIn(sensorOut, LOW);

  // 將綠色成分映射到 0 到 255 的範圍
  int mappedNewGreenFrequency = map(newGreenFrequency, mappedGreenMin, mappedGreenMax, 0, 255);

  // 偵測新顏色的藍色成分
  digitalWrite(S2, LOW); // 藍色
  digitalWrite(S3, HIGH);
  newBlueFrequency = pulseIn(sensorOut, LOW);

  // 將藍色成分映射到 0 到 255 的範圍
  int mappedNewBlueFrequency = map(newBlueFrequency, mappedBlueMin, mappedBlueMax, 0, 255);

  // 在串行監視器上打印新顏色的結果
  Serial.print("Detected New Color: ");
  Serial.print("R = ");
  Serial.print(mappedNewRedFrequency);
  Serial.print(", G = ");
  Serial.print(mappedNewGreenFrequency);
  Serial.print(", B = ");
  Serial.println(mappedNewBlueFrequency);
}
