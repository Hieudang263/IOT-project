#ifndef COREIOT_H
#define COREIOT_H

#include <Arduino.h>
#include <PubSubClient.h>

// ✅ Các hàm cơ bản
void coreiot_loop();
bool mqttReconnect();
void publishData(String json);
void mqttCallback(char* topic, byte* payload, unsigned int length);

// ✅ Hàm kiểm tra trạng thái
bool isMQTTConnected();

// ✅ THÊM: Hàm reconnect cho main loop (wrapper)
void CORE_IOT_reconnect();

#endif