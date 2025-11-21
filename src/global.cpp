#include "global.h"

float glob_temperature = 0;
float glob_humidity = 0;
bool ap_started = false;

String WIFI_SSID;
String WIFI_PASS;
String CORE_IOT_TOKEN;
String CORE_IOT_SERVER;
String CORE_IOT_PORT;

extern String coreiot_server;
extern int    coreiot_port;

// WiFi credentials (mặc định)
String ssid = "ESP32-Setup-Wifi";
String password = "123456789";

// Các giá trị Wi-Fi khác (tuỳ phần code gốc)
String wifi_ssid = "abcde";
String wifi_password = "123456789";

boolean isWifiConnected = false;
SemaphoreHandle_t xBinarySemaphoreInternet = NULL;  // ✅ Chỉ khai báo NULL, sẽ tạo trong setup()