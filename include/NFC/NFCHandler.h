#include <PN532_SPI.h>
#include <PN532.h>
#include <constants.h>

PN532_SPI pn532spi(SPI, NFC_NSS);
PN532 nfc(pn532spi);

/**
 * @class NFCHandler
 * @brief Kelas untuk menangani operasi NFC menggunakan modul PN532.
 *
 * Kelas ini menyediakan metode untuk menginisialisasi modul NFC, membaca UID dari tag NFC,
 * dan mereset UID yang telah dibaca sebelumnya.
 */
class NFCHandler
{
private:
    int nss;
    uint8_t uid[7] = {0, 0, 0, 0, 0, 0, 0};
    uint8_t prevUid[7] = {0, 0, 0, 0, 0, 0, 0};
    uint8_t uidLength;
    bool readNFCSuccess = false;

public:
    /**
     * @brief Menginisialisasi modul NFC.
     *
     * Metode ini memulai komunikasi dengan modul NFC, memeriksa versi firmware, dan mengonfigurasi
     * modul untuk membaca tag RFID.
     *
     * @return true jika inisialisasi berhasil, false jika tidak.
     */
    bool setup()
    {
        nfc.begin();
        uint32_t versiondata = nfc.getFirmwareVersion();
        if (!versiondata)
        {
            Serial.print("Didn't find PN53x board");
            return false;
        }
        // Got ok data, print it out!
        Serial.print("Found chip PN5");
        Serial.println((versiondata >> 24) & 0xFF, HEX);
        Serial.print("Firmware ver. ");
        Serial.print((versiondata >> 16) & 0xFF, DEC);
        Serial.print('.');
        Serial.println((versiondata >> 8) & 0xFF, DEC);

        // configure board to read RFID tags
        nfc.SAMConfig();
        return true;
    }

    /**
     * @brief Membaca UID dari tag NFC.
     *
     * Metode ini membaca UID dari tag NFC yang berada dalam jangkauan. Jika UID yang dibaca sama
     * dengan UID yang dibaca sebelumnya, metode ini akan mengembalikan string kosong. Jika UID
     * berbeda, metode ini akan mengembalikan UID dalam format string heksadesimal.
     *
     * @return String yang berisi UID dalam format heksadesimal, atau string kosong jika UID sama
     * dengan yang sebelumnya.
     */
    String readNFC()
    {
        readNFCSuccess = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength);
        if (readNFCSuccess)
        {
            // check if the uid is the same as the previous one
            if (memcmp(uid, prevUid, sizeof(uid)) == 0)
            {
                return "";
            }

            // save the current uid to the previous uid
            memcpy(prevUid, uid, sizeof(uid));

            String uidStr = "";
            for (uint8_t i = 0; i < uidLength; i++)
            {
                uidStr += " 0x";
                uidStr += String(uid[i], HEX);
            }

            return uidStr;
        }
        return "";
    }

    /**
     * @brief Mereset UID yang telah dibaca sebelumnya.
     *
     * Metode ini mengatur ulang UID yang telah dibaca sebelumnya menjadi nol.
     */
    void resetUid()
    {
        memset(prevUid, 0, sizeof(prevUid));
    }
};