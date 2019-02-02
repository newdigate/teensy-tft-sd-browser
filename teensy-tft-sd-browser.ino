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

// Example use of lfnOpenNext and open by index.
// You can use test files located in
// SdFat/examples/LongFileName/testFiles.a
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

// Try to select the best SD card configuration.
#if HAS_SDIO_CLASS
#define SD_CONFIG SdioConfig(FIFO_SDIO)
#elif ENABLE_DEDICATED_SPI
#define SD_CONFIG SdSpiConfig(SD_CS_PIN, DEDICATED_SPI)
#else  // HAS_SDIO_CLASS
#define SD_CONFIG SdSpiConfig(SD_CS_PIN, SHARED_SPI)
#endif  // HAS_SDIO_CLASS

File file;
File dirFile;

// Number of files found.
uint16_t n = 0;
// Max of ten files since files are selected with a single digit.
const uint16_t nMax = 16;
// Position of file's directory entry.
uint16_t dirIndex[nMax];
char *filenames[nMax];

long currentPage = 0;
long positionLeft  = -999;
long selection = -1;
long last_millis = 0;
long totalFiles = 0;
//------------------------------------------------------------------------------
void setup() {
  Serial.begin(9600);
  while (!Serial) {}
  delay(1000);
  
  if (!SD.begin(SD_CONFIG)) {
    SD.initErrorHalt(&Serial);
  }
  
  tft.initR(INITR_GREENTAB);
  tft.fillScreen(ST7735_BLACK);
  tft.setRotation(1);
  tft.setTextColor(ST7735_BLUE,ST7735_BLACK);
  tft.setTextSize(1);
  tft.setTextWrap(false);
  
  dirFile = SD.open("/");
  
  while (file.openNext(&dirFile, O_RDONLY)) {
    if (!file.isHidden())
      totalFiles++;
    file.close();
  }
  Serial.print("totalFiles:");
  Serial.print(totalFiles, DEC);
  loadDirectory();
  knob.write(0);
  positionLeft = 0;
  displayDirectory();
}

void loadDirectory() {
  //Serial.println("loadDirectory()");
  Serial.print("currentPage:");
  Serial.println(currentPage, DEC);
  
  dirFile.rewind();
  if (currentPage > 0) {

    for (int i=0; i<currentPage; i++){
      int nn = 0;
      while (nn < nMax && file.openNext(&dirFile, O_RDONLY)) {
        if (!file.isHidden()){
          
          Serial.print("skip:");         
          file.printName(&Serial);
          Serial.println();
          nn++;
        }
        file.close();
      }
    }
  }

  for (int i=0; i<nMax; i++) {
    if (filenames[i])
      delete [] filenames[i];
  }
 
  n = 0;
  int total = 0;
  while (n < nMax && file.openNext(&dirFile, O_RDONLY)) {
    // Skip hidden files.
    if (  !file.isHidden()) {
      char filename[32] ;
      file.getName(filename, 32);
      filenames[n] = new char[32];
      strcpy(filenames[n], filename);
      // Save dirIndex of file in directory.
      dirIndex[n] = total + (currentPage*nMax);
      n++;
    }
    total++;
    file.close();
  }
}

void displayDirectory() {
  //Serial.print("displayDirectory()");

  for (int i=0; i<n; i++) {
    // Print the file number and name.
    if (i + currentPage*nMax == positionLeft) {
    
      //selected index
      Serial.print(">>>");
      selectFile(i);
        
    } else {
      
      deselectFile(i);
    }
  }
} 
//------------------------------------------------------------------------------

void loop() {
  long current_millis = millis();
  if (current_millis > last_millis + 100) {
    // put your main code here, to run repeatedly:
    long newLeft;
    newLeft = knob.read() / 4;
    if (newLeft < 0) {
      knob.write(0);
      newLeft = 0;
    } else if (newLeft > totalFiles-1) {
      newLeft = totalFiles-1;
      knob.write(newLeft*4);
    }
    
    long newPage = newLeft / 16;
    
    if (newLeft != positionLeft || newPage != currentPage) {

      Serial.print("pos: ");
      Serial.println(newLeft);
      
      if (newPage != currentPage) {
        currentPage = newPage;
        Serial.print("page:");
        Serial.println(currentPage);
        positionLeft = newLeft; 
        tft.fillScreen(ST7735_BLACK);
        loadDirectory();
        displayDirectory();
      } else {
        deselectFile(positionLeft % nMax);        
        positionLeft = newLeft;         
        selectFile(positionLeft % nMax);
      }

    }
    last_millis = current_millis;
  }
}

void deselectFile(int i) {
  tft.setCursor(1, i * 8 +1 );
  
  file.open(&dirFile,filenames[i], O_RDONLY);
  if (!file.isSubDir()) {
    tft.setTextColor(ST7735_BLUE, ST7735_BLACK);  
  } else
    tft.setTextColor(ST7735_CYAN, ST7735_BLACK);
    
  tft.print(filenames[i]);
  Serial.println(filenames[i]);
  file.close();
}

void selectFile(int i) {
  tft.setCursor(1, i * 8 +1 );
  file.open(&dirFile, filenames[i], O_RDONLY);
  if (!file.isSubDir()) {
    tft.setTextColor(ST7735_MAGENTA, ST7735_BLUE);  
  } else
    tft.setTextColor(ST7735_CYAN, ST7735_BLUE);
  tft.print(filenames[i]);
  file.close();
}
