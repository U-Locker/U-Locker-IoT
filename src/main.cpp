#include <Arduino.h>
#include <Wire.h>

// RFID NFC Reader
#include <Arduino.h>
#include <PN532_SPI.h>
#include <PN532.h>

// // LCD library
// #include <hd44780.h>
// #include <hd44780ioClass/hd44780_I2Clcd.h>

#define NFC_NSS 5
#define NFC_IRQ 17

#define LCD_COLS 20
#define LCD_ROWS 4

// // declare the lcd object for auto i2c address location
// hd44780_I2Clcd lcd;
PN532_SPI pn532spi(SPI, NFC_NSS);
PN532 nfc(pn532spi);

// NFC variables
boolean isNFCReading = false;
uint8_t uid[] = {0, 0, 0, 0, 0, 0, 0};
uint8_t uidLength;

uint8_t doorPin[] = {
    GPIO_NUM_12,
    GPIO_NUM_14,
    GPIO_NUM_27,
    GPIO_NUM_26,
};

// prototype
void IRAM_ATTR ISR();

void setup()
{
  Serial.begin(9600);
  Serial.println("Starting...");
  // int status = lcd.begin(LCD_COLS, LCD_ROWS);
  // if (status) // non zero status means it was unsuccesful
  // {
  //   // begin() failed so blink error code using the onboard LED if possible
  //   hd44780::fatalError(status); // does not return
  // }

  nfc.begin();
  nfc.SAMConfig();

  uint32_t version = nfc.getFirmwareVersion();
  Serial.print("Version: ");
  Serial.println((version >> 24) & 0xFF, HEX);
  Serial.print("Firmware ver. ");
  Serial.print((version >> 16) & 0xFF, DEC);
  Serial.print('.');
  Serial.println((version >> 8) & 0xFF, DEC);

  attachInterrupt(NFC_IRQ, ISR, RISING);
}

void loop(void)
{

  uint8_t success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength);

  if (success)
  {
    // Display some basic information about the card
    Serial.println("Found an ISO14443A card");
    Serial.print("  UID Length: ");
    Serial.print(uidLength, DEC);
    Serial.println(" bytes");
    Serial.print("  UID Value: ");
    nfc.PrintHex(uid, uidLength);

    if (uidLength == 4)
    {
      // We probably have a Mifare Classic card ...
      uint32_t cardid = uid[0];
      cardid <<= 8;
      cardid |= uid[1];
      cardid <<= 8;
      cardid |= uid[2];
      cardid <<= 8;
      cardid |= uid[3];
      Serial.print("Seems to be a Mifare Classic card #");
      Serial.println(cardid);
    }
    Serial.println("");
  }
}

void IRAM_ATTR ISR()
{
  isNFCReading = true;
}