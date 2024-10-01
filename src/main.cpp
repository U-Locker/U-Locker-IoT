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

// PubSubClient
#include <PubSubClient.h>

// Constants
#include <constants.h>

WiFiClient wifi;
PubSubClient client(wifi);

unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE (50)
char msg[MSG_BUFFER_SIZE];
int value = 0;

// declare the lcd object for auto i2c address location
hd44780_I2Clcd lcd;

// NFC Reader
PN532_SPI pn532spi(SPI, NFC_NSS);
PN532 nfc(pn532spi);

WiFiManager wm;

// NFC variables
// boolean isNFCReading = false;
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

bool readNFCSuccess = false;

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
  if (machineId != String(MACHINE_ID))
  {
    return false;
  }

  return true;
}

void onMessageCallback(char *topic, byte *payload, uint16_t length)
{

  // convert c string to String
  String data = "";
  for (int i = 0; i < length; i++)
  {
    data += (char)payload[i];
  }

  Serial.print("[MQTT]: Got Message: ");
  Serial.println(data);

  if (!isMessageValid(data))
  {
    Serial.println("[MQTT]: Invalid Message. Ignoring...");
    return;
  }

  String command = getValue(data, '#', 1);
  String value = getValue(data, '#', 2);

  Serial.println("[MQTT]: Command: " + command);
  Serial.println("[MQTT]: Value: " + value);

  if (command == "NFC_READ")
  {
    // reset uid previous
    memset(prevUid, 0, sizeof(prevUid));

    String message = MACHINE_ID;
    message += "#NFC_READ#";
    message += "OK";

    // send ack
    client.publish(MQTT_TOPIC_RESPONSE, message.c_str());
  }

  if (command == "OPEN_DOOR")
  {
    int doorIndex = value.toInt();
    if (doorIndex >= 0 && doorIndex < sizeof(doorPin))
    {
      lastDoorUnlock = doorIndex;
      lastDoorUnlockTime = millis();
      digitalWrite(doorPin[doorIndex], LOW);
    }

    // send the door status back to the server
    String message = MACHINE_ID;
    message += "#OPEN_DOOR#";
    message += String(doorIndex);

    client.publish(MQTT_TOPIC_RESPONSE, message.c_str());
  }

  if (command == "REBOOT")
  {
    Serial.println("[MQTT]: Rebooting...");

    String message = MACHINE_ID;
    message += "#REBOOT#";
    message += "OK";

    client.publish(MQTT_TOPIC_RESPONSE, message.c_str());
    ESP.restart();
  }

  if (command == "FREE_MEM")
  {
    Serial.print("[MQTT]: Free Memory: ");
    Serial.println(ESP.getFreeHeap());

    // send the free memory back to the server
    String message = MACHINE_ID;
    message += "#FREE_MEM#";
    message += String(ESP.getFreeHeap());

    client.publish(MQTT_TOPIC_RESPONSE, message.c_str());
  }

  // add commands here
}

void reconnect()
{
  while (!client.connected())
  {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "Locker-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str()))
    {
      Serial.println("connected");
      client.subscribe(MQTT_TOPIC_COMMAND);
    }
    else
    {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }

    // check wifi connector
    if (WiFi.status() != WL_CONNECTED)
    {
      Serial.println(F("[WiFi]: WiFi Disconnected. Reconnecting..."));
      wm.autoConnect("U-Locker IoT");

      // check if wifi is connected
      if (WiFi.status() == WL_CONNECTED)
      {
        Serial.println(F("[WiFi]: Connected to WiFi"));
      }
    }
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

  // PAEL BELE NFC KITA GA DI DIBAWA
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

  // Serial.println(F("[SYSTEM]: Initializing NFC IRQ..."));
  // pinMode(NFC_IRQ, INPUT_PULLDOWN);
  // attachInterrupt(NFC_IRQ, ISR, RISING);

  // setup PINS for door lock

  Serial.println(F("[SYSTEM]: Initializing Door Lock..."));

  for (int i = 0; i < sizeof(doorPin); i++)
  {
    pinMode(doorPin[i], OUTPUT);
    digitalWrite(doorPin[i], HIGH);
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

  Serial.println(F("[SYSTEM]: Setting up MQTT client..."));
  client.setServer(MQTT_SERVER, MQTT_PORT);
  client.setCallback(onMessageCallback);

  // setup done
  Serial.println(F("[SYSTEM]: Setup done."));
}

void loop(void)
{

  if (!client.connected())
  {
    reconnect();
  }

  client.loop();

  // if (isNFCReading)
  // {
  //   isNFCReading = false;

  // Serial.println(F("[NFC]: Reading NFC Tag..."));

  // // reset uid buffer
  // memset(uid, 0, sizeof(uid));

  readNFCSuccess = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength);

  if (readNFCSuccess)
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

    client.publish(MQTT_TOPIC_RESPONSE, message.c_str());
  }
  // else
  // {
  //   Serial.println(F("[NFC]: No NFC Tag found."));
  // }
  // }

  // door unlock
  if (lastDoorUnlock != -1)
  {
    if (millis() - lastDoorUnlockTime > 5000)
    {
      digitalWrite(doorPin[lastDoorUnlock], HIGH);
      lastDoorUnlock = -1;
    }
  }

  delay(10);
}

// void IRAM_ATTR ISR()
// {
//   isNFCReading = true;
// }

/*
  MACHINE_CODE#COMMAND#DATA
*/