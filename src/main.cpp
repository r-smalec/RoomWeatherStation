#include <Arduino.h>

/***************************************************************************
  This is a library for the BME280 humidity, temperature & pressure sensor

  Designed specifically to work with the Adafruit BME280 Breakout
  ----> http://www.adafruit.com/products/2650

  These sensors use I2C or SPI to communicate, 2 or 4 pins are required
  to interface. The device's I2C address is either 0x76 or 0x77.

  Adafruit invests time and resources providing this open source code,
  please support Adafruit andopen-source hardware by purchasing products
  from Adafruit!

  Written by Limor Fried & Kevin Townsend for Adafruit Industries.
  BSD license, all text above must be included in any redistribution
  See the LICENSE file for details.
 ***************************************************************************/

#include <array>
#include <algorithm>

#include <Wire.h>
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DFRobot_ENS160.h>
#include <SPIMemory.h>
#include <SPI.h>

#define ENS_SCK 13
#define ENS_MISO 12
#define ENS_MOSI 11
#define ENS_CS 10
//DFRobot_ENS160_SPI ens(&SPI, ENS_CS);

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define SEALEVELPRESSURE_HPA (1013.25)
void printValues(void);

Adafruit_BME280 bme; // I2C

#define MEM_CS 10
#define MEM_BUF_LENGTH 100

unsigned long delayTime;

void setup() {
    
    SPI.begin();
    SPI.beginTransaction(SPISettings(20000, MSBFIRST, SPI_MODE0));
    pinMode(MEM_CS, OUTPUT);
    Serial.begin(9600);
    while(!Serial);    // time to get serial running
        // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
    if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
    }
    // Show initial display buffer contents on the screen --
    // the library initializes this with an Adafruit splash screen.
    display.display();
    delay(2000); // Pause for 2 seconds

    // Clear the buffer
    display.clearDisplay();

    
    // default settings
    //status = bme.begin();  
    // You can also pass in a Wire library object like &Wire2

    uint8_t timeoutNo = 3;

    if (!bme.begin(0x76, &Wire)) {
        Serial.println("Could not find a valid BME280 sensor, check wiring, address, sensor ID!");
        Serial.print("SensorID was: 0x"); Serial.println(bme.sensorID(),16);
        while (timeoutNo--) delay(10);
    }

    timeoutNo = 3;

    // if(ens.begin() != NO_ERR) {
    //     Serial.println("Communication with ENS160 failed, chcek connections and power");
    //     while (timeoutNo--) delay(10);
    // }

    // String bmeStat(bme.sensorID(), DEC);
    // String ensStat(ens.getENS160Status(), DEC);
    // Serial.println("BME status: " + bmeStat + " ENS status " + ensStat + ".");
    
    // ens.setPWRMode(ENS160_STANDARD_MODE);
    // ens.setTempAndHum(bme.readTemperature(), bme.readHumidity());
    

    delayTime = 500;

    Serial.println();
    
    digitalWrite(MEM_CS, LOW);
    uint16_t manufacturerID = 0x0000;
    SPI.transfer(0x90);
    SPI.transfer(0x00);
    SPI.transfer(0x00);
    manufacturerID = SPI.transfer(0x00);
    digitalWrite(MEM_CS, HIGH);
    String manufacturerIDstr = String(manufacturerID, HEX);
    Serial.print("Manufacturer ID: ");
    Serial.println(manufacturerIDstr);
}


void loop() {

    printValues();
    delay(delayTime);
    // for(uint8_t memAddr = 0x01; memAddr < 0xFF; memAddr++) {
    //     digitalWrite(MEM_CS, LOW);
    //     uint8_t memData = 0x69;
    //     SPI.transfer(0x03);
    //     SPI.transfer(0x00);
    //     SPI.transfer(0x00);
    //     memData = SPI.transfer(memAddr);
    //     Serial.print(memAddr);
    //     Serial.print(": ");
    //     Serial.println(memData);
    //     digitalWrite(MEM_CS, HIGH);
    // }

    digitalWrite(MEM_CS, LOW);
    uint8_t manufacturerID = 0x0000;
    uint8_t deviceID = 0x0000;
    SPI.transfer(0x90);
    SPI.transfer(0x00);
    SPI.transfer(0x00);
    SPI.transfer(0x00);
    manufacturerID = SPI.transfer(0x55);
    deviceID = SPI.transfer(0x55);
    delay(1);
    digitalWrite(MEM_CS, HIGH);
    String manufacturerID_str = String(manufacturerID, HEX);
    String deviceID_str = String(deviceID, HEX);
    Serial.print("Manufacturer ID: ");
    Serial.println(manufacturerID_str);
    Serial.print("Device ID: ");
    Serial.println(deviceID_str);
}

template <class T> void printLine(String txt, T val) {

    const int16_t curX = 90;

    display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
    display.print(txt);
    display.setTextColor(SSD1306_WHITE);
    display.fillRect(curX, display.getCursorY(), 50, 8, SSD1306_BLACK);
    display.setCursor(curX, display.getCursorY());
    display.println(val);
}

void printValues() {

    display.setTextSize(1);
    display.setCursor(0, 0);
    
    printLine<float>(" Temp[C] = ",       bme.readTemperature());
    printLine<float>(" Pre[hPa] = ",      bme.readPressure() / 100.0F);
    printLine<float>(" Alt[m] = ",        bme.readAltitude(SEALEVELPRESSURE_HPA));
    printLine<float>(" Hum[%] = ",        bme.readHumidity());

    // printLine<uint8_t>(" AQI[-] = ",      ens.getAQI());
    // printLine<uint16_t>(" TVOC[ppb] = ",  ens.getTVOC());
    // printLine<uint16_t>(" ECO2[ppb] = ",  ens.getECO2());

    display.display();
}