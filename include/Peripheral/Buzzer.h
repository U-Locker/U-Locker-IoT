
#include <Arduino.h>
#include <constants.h>

/**
 * @class Buzzer
 * @brief Kelas untuk mengendalikan buzzer.
 */
class Buzzer
{

public:
    /**
     * @brief Mengatur pin buzzer sebagai output.
     */
    void setup()
    {
        pinMode(BUZZER_PIN, OUTPUT);
    }

    /**
     * @brief Memainkan bunyi beep pada buzzer.
     */
    void playBeep(int times)
    {
        for (int i = 0; i < times; i++)
        {
            tone(BUZZER_PIN, BUZZER_FREQ, BUZZER_DELAY);
            delay(BUZZER_DELAY);
        }
    }
};