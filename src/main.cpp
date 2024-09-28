#include <Arduino.h>
#include <Wire.h>

// RFID NFC Reader
#include <Arduino.h>
#include <PN532_SPI.h>
#include <PN532.h>

// LCD library
#include <hd44780.h>
#include <hd44780ioClass/hd44780_I2Clcd.h>

// WiFi Manager Library
#include <WiFiManager.h>
#include <WiFi.h>

// Constants
#include <constants.h>

// Websocket Client
#include <ArduinoWebsockets.h>

// declare the lcd object for auto i2c address location
hd44780_I2Clcd lcd;
PN532_SPI pn532spi(SPI, NFC_NSS);
PN532 nfc(pn532spi);

WiFiManager wm;
websockets::WebsocketsClient ws;

// NFC variables
boolean isNFCReading = false;
uint8_t uid[] = {0, 0, 0, 0, 0, 0, 0};
uint8_t prevUid[] = {0, 0, 0, 0, 0, 0, 0};
uint8_t uidLength;

uint8_t doorPin[] = {
    GPIO_NUM_12, // 0
    GPIO_NUM_14, // 1
    GPIO_NUM_27, // 2
    GPIO_NUM_26, // 3
};

int8_t lastDoorUnlock = -1;
uint32_t lastDoorUnlockTime = 0;

// prototype
void IRAM_ATTR ISR();

String getValue(String data, char separator, int index)
{
  int found = 0;
  int strIndex[] = {0, -1};
  int maxIndex = data.length() - 1;

  for (int i = 0; i <= maxIndex && found <= index; i++)
  {
    if (data.charAt(i) == separator || i == maxIndex)
    {
      found++;
      strIndex[0] = strIndex[1] + 1;
      strIndex[1] = (i == maxIndex) ? i + 1 : i;
    }
  }

  return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}

bool isMessageValid(String message)
{
  // check if the message is valid
  if (message.length() < 3)
  {
    return false;
  }

  // split the message by # and check if machine id is the same
  String machineId = message.substring(0, 14);
  if (machineId != MACHINE_ID)
  {
    return false;
  }

  return true;
}

void onMessageCallback(websockets::WebsocketsMessage message)
{
  Serial.print("Got Message: ");

  String data = message.data();
  Serial.print("[WS]: Got Message: ");
  Serial.println(data);

  if (!isMessageValid(data))
  {
    Serial.println("[WS]: Invalid Message. Ignoring...");
    return;
  }

  String command = getValue(data, '#', 1);
  String value = getValue(data, '#', 2);

  if (command == "NFC_READ")
  {
    // reset uid previous
    memset(prevUid, 0, sizeof(prevUid));
  }

  if (command == "OPEN_DOOR")
  {
    int doorIndex = value.toInt();
    if (doorIndex >= 0 && doorIndex < sizeof(doorPin))
    {
      lastDoorUnlock = doorIndex;
      lastDoorUnlockTime = millis();
      digitalWrite(doorPin[doorIndex], HIGH);
    }
  }

  if (command == "REBOOT")
  {
    Serial.println("[WS]: Rebooting...");
    ESP.restart();
  }

  if (command == "FREE_MEM")
  {
    Serial.print("[WS]: Free Memory: ");
    Serial.println(ESP.getFreeHeap());

    // send the free memory back to the server
    String message = MACHINE_ID;
    message += "#FREE_MEM#";
    message += String(ESP.getFreeHeap());
  }

  // add commands here
}

void onEventsCallback(websockets::WebsocketsEvent event, String data)
{
  if (event == websockets::WebsocketsEvent::ConnectionOpened)
  {
    Serial.println("[WS]: Connnection Opened");
  }
  else if (event == websockets::WebsocketsEvent::ConnectionClosed)
  {
    Serial.println("[WS]: Connnection Closed");
  }
  else if (event == websockets::WebsocketsEvent::GotPing)
  {
    Serial.println("[WS]: Got a Ping!");
  }
  else if (event == websockets::WebsocketsEvent::GotPong)
  {
    Serial.println("[WS]: Got a Pong!");
  }
}

void setup()
{
  // initialize
  Serial.begin(9600);

  Serial.println(F("[SYSTEM]: Starting U-Locker System v1..."));

  Serial.println(F("[POST]: Initializing LCD..."));

  int status = lcd.begin(LCD_COLS, LCD_ROWS);
  if (status) // non zero status means it was unsuccesful
  {
    Serial.print(F("[POST]: LCD begin() failed, status: "));
    Serial.println(status);

    // proceed the application regardless of the LCD status
    // hd44780::fatalError(status);
  }

  // begin NFC reader
  nfc.begin();
  nfc.SAMConfig();

  Serial.println(F("[POST]: Initializing NFC Reader..."));
  uint32_t version = nfc.getFirmwareVersion();

  if (!version)
  {
    Serial.println(F("[POST]: NFC Reader not found."));

    // halt the system
    while (1)
    {
      delay(10);
    }
  }

  Serial.println(F("[POST]: NFC Reader found. "));
  Serial.print("Version: ");
  Serial.print((version >> 24) & 0xFF, HEX);
  Serial.print(", Firmware ver. ");
  Serial.print((version >> 16) & 0xFF, DEC);
  Serial.print('.');
  Serial.println((version >> 8) & 0xFF, DEC);

  // setup NFC IRQ

  Serial.println(F("[SYSTEM]: Initializing NFC IRQ..."));
  pinMode(NFC_IRQ, INPUT_PULLDOWN);
  attachInterrupt(NFC_IRQ, ISR, RISING);

  // setup PINS for door lock

  Serial.println(F("[SYSTEM]: Initializing Door Lock..."));

  for (int i = 0; i < sizeof(doorPin); i++)
  {
    pinMode(doorPin[i], OUTPUT);
    digitalWrite(doorPin[i], LOW);
  }

  // WiFi setup
  Serial.println(F("[SYSTEM]: Initializing WiFi Manager..."));
  wm.setDebugOutput(true);

  // wifi setup
  WiFi.mode(WIFI_STA); // explicitly set mode, esp defaults to STA+AP
  // set timeout for portal, if no access point is found, device will restart
  wm.setConfigPortalTimeout(180);

  // res = wm.autoConnect(); // auto generated AP name from chipid
  bool res = wm.autoConnect("U-Locker IoT"); // anonymous ap

  if (!res)
  {
    Serial.println(F("[WiFi]: Failed to connect. Restarting..."));
    ESP.restart();
  }
  else
  {
    // if you get here you have connected to the WiFi
    Serial.println(F("[WiFi]: Connected to WiFi"));
  }

  // websocket setup
  Serial.println(F("[SYSTEM]: Initializing Websocket Client..."));
  ws.onMessage(onMessageCallback);
  ws.onEvent(onEventsCallback);

  while (!ws.connect(WS_HOST, WS_PORT, WS_PATH))
  {

    // check if wifi is still connected
    if (WiFi.status() != WL_CONNECTED)
    {
      Serial.println(F("[WS]: WiFi Disconnected. Reconnecting..."));
      wm.autoConnect("U-Locker IoT");
    }

    Serial.println(F("[WS]: Connection failed. Retrying..."));
    delay(1000);
  }

  // setup done
  Serial.println(F("[SYSTEM]: Setup done."));
}

void loop(void)
{

  if (isNFCReading)
  {
    isNFCReading = false;

    Serial.println(F("[NFC]: Reading NFC Tag..."));

    // reset uid buffer
    memset(uid, 0, sizeof(uid));

    bool success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength);

    if (success)
    {
      // check if the uid is the same as the previous one
      if (memcmp(uid, prevUid, sizeof(uid)) == 0)
      {
        Serial.println(F("[NFC]: Same NFC Tag detected. Ignoring..."));
        return;
      }

      // save the current uid to the previous uid
      memcpy(prevUid, uid, sizeof(uid));

      Serial.print(F("[NFC]: NFC Tag detected. UID Length: "));
      Serial.print(uidLength, DEC);
      Serial.print(F(", UID Value: "));
      for (uint8_t i = 0; i < uidLength; i++)
      {
        Serial.print(" 0x");
        Serial.print(uid[i], HEX);
      }

      Serial.println();

      // send the uid to the websocket server with this format MACHINE_CODE#NFC_READ#UID
      String message = MACHINE_ID;
      message += "#NFC_READ#";
      for (uint8_t i = 0; i < uidLength; i++)
      {
        message += String(uid[i], HEX);
      }
    }
    else
    {
      Serial.println(F("[NFC]: No NFC Tag found."));
    }
  }

  // door unlock
  if (lastDoorUnlock != -1)
  {
    if (millis() - lastDoorUnlockTime > 5000)
    {
      digitalWrite(doorPin[lastDoorUnlock], LOW);
      lastDoorUnlock = -1;
    }
  }

  ws.poll();

  delay(10);
}

void IRAM_ATTR ISR()
{
  isNFCReading = true;
}

/*
  MACHINE_CODE#COMMAND#DATA
*/