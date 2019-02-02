// Set USE_SD_H nonzero to use SD.h.
// Set USE_SD_H zero to use SdFs.h.
//
#define USE_SD_H 0
//
#if USE_SD_H
#include <SD.h>
#else  // USE_SD_H
#include "SdFs.h"
SdFat SD;
#endif  // USE_SD_H

#include<SPI.h>

#define ENC2A 29 
#define ENC2B 30 
#define BUTTON1 25  //NEXT
#include <Bounce.h>
#include <Encoder.h>
Bounce button0 = Bounce(BUTTON1, 15);
Encoder knob(ENC2A, ENC2B);

#define TFT_SCLK 14  // SCLK can also use pin 14
#define TFT_MOSI 7  // MOSI can also use pin 7
#define TFT_CS   6  // CS & DC can use pins 2, 6, 9, 10, 15, 20, 21, 22, 23
#define TFT_DC    12  //  but certain pairs must NOT be used: 2+10, 6+9, 20+23, 21+22
#define TFT_RST   17  // RST can use any pin

#include <Adafruit_GFX.h>    // Core graphics library
//#include <Adafruit_ST7735.h> // Hardware-specific library
#include <ST7735_t3.h> // Hardware-specific library
#include <SPI.h>

// Option 1: use any pins but a little slower
//Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);
ST7735_t3 tft = ST7735_t3(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);


// SDCARD_SS_PIN is defined for the built-in SD on some boards.
#ifndef SDCARD_SS_PIN
const uint8_t SD_CS_PIN = SS;
#else  // SDCARD_SS_PIN
// Assume built-in SD is used.
const uint8_t SD_CS_PIN = SDCARD_SS_PIN;
#endif  // SDCARD_SS_PIN

#if USE_SD_H
// using regular SD library
#elif HAS_SDIO_CLASS
#define SD_CONFIG SdioConfig(FIFO_SDIO)
#elif ENABLE_DEDICATED_SPI
#define SD_CONFIG SdSpiConfig(SD_CS_PIN, DEDICATED_SPI)
#else  // HAS_SDIO_CLASS
#define SD_CONFIG SdSpiConfig(SD_CS_PIN, SHARED_SPI)
#endif  // HAS_SDIO_CLASS


#define USE_SD_H 0
#include "TFTSdDirectoryBrowser.h"

TFTSdDirectoryBrowser browser(&tft, &SD, 16, 
  ST7735_BLUE, ST7735_BLACK, 
  ST7735_BLUE, ST7735_WHITE, 
  ST7735_GREEN, ST7735_BLACK,
  ST7735_GREEN, ST7735_WHITE);
long last_millis = 0;
long pos = 0;
//------------------------------------------------------------------------------
void setup() {
  pinMode(BUTTON1, INPUT_PULLUP);
    
  Serial.begin(9600);
  while (!Serial) {}

  #if USE_SD_H
  if (!SD.begin(BUILTIN_SDCARD)) {
  }
  #else
  if (!SD.begin(SD_CONFIG)) {
    SD.initErrorHalt(&Serial);
  }
  #endif

  tft.initR(INITR_GREENTAB);
  tft.fillScreen(ST7735_BLACK);
  tft.setRotation(1);
  tft.setTextColor(ST7735_BLUE,ST7735_BLACK);
  tft.setTextSize(1);
  tft.setTextWrap(false);

  browser.initialize();
  Serial.print("totalFiles:");
  Serial.print(browser.getTotalFileCount(), DEC);
  browser.reload();
  browser.update();
  knob.write(0);
  pos = 0;
}


//------------------------------------------------------------------------------

void loop() {
  long current_millis = millis();
  if (current_millis > last_millis + 100) {
    //Serial.print(".");
    // put your main code here, to run repeatedly:
    long newLeft;
    newLeft = knob.read() / 4;
    if (newLeft < 0) {
      knob.write(0);
      newLeft = 0;
    } else if (newLeft > browser.getTotalFileCount()-1) {
      newLeft = browser.getTotalFileCount()-1;
      knob.write(newLeft*4);
    }
    
    if (newLeft != pos) {
      pos = newLeft;
      browser.setSelectFileIndex(pos);     
    }
    last_millis = current_millis;
  }
  button0.update();
  if ( button0.fallingEdge()) { 
     Serial.print("SELECTED: ");
     Serial.println( browser.selectedFilename());
  }
}
