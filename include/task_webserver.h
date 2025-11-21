#ifndef TASK_WEBSERVER_H
#define TASK_WEBSERVER_H

#include <ESPAsyncWebServer.h>
#include "LittleFS.h"
#include <AsyncTCP.h>
#include <ArduinoJson.h>
#include <ElegantOTA.h>

// ✅ Thêm hàm xử lý WebSocket message
void handleWebSocketMessage(String message);

void Webserver_stop();
void Webserver_reconnect();
void Webserver_sendata(String data);

#endif