#include <hd44780.h>
#include <hd44780ioClass/hd44780_I2Cexp.h>

#include <constants.h>

hd44780_I2Cexp lcd;

/**
 * @class LCDController
 * @brief Kelas untuk mengontrol tampilan LCD.
 */
class LCDController
{
private:
    uint32_t lastLCDWrite = 0;

public:
    /**
     * @brief Menginisialisasi LCD dengan jumlah kolom dan baris yang ditentukan.
     * @return True jika inisialisasi berhasil, false jika gagal.
     */
    bool setup()
    {
        int status = lcd.begin(LCD_COLS, LCD_ROWS);
        lcd.clear();
        lcd.setCursor(4, 2);
        lcd.lineWrap();
        lcd.print("U-Locker IoT");

        return status;
    };

    /**
     * @brief Menampilkan pesan pada LCD.
     * @param message Pesan yang akan ditampilkan pada LCD.
     */
    void print(String message)
    {

        lastLCDWrite = millis();
        lcd.clear();
        lcd.print(message);
    }

    void loop()
    {
        if (millis() - lastLCDWrite >= LCD_SCREENTIME)
        {
            lcd.clear();
            lcd.setCursor(4, 2);
            lcd.print("U-Locker IoT");
        }
    }
};