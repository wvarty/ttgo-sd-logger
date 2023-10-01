#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include <EEPROM.h>

#define EEPROM_SIZE 1

// SD SPI pins
#define SD_CS 13
#define SD_SCK 14
#define SD_MOSI 15
#define SD_MISO 2

SPIClass sd_spi(HSPI);

char filename[50];
File fd;

Stream *sdCardLogger;
#define GPIO_PIN_SDLOG_RX 23
#define GPIO_PIN_SDLOG_TX 19

#define BUF_MAX 2048
uint8_t buf[BUF_MAX];
uint16_t bufferLen = 0;

void setup()
{
  Serial.begin(460800);

  Stream *serialPort;
  serialPort = new HardwareSerial(2);
  ((HardwareSerial *)serialPort)->begin(460800, SERIAL_8N1, GPIO_PIN_SDLOG_RX, GPIO_PIN_SDLOG_TX);
  sdCardLogger = serialPort;

  // start the EEPROM
  EEPROM.begin(EEPROM_SIZE);

  // Get the current log number from address 0 and increment it for the new log
  int currentFilenum = EEPROM.read(0);
  currentFilenum++;

  // start the SD card
  sd_spi.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);

  if (!SD.begin(SD_CS, sd_spi))
  {
    Serial.println("SD Card: mounting failed");
  } 
  else
  {
    Serial.println("SD Card: mounted successfully");
  }

  sprintf(filename, "/log %d.csv", currentFilenum);
  fd = SD.open(filename, FILE_APPEND);

  if (!fd)
  {
    Serial.println("SD Card: writing file failed");
  }
  else
  {
    // write the column header
    fd.println("UPTIME");
    fd.close();
    // save the current log num to EEPROM
    EEPROM.write(0, currentFilenum);
    EEPROM.commit();
    Serial.printf("SD Card: writing successful. New log created with num = %d", currentFilenum);
  }
}

void loop()
{
  while(sdCardLogger->available())
  {
    // Read single character and save it in the buffer
    char c = sdCardLogger->read();

    if (bufferLen >= BUF_MAX)
    {
      return;
    }

    buf[bufferLen++] = c;

    // If character saved in the buffer is \n, save this line to the sd and flush the buffer
    if (c == '\n')
    {
      fd = SD.open(filename, FILE_APPEND);
      fd.print(millis());
      fd.print(',');
      fd.write(buf, bufferLen);
      fd.close();
      bufferLen = 0;
    }
  }
}