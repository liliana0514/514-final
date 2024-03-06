# Color Analyzer
A **smart color collector** for your future design 💡 

### Background
- In design and art, getting colors right is tricky. Designers and artists struggle to capture exact shades from life or other works. 
- Usually, picking colors is pretty subjective, which can mess up how the final work looks and communicates. 
- Plus, there’s no easy way to turn real-life colors into digital codes, making the whole process even more of a headache. 

### Proposed Solution
- TCS2300 color sensor allowing for detailed calibration and precise capture of new color parameters. With its built-in Bluetooth functionality, the analyzer sends color and lux values in real time to a display device. A stepper Motor automatically rotates to the corresponding angle based on color parameters, offering physical feedback.

## The Sensor Device (Color Analyzer)
### Components
- SEEED STUDIO XIAO ESP32S3 Wi-Fi/B
- VEML7700 (Lux Sensor)
- Button(6x6mm)
- TCS2300 (Color Sensor)

## The Display Device (Wireless Display Screen)
### Components
- SEEED STUDIO XIAO ESP32S3 Wi-Fi/B
- LED
- X27 168 bipolar Stepper Motor
- 2.2 Inch ILI9341 SPI TFT LCD
- 3-pin Switch
- Battery 1000mAh 3.7V 

## Information Architecture
![Information flow](https://github.com/liliana0514/514-final/blob/main/Information%20flow.jpg?raw=true)
