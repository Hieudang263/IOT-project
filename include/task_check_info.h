#ifndef __TASK_CHECK_INFO_H__
#define __TASK_CHECK_INFO_H__

#include <ArduinoJson.h>
#include "LittleFS.h"
#include "global.h"
// ✅ BỎ include task_wifi.h vì nó dẫn đến include ESPAsyncWebServer
// #include "task_wifi.h"  // ❌ XÓA DÒNG NÀY

bool check_info_File(bool check);
void Load_info_File();
void Delete_info_File();
void Save_info_File(String WIFI_SSID, String WIFI_PASS, String CORE_IOT_TOKEN, String CORE_IOT_SERVER, String CORE_IOT_PORT);

#endif