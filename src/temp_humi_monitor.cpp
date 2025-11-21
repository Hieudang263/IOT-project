#include "temp_humi_monitor.h"
#include "coreiot.h"  // ✅ THÊM DÒNG NÀY
#include <ArduinoJson.h>

DHT20 dht20;
LiquidCrystal_I2C lcd(33,16,2);


void temp_humi_monitor(void *pvParameters){

    Wire.begin(11, 12);
    Serial.begin(115200);
    dht20.begin();

    while (1){
        dht20.read();
        float temperature = dht20.getTemperature();
        float humidity = dht20.getHumidity();

        if (isnan(temperature) || isnan(humidity)) {
            Serial.println("Failed to read from DHT sensor!");
            temperature = humidity = -1;
        }

        glob_temperature = temperature;
        glob_humidity = humidity;

        Serial.print("Humidity: ");
        Serial.print(humidity);
        Serial.print("%  Temperature: ");
        Serial.print(temperature);
        Serial.println("°C");

        // ✅ THAY THÀNH HÀM publishData (gửi JSON)
        StaticJsonDocument<128> doc;
        doc["temperature"] = temperature;
        doc["humidity"] = humidity;
        
        String jsonData;
        serializeJson(doc, jsonData);
        publishData(jsonData);
        
        vTaskDelay(5000);
    }
    
}