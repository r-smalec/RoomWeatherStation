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
#include <DFRobot_DS1307.h>

#define SERIAL_DBG
//#define SERIAL_THRHLD
template <class T> void printLine(String txt, T val);
void printValues(void);
bool confTerminal(void);

const int cmdMaxSize = 15;
    
DFRobot_DS1307 rtcClk;

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define SEALEVELPRESSURE_HPA (1013.25)
Adafruit_BME280 bme; // I2C

#define ENS_SCK 13
#define ENS_MISO 12
#define ENS_MOSI 11
#define ENS_CS 9
DFRobot_ENS160_I2C ens(&Wire, 0x53);

#define MEM_CS 10
#define MEM_BUF_LENGTH 100

unsigned long delayTime;
uint8_t actualAQI;
bool changeAQI;

void setup() {
    
    uint8_t timeoutCount = 0;

    Serial.begin(9600);
    while(!Serial && timeoutCount <= 5) {
        delay(500);
        timeoutCount++;
    }
    
    while(!rtcClk.begin() && timeoutCount <= 5) {
        #ifdef SERIAL_DBG
            Serial.print("Could not find a valid DS1307 RTC, attempt no: ");
            Serial.println(timeoutCount + 1);
        #endif
        delay(500);
        timeoutCount++;
    }
    //SEC-MIN-HOUR DAY-MONTH-YEAR
    uint16_t setTimeBuff[7] = {22, 21, 19, 5, 20, 2, 2023};
    rtcClk.setTime(setTimeBuff);
    rtcClk.start();
    rtcClk.setSqwPinMode(rtcClk.eSquareWave_1Hz);

    SPI.begin();
    SPI.beginTransaction(SPISettings(20000, MSBFIRST, SPI_MODE0));
    pinMode(MEM_CS, OUTPUT);

    timeoutCount = 0;
    while(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS) && timeoutCount <= 5) {
        #ifdef SERIAL_DBG    
            Serial.print("SSD1306 allocation failed, attempt no");
            Serial.println(timeoutCount + 1);
        #endif
        delay(500);
        timeoutCount++;
    }

    display.clearDisplay();

    
    timeoutCount = 0;
    while(!bme.begin(0x76, &Wire) && timeoutCount <= 5) {
        #ifdef SERIAL_DBG
            Serial.print("Could not find a valid BME280 sensor, attempt no: ");
            Serial.println(timeoutCount + 1);
        #endif
        delay(500);
        timeoutCount++;
    }

    String bmeStat(bme.sensorID(), DEC);
    #ifdef SERIAL_DBG
        Serial.println("BME status: " + bmeStat);
    #endif

    timeoutCount = 0;
    while(!ens.begin() == NO_ERR && timeoutCount <= 5) {
        #ifdef SERIAL_DBG
            Serial.print("Communication with ENS160 failed, attempt no: ");
            Serial.println(timeoutCount + 1);
        #endif
        delay(500);
        timeoutCount++;
    }

    String ensStat(ens.getENS160Status(), DEC);
    #ifdef SERIAL_DBG
        Serial.println("ENS status: " + ensStat);
    #endif

    Serial.println("Press any key to access configuration termianl or application will start");
    for(int bootCount = 3; bootCount > 0; bootCount--) {
        Serial.println(bootCount);
        delay(1000);
        if(Serial.available()) {
            confTerminal();
            break;
        }
    }
    
    ens.setPWRMode(ENS160_STANDARD_MODE);
    ens.setTempAndHum(bme.readTemperature(), bme.readHumidity());

    digitalWrite(MEM_CS, LOW);
    uint8_t manufacturerID = 0x00;
    uint8_t memType = 0x00;
    uint8_t memCap = 0x00;
    SPI.transfer(0x9F);
    manufacturerID = SPI.transfer(0x55);
    memType = SPI.transfer(0x55);
    memCap = SPI.transfer(55);
    digitalWrite(MEM_CS, HIGH);
    #ifdef SERIAL_DBG
        String manufacturerID_str = String(manufacturerID, HEX);
        String memType_str = String(memType, HEX);
        String memCap_str = String(memCap, HEX);
        Serial.print("Manufacturer ID: ");
        Serial.println(manufacturerID_str);
        Serial.print("Memory type: ");
        Serial.println(memType_str);
        Serial.print("Memory capacity: ");
        Serial.println(memCap_str);
    #endif

    actualAQI = ens.getAQI();
    changeAQI = true;
    delayTime = 500;
    Serial.println();
}


void loop() {

    printValues();
    delay(delayTime);

    #ifdef SERIAL_THRHLD
        if(actualAQI != ens.getAQI()) {
            actualAQI = ens.getAQI();
            changeAQI = true;
            Serial.println();
        }
    #endif
}

template <class T> void printLine(String txt, T val) {

    const int16_t curX = 80;

    display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
    display.print(txt);
    display.setTextColor(SSD1306_WHITE);
    display.fillRect(curX, display.getCursorY(), 50, 8, SSD1306_BLACK);
    display.setCursor(curX, display.getCursorY());
    display.println(val);

    #ifdef SERIAL_DBG
        Serial.print(val);
        Serial.print(",");
    #endif

    #ifdef SERIAL_THRHLD
        if(changeAQI) {
            Serial.print(txt);
            Serial.println(val);
        }
    #endif
}

void printValues() {

    uint16_t getTimeBuff[7] = {0};
    rtcClk.getTime(getTimeBuff);
    char currTime[20];
    char currDate[20];
    sprintf(currDate, " %d/%d/%d ", getTimeBuff[6], getTimeBuff[5], getTimeBuff[4]);
    sprintf(currTime, "%d:%d:%d", getTimeBuff[2], getTimeBuff[1], getTimeBuff[0]);
    String currDate_s(currDate);
    String currTime_s(currTime);

    display.setTextSize(1);
    display.setCursor(0, 0);

    printLine<String>(currDate_s,        currTime_s);
    printLine<float>(" Temp[C]: ",       bme.readTemperature());
    printLine<float>(" Pre[hPa]: ",      bme.readPressure() / 100.0F);
    printLine<float>(" Alt[m]: ",        bme.readAltitude(SEALEVELPRESSURE_HPA));
    printLine<float>(" Hum[%]: ",        bme.readHumidity());

    printLine<uint8_t>(" AQI[-]: ",      ens.getAQI());
    printLine<uint16_t>(" TVOC[ppb]: ",  ens.getTVOC());
    printLine<uint16_t>(" ECO2[ppb]: ",  ens.getECO2());

    #ifdef SERIAL_DBG
        Serial.println();
    #endif

    #ifdef SERIAL_THRHLD
        changeAQI = false;
    #endif

    display.display();
}

typedef enum {
    HELP,
    QUIT,
    CMD_COUNT
} cmd_names;

std::array<String, CMD_COUNT> cmdList = {"Help", "Quit"};

void printAllCmds(void) {
    for(uint8_t cmdName = 0; cmdName < (uint8_t)cmd_names::CMD_COUNT; cmdName++) {
        Serial.println(cmdList[cmdName]);
    }
}

bool confTerminal(void) {
    Serial.println("Welcome in terminal Type 'help' to list commands or 'quit' to discard terminal");
    char cmd[cmdMaxSize];
    while(!Serial.available());
    strncpy(cmd, Serial.readStringUntil('\n').c_str(), sizeof(Serial.readString().c_str())>cmdMaxSize? sizeof(Serial.readString().c_str()) : cmdMaxSize);

    if(strstr(cmd, "help") != NULL) {
        printAllCmds();
        return 0;
    }
    else if(strstr(cmd, "quit"))
        return 0;
    else
        return 1;
}