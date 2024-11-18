
// PubSubClient
#include <PubSubClient.h>
#include <WiFiClient.h>
#include <constants.h>

class MQTTHandler
{

private:
    WiFiClient wifi;
    const char *server;
    int port;
    const char *commandTopic;
    const char *responseTopic;
    uint32_t lastHeartbeatSent = 0;

public:
    /**
     * @class MQTTHandler
     * @brief Kelas untuk menangani koneksi dan komunikasi MQTT.
     *
     * Kelas ini menyediakan metode untuk mengatur koneksi ke server MQTT,
     * mengirim dan menerima pesan, serta memproses pesan yang diterima.
     */

    /**
     * @brief Konstruktor untuk kelas MQTTHandler.
     *
     * @param server Alamat server MQTT.
     * @param port Port server MQTT.
     * @param commandTopic Topik untuk menerima perintah.
     * @param responseTopic Topik untuk mengirim respons.
     */

    PubSubClient client;

    MQTTHandler(const char *server, int port, const char *commandTopic, const char *responseTopic)
    {
        this->server = server;
        this->port = port;
        this->commandTopic = commandTopic;
        this->responseTopic = responseTopic;
    }

    /**
     * @brief Memulai koneksi MQTT dan mengatur callback untuk pesan yang diterima.
     *
     * @param callback Fungsi callback yang akan dipanggil saat pesan diterima.
     */
    void begin(std::function<void(char *, uint8_t *, unsigned int)> callback)
    {
        client.setServer(server, port);
        client.setCallback(callback);

        sendResponse("STARTUP", "");
    };

    /**
     * @brief Menghubungkan kembali ke server MQTT jika koneksi terputus.
     *
     * Metode ini akan mencoba menghubungkan kembali ke server MQTT jika koneksi terputus.
     * Jika koneksi berhasil, metode ini akan berlangganan ke topik perintah.
     */
    void reconnect()
    {
        // while (!client.connected())
        // {
        Serial.print("Attempting MQTT connection...");
        // Create a random client ID
        String clientId = "Locker-";
        clientId += String(random(0xffff), HEX);
        // Attempt to connect
        if (client.connect(clientId.c_str()))
        {
            Serial.println("connected");
            client.subscribe(this->commandTopic);
        }
        else
        {
            Serial.print("failed, rc=");
            Serial.print(client.state());
            Serial.println(" try again in 5 seconds");
            // Wait 5 seconds before retrying
            delay(5000);
        }
        // }
    };

    /**
     * @brief Memproses loop MQTT.
     *
     * Metode ini harus dipanggil secara berkala untuk memastikan koneksi tetap aktif
     * dan pesan diproses.
     */
    void loop()
    {
        if (!client.connected())
        {
            reconnect();
        }

        client.loop();

        // send heartbeat to indicate that this device is still alive
        if (millis() - lastHeartbeatSent >= MQTT_HEARTBEAT_INTERVAL)
        {
            sendResponse("HEARTBEAT", "");
            lastHeartbeatSent = millis();
        }
    };

    /**
     * @brief Mendapatkan nilai dari string yang dipisahkan oleh separator.
     *
     * @param data String yang akan diproses.
     * @param separator Karakter pemisah.
     * @param index Indeks nilai yang akan diambil.
     * @return String Nilai yang diambil dari string.
     */
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

    /**
     * @brief Memeriksa apakah pesan valid.
     *
     * @param message Pesan yang akan diperiksa.
     * @param selfMachineId ID mesin sendiri untuk validasi.
     * @return true Jika pesan valid.
     * @return false Jika pesan tidak valid.
     */
    bool isMessageValid(String message, String selfMachineId)
    {
        // check if the message is valid
        if (message.length() < 15)
        {
            return false;
        }

        // split the message by # and check if machine id is the same
        String machineId = message.substring(0, 14);
        return machineId != selfMachineId;
    }

    /**
     * @brief Mengirim pesan ke topik tertentu.
     *
     * @param topic Topik tujuan.
     * @param message Pesan yang akan dikirim.
     */
    void publish(const char *topic, const char *message)
    {
        client.publish(topic, message);
    }

    /**
     * @brief Mengirim respons ke topik respons.
     *
     * @param command Perintah yang akan dikirim.
     * @param value Nilai tambahan untuk perintah.
     */
    void sendResponse(String command, String value)
    {
        String message = MACHINE_ID;
        message += "#";
        message += command;

        // check if value is not empty
        if (value != "")
        {
            message += "#";
            message += value;
        }

        publish(responseTopic, message.c_str());
    }
};