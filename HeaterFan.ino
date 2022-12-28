
#include <TFT_eSPI.h>
#include <SPI.h>
#include "WiFi.h"
#include <Wire.h>
#include "Button2.h"
#include "esp_adc_cal.h"
#include "bmp.h"
#include <OneWire.h>
#include <DallasTemperature.h> 
//#include <pwmWrite.h>

// TFT Pins has been set in the TFT_eSPI library in the User Setup file TTGO_T_Display.h
// #define TFT_MOSI         19
// #define TFT_SCLK         18
// #define TFT_CS           5
// #define TFT_DC           16
// #define TFT_RST          23
// #define TFT_BL           4   // Display backlight control pin

/* Setting PWM Properties */
const int PWMFreq = 5000; /* 5 KHz */
const int PWMChannel = 0;
const int PWMResolution = 8;
const int MAX_DUTY_CYCLE = (int)(pow(2, PWMResolution) - 1);
const int MIN = 0;
const int MAX = 255;

#define ADC_EN              14  //ADC_EN is the ADC detection enable port
#define ADC_PIN             34
#define BUTTON_1            35
#define BUTTON_2            0
#define PWM_PIN             25


// Data wire is plugged into digital pin 2 on the Arduino
#define ONE_WIRE_BUS_ROOM   32
#define ONE_WIRE_BUS_RAD    33

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
int    tempRoom =      0;
int    tempRadiator =  0;
int    PWM =           MIN;
int    oldPWM =        0;
float  energy =        0;
int btnClick = false;
int btnClickUp = false;
int btnClickDown = false;

const float freq = 100;
const byte resolution = 10;
//Pwm pwm = Pwm();
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
  static uint64_t timeStamp = 0;
  if (millis() - timeStamp > 3000) 
  {
    // Send the command to get temperatures
    sensRoom.requestTemperatures(); 
    sensRad.requestTemperatures(); 
  
    //print the temperature in Celsius
    tempRoom     = sensRoom.getTempCByIndex(0);
    strRoom      = "Ruimte   " + String(tempRoom) + " C   ";
    tempRadiator = sensRad.getTempCByIndex(0);
    strRadiator  = "Radiator " + String(tempRadiator) + " C   ";
    strPWM       = "PWM      " + String(PWM) + "   ";
    tft.drawString(strRoom,     0, offset );
    offset = offset + 25;
    tft.drawString(strRadiator, 0, offset );
    offset = offset + 25;
    tft.drawString(strPWM,      0, offset );
    offset = offset + 25;
    tft.drawString("Energy   " + String(energy) + "    ",     0, offset );
    timeStamp = millis();
  }
}

#define PWM_MAX 255

void calcPWM()
{
  if (btnClick == false)
  {
    if (tempRadiator >= (tempRoom + 2))
    {
      PWM = (tempRadiator - tempRoom ) * 10;
      Serial.println(PWM);
      Serial.println(PWM_MAX);
      if (PWM > PWM_MAX)
      {
        PWM = PWM_MAX;
      }
      Serial.println(PWM);
    }
    else
    {
      PWM = 0;
    }
  }
  //PWM = PWM + 1;
  //delay(15000);
  if ( PWM > MAX )
  {
    PWM = MIN;
  }
  //pwm.write(PWM_PIN, 341, freq, resolution, PWM);
  if ( oldPWM != PWM )
  {
    ledcWrite(PWMChannel, PWM);
    //dacWrite(25,150);
    //delay(200);
    //dacWrite(25,PWM);
    oldPWM = PWM;
  }
}

void calcEnergy()
{
  static uint64_t timeStamp = 0;
  if (millis() - timeStamp > 3000) 
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
  int deviceCount;
  Serial.begin(9600);
  Serial.println("Start");

  sensRoom.begin();  // Start up the library
  sensRad.begin();  // Start up the library

// locate devices on the bus
  Serial.print("Locating devices...");
  Serial.print("Room sensors ");
  deviceCount = sensRoom.getDeviceCount();
  Serial.print(deviceCount, DEC);
  Serial.println(" devices.");
  Serial.print("Radiator sensors ");
  deviceCount = sensRad.getDeviceCount();
  Serial.print(deviceCount, DEC);
  Serial.println(" devices.");
  Serial.println("");

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

  // //Declaring LED pin as output
  pinMode(PWM_PIN, OUTPUT);
  
  ledcSetup(PWMChannel, PWMFreq, PWMResolution);
  /* Attach the LED PWM Channel to the GPIO Pin */
  ledcAttachPin(PWM_PIN, PWMChannel);

  //setupSDCard();
  button_init();
}

void button_init()
{
  btn1.setPressedHandler([](Button2 & b) 
  {
    btnClickDown = true;
    btnClick = true;
  });

  btn2.setPressedHandler([](Button2 & b) 
  {
    btnClickUp = true;
    btnClick = true;
  });
}
void button_loop()
{
    btn1.loop();
    btn2.loop();
}
void loop()
{
  showTemperatures();
  calcPWM();
  //calcEnergy();
  button_loop();
  if (btnClickDown == true)
  {
    Serial.println("Down..");
    btnClickDown = false;
    PWM = PWM - 1;
  }
  if (btnClickUp == true)
  {
    Serial.println("Up..");
    btnClickUp = false;
    PWM = PWM + 1;
  }
}
