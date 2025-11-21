#include <Arduino.h>
#include <WiFi.h>
#include "coreiot.h"
#include "config_coreiot.h"
#include "global.h"

void task_mqtt(void *pv) {

    Serial.println("=== MQTT task start ===");

    // ✅ Đợi semaphore Internet trước khi chạy MQTT
    Serial.println("⏳ Đợi WiFi kết nối...");
    if (xBinarySemaphoreInternet != NULL) {
        xSemaphoreTake(xBinarySemaphoreInternet, portMAX_DELAY);
        Serial.println("✅ WiFi đã sẵn sàng, bắt đầu MQTT task");
    }

    for (;;) {
        // ---------------------------------------------------------
        // ✅ Điều kiện hợp lệ để chạy MQTT loop:
        // - WiFi đã kết nối
        // - Server hợp lệ
        // - Port hợp lệ
        // - Client ID & Username hợp lệ
        // ---------------------------------------------------------
        if (WiFi.isConnected() &&
            coreiot_server != "" &&
            coreiot_port > 0 &&
            coreiot_client_id != "" &&
            coreiot_username != "") 
        { 
            coreiot_loop(); 
        } 
        else 
        {
            // 🔁 Log 1 lần duy nhất
            static bool logged = false;
            if (!logged) {

                // Trường hợp thiếu cấu hình MQTT
                if (coreiot_server == "" || 
                    coreiot_port == 0 || 
                    coreiot_client_id == "" || 
                    coreiot_username == "") 
                {
                    Serial.println("⚠️ CoreIOT config chưa đầy đủ, vui lòng vào Settings để cấu hình");
                } 
                else 
                {
                    // Trường hợp WiFi lỗi
                    Serial.println("⚠️ WiFi mất kết nối, đợi reconnect...");
                }

                logged = true;
            }
        }
        
        vTaskDelay(5000 / portTICK_PERIOD_MS); // Check mỗi 5s
    }
}
