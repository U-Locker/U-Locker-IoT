/*
    Constants untuk digunakan dalam program
*/

#define MACHINE_ID "0cfa-4ed7-a8d7"

// MQTT API
#define MQTT_SERVER "broker.emqx.io"
#define MQTT_PORT 1883
#define MQTT_TOPIC_COMMAND "/u-locker/command"
#define MQTT_TOPIC_RESPONSE "/u-locker/response"
#define MQTT_HEARTBEAT_INTERVAL 60000

// PINOUT

// NFC
#define NFC_NSS 5
// #define NFC_IRQ 17

// LCD
#define LCD_COLS 20
#define LCD_ROWS 4
#define LCD_SCREENTIME 5000

// Buzzer
#define BUZZER_PIN 33
#define BUZZER_FREQ 5000
#define BUZZER_DELAY 150