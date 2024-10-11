#include <Arduino.h>
#include <Wire.h>

// WiFi Manager Library
#include <WiFiManager.h>

// Constants
#include <constants.h>

// modular libraries
#include <DoorController/DoorController.h>
#include <MQTT/MQTTHandler.h>
#include <NFC/NFCHandler.h>

WiFiManager wm;
DoorController doorHandler;
MQTTHandler mqttHandler(MQTT_SERVER, MQTT_PORT, MQTT_TOPIC_COMMAND, MQTT_TOPIC_RESPONSE);
NFCHandler nfcHandler;

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

  if (!mqttHandler.isMessageValid(data, MACHINE_ID))
  {
    Serial.println("[MQTT]: Invalid Message. Ignoring...");
    return;
  }

  String command = mqttHandler.getValue(data, '#', 1);
  String value = mqttHandler.getValue(data, '#', 2);

  Serial.println("[MQTT]: Command: " + command);
  Serial.println("[MQTT]: Value: " + value);

  if (command == "NFC_READ")
  {
    // reset uid previous
    nfcHandler.resetUid();

    // send ack
    mqttHandler.sendResponse("NFC_READ", "OK");
  }

  if (command == "OPEN_DOOR")
  {
    int doorIndex = value.toInt();
    doorHandler.unlockDoor(doorIndex);

    // sementara reset UID kalo pintu kebuka
    nfcHandler.resetUid();

    // send the door status back to the server
    mqttHandler.sendResponse("OPEN_DOOR", String(doorIndex));
  }

  if (command == "REBOOT")
  {
    Serial.println("[MQTT]: Rebooting...");

    // send response
    mqttHandler.sendResponse("REBOOT", "OK");
    ESP.restart();
  }

  if (command == "FREE_MEM")
  {
    Serial.print("[MQTT]: Free Memory: ");
    Serial.println(ESP.getFreeHeap());

    // send the free memory back to the server
    mqttHandler.sendResponse("FREE_MEM", String(ESP.getFreeHeap()));
  }

  // add commands here
}

void setup()
{
  // initialize
  Serial.begin(9600);

  Serial.println(F("[SYSTEM]: Starting U-Locker System v1..."));
  Serial.println(F("[POST]: Initializing LCD..."));

  // NFC Setup
  if (!nfcHandler.setup())
  {
    Serial.println(F("[SYSTEM]: NFC setup failed. "));
    while (1)
    {

      // send report to server

      delay(10);
    }
  }

  // Door Controller Setup
  doorHandler.setup();

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
  mqttHandler.begin(onMessageCallback);

  // setup done
  Serial.println(F("[SYSTEM]: Setup done."));
}

void loop(void)
{
  // handle mqtt
  mqttHandler.loop();

  // handle nfc
  String uid = nfcHandler.readNFC();
  if (uid != "")
  {
    Serial.print("[NFC]: UID: ");
    Serial.println(uid);

    // send response
    mqttHandler.sendResponse("NFC_READ", uid);
  }

  // handle door controller
  doorHandler.loop();

  // WDT: watchdog timer
  delay(10);
}