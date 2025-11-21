#include "task_rs485.h"

HardwareSerial RS485Serial(1);

#define delay_connect 100
#define TXD_RS485 9
#define RXD_RS485 10

void sendRS485Command(byte *command, int commandSize, byte *response, int responseSize)
{
    RS485Serial.write(command, commandSize);
    RS485Serial.flush();
    delay(100);
    if (RS485Serial.available() >= responseSize)
    {
        RS485Serial.readBytes(response, responseSize);
    }
    else
    {
        Serial.println("Failed to read response - - - - - -");
    }
}

void sendModbusCommand(const uint8_t command[], size_t length)
{
    for (size_t i = 0; i < length; i++)
    {
        Serial2.write(command[i]);
    }
    delay(delay_connect);
}

void _sensor_read()
{
    float sound = 0.0;
    float pressure = 0.0;
    byte response[7];
    byte soundRequest[] = {0x06, 0x03, 0x01, 0xF6, 0x00, 0x01, 0x64, 0x73};
    byte PressureRequest[] = {0x06, 0x03, 0x01, 0xF9, 0x00, 0x01, 0x54, 0x70};

    sendRS485Command(soundRequest, sizeof(soundRequest), response, sizeof(response));
    if (response[1] == 0x03)
    {
        sound = (response[3] << 8) | response[4];
        sound /= 10.0;
    }
    else
    {
        Serial.println("Failed to read sound");
    }

    delay(delay_connect);
    memset(response, 0, sizeof(response));

    sendRS485Command(PressureRequest, sizeof(PressureRequest), response, sizeof(response));
    if (response[1] == 0x03)
    {
        pressure = (response[3] << 8) | response[4];
        pressure /= 10.0;
    }
    else
    {
        Serial.println("Failed to read pressure");
    }

    delay(delay_connect);
    memset(response, 0, sizeof(response));

    Serial.println("sound : " + String(sound));
    Serial.println("pressure: " + String(pressure));
}

void Task_Read_Sensor(void *pvParameters)
{
    while (true)
    {
        _sensor_read();
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

void Task_Send_data(void *pvParameters)
{
    // Relay ON command template
    const uint8_t relay_ON[][8] = {
        {1, 5, 0, 0, 255, 0, 140, 58},  // Relay 0 ON
        {1, 5, 0, 1, 255, 0, 221, 250}, // Relay 1 ON
        {1, 5, 0, 2, 255, 0, 45, 250},  // Relay 2 ON
        {1, 5, 0, 3, 255, 0, 124, 58},  // Relay 3 ON
        {1, 5, 0, 31, 255, 0, 189, 252} // Relay ALL ON
    };

    // Relay OFF command template
    const uint8_t relay_OFF[][8] = {
        {1, 5, 0, 0, 0, 0, 205, 202}, // Relay 0 OFF
        {1, 5, 0, 1, 0, 0, 156, 10},  // Relay 1 OFF
        {1, 5, 0, 2, 0, 0, 108, 10},  // Relay 2 OFF
        {1, 5, 0, 3, 0, 0, 61, 202},  // Relay 3 OFF
        {1, 5, 0, 31, 0, 0, 252, 207} // Relay ALL OFF
    };
    bool state = false; // false = bật, true = tắt

    while (true)
    {
        if (!state)
        {
            Serial.println("🟢 Đang bật từng relay...");
            for (int i = 0; i < 4; i++)
            {
                sendModbusCommand(relay_ON[i], sizeof(relay_ON[i]));
                Serial.println("Bật relay " + String(i));
                vTaskDelay(1000 / portTICK_PERIOD_MS); // Giữ 1 giây giữa mỗi lần bật
            }
        }
        else
        {
            Serial.println("🔴 Đang tắt từng relay...");
            for (int i = 0; i < 4; i++)
            {
                sendModbusCommand(relay_OFF[i], sizeof(relay_OFF[i]));
                Serial.println("Tắt relay " + String(i));
                vTaskDelay(1000 / portTICK_PERIOD_MS); // Giữ 1 giây giữa mỗi lần tắt
            }
        }

        if (!state)
            Serial.println("✅ Hoàn tất bật tất cả relay!");
        else
            Serial.println("✅ Hoàn tất tắt tất cả relay!");

        // Đảo trạng thái cho lần kế tiếp
        state = !state;

        // Nghỉ giữa 2 chu kỳ (3 giây)
        vTaskDelay(3000 / portTICK_PERIOD_MS);
    }
}

void tasksensor_init()
{
    RS485Serial.begin(9600, SERIAL_8N1, TXD_RS485, RXD_RS485);
    xTaskCreate(Task_Read_Sensor, "Task_Read_Sensor", 4096, NULL, 1, NULL);
    xTaskCreate(Task_Send_data, "Task_Send_data", 4096, NULL, 1, NULL);
}