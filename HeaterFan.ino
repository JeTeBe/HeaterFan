#include <TFT_eSPI.h>
#include <SPI.h>
#include "WiFi.h"
#include <Wire.h>
#include "Button2.h"
#include "esp_adc_cal.h"
#include "bmp.h"
#include <OneWire.h>
#include <DallasTemperature.h> 

// TFT Pins has been set in the TFT_eSPI library in the User Setup file TTGO_T_Display.h
// #define TFT_MOSI            19
// #define TFT_SCLK            18
// #define TFT_CS              5
// #define TFT_DC              16
// #define TFT_RST             23
// #define TFT_BL              4   // Display backlight control pin


#define ADC_EN              14  //ADC_EN is the ADC detection enable port
#define ADC_PIN             34
#define BUTTON_1            35
#define BUTTON_2            0

// Data wire is plugged into digital pin 2 on the Arduino
#define ONE_WIRE_BUS_ROOM 2
#define ONE_WIRE_BUS_RAD  3

// Setup a oneWire instance to communicate with any OneWire device
OneWire oneWireRoom(ONE_WIRE_BUS_ROOM);  
OneWire oneWireRad( ONE_WIRE_BUS_RAD);  

// Pass oneWire reference to DallasTemperature library
DallasTemperature sensRoom(&oneWireRoom);
DallasTemperature sensRad(&oneWireRad);

TFT_eSPI tft = TFT_eSPI(135, 240); // Invoke custom library
Button2 btn1(BUTTON_1);
Button2 btn2(BUTTON_2);

String strRoom;
String strRadiator;
String strPWM;
int    tempRoom = 0;
int    tempRadiator = 0;
int    PWM = 0;
float    energy = 0;

#define ENABLE_SPI_SDCARD

//Uncomment will use SDCard, this is just a demonstration,
//how to use the second SPI
#ifdef ENABLE_SPI_SDCARD

#include "FS.h"
#include "SD.h"

SPIClass SDSPI(HSPI);

#define MY_CS       33
#define MY_SCLK     25
#define MY_MISO     27
#define MY_MOSI     26

void setupSDCard()
{
    SDSPI.begin(MY_SCLK, MY_MISO, MY_MOSI, MY_CS);
    //Assuming use of SPI SD card
    if (!SD.begin(MY_CS, SDSPI)) {
        Serial.println("Card Mount Failed");
        tft.setTextColor(TFT_RED);
        tft.drawString("SDCard Mount FAIL", tft.width() / 2, tft.height() / 2 - 32);
        tft.setTextColor(TFT_GREEN);
    } else {
        tft.setTextColor(TFT_GREEN);
        Serial.println("SDCard Mount PASS");
        tft.drawString("SDCard Mount PASS", tft.width() / 2, tft.height() / 2 - 48);
        String size = String((uint32_t)(SD.cardSize() / 1024 / 1024)) + "MB";
        tft.drawString(size, tft.width() / 2, tft.height() / 2 - 32);
    }
}
#else
#define setupSDCard()
#endif


void wifi_scan();

//! Long time delay, it is recommended to use shallow sleep, which can effectively reduce the current consumption
void espDelay(int ms)
{
    esp_sleep_enable_timer_wakeup(ms * 1000);
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON);
    esp_light_sleep_start();
}

void showTemperatures()
{
  int offset=25;
  
  // Send the command to get temperatures
  sensRoom.requestTemperatures(); 
  sensRad.requestTemperatures(); 

  //print the temperature in Celsius
  tempRoom     = sensRoom.getTempCByIndex(0);
  strRoom      = "Ruimte   " + String(tempRoom) + " C";
  tempRadiator = sensRad.getTempCByIndex(0);
  strRadiator  = "Radiator " + String(tempRadiator) + " C";
  strPWM       = "PWM      " + String(PWM/255*100) + "%";
  tft.drawString(strRoom,     0, offset );
  offset = offset + 25;
  tft.drawString(strRadiator, 0, offset );
  offset = offset + 25;
  tft.drawString(strPWM,      0, offset );
  offset = offset + 25;
  tft.drawString("Energy   " + String(energy),     0, offset );
}

void calcPWM()
{
  
}

void calcEnergy()
{
  static uint64_t timeStamp = 0;
  if (millis() - timeStamp > 1000) 
  {
    if ( tempRadiator > tempRoom )
    {
      energy = energy + (tempRadiator - tempRoom) / 1000; 
    }
    timeStamp = millis();
  }
}

void setup()
{
    Serial.begin(115200);
    Serial.println("Start");

    sensRoom.begin();  // Start up the library
    sensRad.begin();  // Start up the library

    pinMode(ADC_EN, OUTPUT);
    digitalWrite(ADC_EN, HIGH);

    tft.init();
    tft.setRotation(1);

    tft.setSwapBytes(true);
    tft.pushImage(0, 0,  240, 135, ttgo);
    espDelay(1000);


    
    tft.setCursor(0, 0);
    tft.setTextSize(2);
    tft.fillScreen(TFT_BLACK);
    
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.drawString("Radiator booster", 0, 0 );
    tft.setTextColor(TFT_GREEN, TFT_BLACK);


    //setupSDCard();
}

void loop()
{
  showTemperatures();
  calcPWM();
  calcEnergy();
}
