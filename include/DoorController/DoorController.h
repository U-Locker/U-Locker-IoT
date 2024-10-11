// #include <Arduino.h>

#define DOOR_TIMEOUT 5000

/**
 * @class DoorController
 * @brief Class untuk mengontrol pintu menggunakan pin GPIO.
 *
 * Class ini menyediakan method untuk mengatur pin GPIO sebagai output,
 * membuka pintu, dan memantau status pintu dalam loop utama.
 *
 * @var doorPin
 * Array yang menyimpan nomor pin GPIO untuk setiap pintu.
 *
 * @var lastDoorUnlock
 * Array yang menyimpan index pintu terakhir yang dibuka.
 *
 * @var lastDoorUnlockTime
 * Array yang menyimpan waktu terakhir pintu dibuka.
 */

class DoorController
{
private:
    uint8_t doorPin[4] = {
        GPIO_NUM_12, // 0
        GPIO_NUM_14, // 1
        GPIO_NUM_27, // 2
        GPIO_NUM_26, // 3
    };

    int8_t lastDoorUnlock[4] = {-1, -1, -1, -1};
    uint32_t lastDoorUnlockTime[4] = {0, 0, 0, 0};

public:
    /**
     * @brief Mengatur pin GPIO sebagai output dan mengatur status awal pin.
     *
     * Method ini harus dipanggil dalam setup() untuk menginisialisasi pin GPIO
     * yang digunakan untuk mengontrol pintu.
     */
    void setup()
    {
        for (int i = 0; i < sizeof(doorPin); i++)
        {
            pinMode(doorPin[i], OUTPUT);
            digitalWrite(doorPin[i], HIGH);
        }
    }

    /**
     * @brief Memantau status pintu dan menutup pintu jika waktu timeout tercapai.
     *
     * Method ini harus dipanggil dalam loop() untuk memeriksa apakah pintu perlu
     * ditutup berdasarkan waktu timeout yang telah ditentukan.
     */
    void loop()
    {
        for (int i = 0; i < sizeof(doorPin); i++)
        {
            if (lastDoorUnlock[i] != -1)
            {
                if (millis() - lastDoorUnlockTime[i] > DOOR_TIMEOUT)
                {
                    digitalWrite(doorPin[lastDoorUnlock[i]], HIGH);
                    lastDoorUnlock[i] = -1;
                }
            }
        }
    }

    /**
     * @brief Membuka pintu berdasarkan index pintu yang diberikan.
     *
     * @param doorIndex index pintu yang akan dibuka (0-3).
     *
     * Method ini akan membuka pintu yang sesuai dengan index yang diberikan,
     * mengatur waktu pembukaan, dan mengatur status pin GPIO untuk membuka pintu.
     * Jika index pintu tidak valid, akan mencetak pesan error ke Serial.
     */
    void unlockDoor(int doorIndex)
    {
        if (doorIndex >= 0 && doorIndex < sizeof(doorPin))
        {
            lastDoorUnlock[doorIndex] = doorIndex;
            lastDoorUnlockTime[doorIndex] = millis();
            digitalWrite(doorPin[doorIndex], LOW);
        }
        else
        {
            Serial.println("Invalid door index");
        }
    }
};