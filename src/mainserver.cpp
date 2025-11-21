#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <LittleFS.h>

#include "global.h"
#include "task_check_info.h"
#include "mainserver.h"

// ✅ LED PWM Config
#define LED1_PIN 48
#define LED2_PIN 41
#define PWM_FREQ 5000
#define PWM_RESOLUTION 8 // 0-255
#define PWM_CHANNEL_1 0
#define PWM_CHANNEL_2 1

// ✅ LED State
struct LEDState {
  bool isOn;
  int brightness; // 0-100
  int pwmValue;   // 0-255
};

LEDState led1 = {false, 50, 127};
LEDState led2 = {false, 50, 127};

// Extern variables
extern String ssid;
extern String password;
extern String wifi_ssid;
extern String wifi_password;
extern bool isWifiConnected;
extern SemaphoreHandle_t xBinarySemaphoreInternet;
extern String WIFI_SSID;
extern String WIFI_PASS;
extern void Save_info_File(String wifi_ssid, String wifi_pass,
                           String token, String server, String port);
// Biến toàn cục sensor (từ global.cpp)
extern float glob_temperature;
extern float glob_humidity;


WebServer server(80);

bool isAPMode = false;
bool connecting = false;
unsigned long connect_start_ms = 0;

// Thêm các biến toàn cục để lưu AP config
String ap_ssid = "ESP32-Setup-Wifi";
String ap_password = "123456789";


// ==================== PWM FUNCTIONS ====================
void setupPWM() {
  ledcSetup(PWM_CHANNEL_1, PWM_FREQ, PWM_RESOLUTION);
  ledcSetup(PWM_CHANNEL_2, PWM_FREQ, PWM_RESOLUTION);
  
  ledcAttachPin(LED1_PIN, PWM_CHANNEL_1);
  ledcAttachPin(LED2_PIN, PWM_CHANNEL_2);
  
  // Start with LEDs OFF
  ledcWrite(PWM_CHANNEL_1, 0);
  ledcWrite(PWM_CHANNEL_2, 0);
  
  Serial.println("✅ PWM initialized (LED1: GPIO48, LED2: GPIO41)");
}

void setLED(int ledNum, bool state, int brightness) {
  LEDState* led = (ledNum == 1) ? &led1 : &led2;
  int channel = (ledNum == 1) ? PWM_CHANNEL_1 : PWM_CHANNEL_2;
  
  led->isOn = state;
  led->brightness = constrain(brightness, 0, 100);
  
  if (state) {
    // Map 0-100 to 0-255
    led->pwmValue = map(led->brightness, 0, 100, 0, 255);
    ledcWrite(channel, led->pwmValue);
    Serial.printf("💡 LED%d ON - Brightness: %d%% (PWM: %d)\n", 
                  ledNum, led->brightness, led->pwmValue);
  } else {
    led->pwmValue = 0;
    ledcWrite(channel, 0);
    Serial.printf("💡 LED%d OFF\n", ledNum);
  }
}

// ==================== HTML PAGES ====================
String mainPage() {
  // ✅ SERVE FROM LITTLEFS (PRIORITY)
  if (LittleFS.exists("/index.html")) {
    File file = LittleFS.open("/index.html", "r");
    if (file) {
      String html = file.readString();
      file.close();
      Serial.println("📄 Serving /index.html from LittleFS");
      return html;
    }
  }
  
  // ❌ FALLBACK: Simple HTML nếu không có file
  Serial.println("⚠️ /index.html not found in LittleFS, using fallback");
  return R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <title>ESP32 - Upload Required</title>
</head>
<body>
  <h1>⚠️ Files Missing</h1>
  <p>Please upload index.html to LittleFS.</p>
</body>
</html>
)rawliteral";
}

String settingsPage() {
  // ✅ MINIMAL SETTINGS PAGE (không cần file riêng)
  return R"rawliteral(
<!DOCTYPE html>
<html lang="vi">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Cấu hình Wi-Fi</title>
  <style>
    /* CSS Giản lược */
    body { font-family: Arial; padding: 20px; }
    input { display: block; margin: 10px 0; padding: 5px; width: 100%; }
    button { padding: 10px; width: 100%; }
  </style>
</head>
<body>
  <h2>⚙️ Cấu hình Wi-Fi</h2>
  <input id="ssid" placeholder="SSID">
  <input id="pass" type="password" placeholder="Password">
  <button onclick="sendConfig()">Kết nối</button>
  
  <script>
    function sendConfig() {
      var ssid = document.getElementById('ssid').value;
      var pass = document.getElementById('pass').value;
      window.location.href = "/connect?ssid=" + encodeURIComponent(ssid) + "&pass=" + encodeURIComponent(pass);
    }
  </script>
</body>
</html>
)rawliteral";
}

// ==================== WIFI SCAN HANDLER (NEW) ====================
void handleScan() {
  Serial.println("\n=== WiFi Scan Request ===");
  
  int n = WiFi.scanNetworks();
  Serial.printf("Found %d networks\n", n);
  
  String json = "{\"networks\":[";
  
  if (n > 0) {
    for (int i = 0; i < n; i++) {
      if (i > 0) json += ",";
      
      json += "{";
      json += "\"ssid\":\"" + WiFi.SSID(i) + "\",";
      json += "\"rssi\":" + String(WiFi.RSSI(i)) + ",";
      json += "\"encryption\":\"";
      
      switch (WiFi.encryptionType(i)) {
        case WIFI_AUTH_OPEN: json += "Open"; break;
        case WIFI_AUTH_WEP: json += "WEP"; break;
        case WIFI_AUTH_WPA_PSK: json += "WPA"; break;
        case WIFI_AUTH_WPA2_PSK: json += "WPA2"; break;
        case WIFI_AUTH_WPA_WPA2_PSK: json += "WPA/WPA2"; break;
        case WIFI_AUTH_WPA2_ENTERPRISE: json += "WPA2-Enterprise"; break;
        default: json += "Unknown";
      }
      
      json += "\"";
      json += "}";
    }
  }
  
  json += "]}";
  
  server.send(200, "application/json", json);
  Serial.println("✅ Scan results sent");
}

// ==================== AP CONFIG HANDLER (NEW) ====================
void handleAPConfig() {
  Serial.println("\n=== AP Config Request ===");
  
  String new_ssid = server.arg("ssid");
  String new_password = server.arg("pass");
  
  Serial.println("New SSID: " + new_ssid);
  
  if (new_ssid.isEmpty()) {
    server.send(400, "text/plain", "SSID không được để trống!");
    return;
  }
  
  if (!new_password.isEmpty() && new_password.length() < 8) {
    server.send(400, "text/plain", "Mật khẩu phải có ít nhất 8 ký tự!");
    return;
  }
  
  // Lưu config vào LittleFS
  File f = LittleFS.open("/ap_config.txt", "w");
  if (f) {
    f.println(new_ssid);
    f.println(new_password);
    f.close();
    Serial.println("💾 AP config saved to file");
  }
  
  server.send(200, "text/plain", "Đã lưu cấu hình AP! ESP32 sẽ khởi động lại...");
  
  delay(1000);
  ESP.restart();
}

// ==================== SENSOR HANDLER (NEW) ====================
void handleSensor() {
  // Serial.println("\n=== Sensor Data Request ==="); // Comment bớt log để đỡ spam
  
  String json = "{";
  
  if (isnan(glob_temperature) || glob_temperature == -1 || 
      isnan(glob_humidity) || glob_humidity == -1) {
    json += "\"error\":true,";
    json += "\"message\":\"Sensor error\",";
    json += "\"temperature\":0,";
    json += "\"humidity\":0";
  } else {
    json += "\"error\":false,";
    json += "\"temperature\":" + String(glob_temperature, 2) + ",";
    json += "\"humidity\":" + String(glob_humidity, 2);
  }
  
  json += "}";
  
  server.send(200, "application/json", json);
}


// ==================== HTTP HANDLERS ====================
void handleRoot() {
  server.send(200, "text/html", mainPage());
}

void handleSettings() {
  server.send(200, "text/html", settingsPage());
}

void handleConnect() {
  Serial.println("\n===== /connect called =====");
  
  wifi_ssid = server.arg("ssid");
  wifi_password = server.arg("pass");
  
  Serial.println("SSID from web: " + wifi_ssid);
  
  // Update global variables
  WIFI_SSID = wifi_ssid;
  WIFI_PASS = wifi_password;
  
  // ✅ SAVE TO FILE
  // Lưu ý: Cần đảm bảo biến CORE_IOT_... có giá trị hoặc truyền chuỗi rỗng nếu chưa có
  Save_info_File(WIFI_SSID, WIFI_PASS, "", "", ""); 
  Serial.println("💾 Saved WiFi to /info.dat");
  
  server.send(200, "text/plain", "Đang kết nối... Vui lòng đợi");
  
  connecting = true;
  connect_start_ms = millis();
  connectToWiFi();
}

void handleControl() {
  int device = server.arg("device").toInt();
  String state = server.arg("state");
  int brightness = server.arg("brightness").toInt();
  
  Serial.printf("📥 Control: LED%d = %s, Brightness = %d%%\n", 
                device, state.c_str(), brightness);
  
  bool isOn = (state == "ON");
  setLED(device, isOn, brightness);
  
  server.send(200, "text/plain", "OK");
}

// ✅ SERVE STATIC FILES FROM LITTLEFS
void handleStaticFile(String path, String contentType) {
  if (LittleFS.exists(path)) {
    File file = LittleFS.open(path, "r");
    if (file) {
      server.streamFile(file, contentType);
      file.close();
      return;
    }
  }
  server.send(404, "text/plain", "File not found");
}

// ==================== WIFI FUNCTIONS ====================
void startAP() {
  Serial.println("\n=== Starting AP Mode ===");
  Serial.println("SSID: " + ap_ssid);
  
  WiFi.mode(WIFI_AP);
  
  if (ap_password.isEmpty() || ap_password.length() < 8) {
     WiFi.softAP(ap_ssid.c_str());
  } else {
     WiFi.softAP(ap_ssid.c_str(), ap_password.c_str());
  }
  
  IPAddress ip = WiFi.softAPIP();
  Serial.print("AP IP: ");
  Serial.println(ip);
  
  isAPMode = true;
}

void connectToWiFi() {
  if (wifi_ssid.isEmpty()) return;
  
  Serial.println("\n=== Connecting to WiFi ===");
  Serial.println("SSID: " + wifi_ssid);
  
  WiFi.mode(WIFI_STA);
  if (wifi_password.isEmpty()) {
    WiFi.begin(wifi_ssid.c_str());
  } else {
    WiFi.begin(wifi_ssid.c_str(), wifi_password.c_str());
  }
}

// ==================== SETUP SERVER (NEW) ====================
void setupServer() {
  // Load AP config from file if exists
  if (LittleFS.exists("/ap_config.txt")) {
    File f = LittleFS.open("/ap_config.txt", "r");
    if (f) {
      ap_ssid = f.readStringUntil('\n');
      ap_password = f.readStringUntil('\n');
      ap_ssid.trim();
      ap_password.trim();
      f.close();
      Serial.println("📄 Loaded AP config: " + ap_ssid);
    }
  }
}

// ==================== MAIN TASK ====================
void main_server_task(void *pvParameters) {
  Serial.println("\n=== Main Server Task Started ===");
  
  setupPWM();
  setupServer(); // Load AP config
  
  // Start AP if not already in AP/STA mode
  if (WiFi.getMode() != WIFI_AP && WiFi.getMode() != WIFI_AP_STA) {
    startAP();
  }
  
  // ✅ Register ALL HTTP routes
  server.on("/", HTTP_GET, handleRoot);
  server.on("/settings", HTTP_GET, handleSettings);
  server.on("/connect", HTTP_GET, handleConnect);
  server.on("/control", HTTP_GET, handleControl);
  
  // New Routes
  server.on("/scan", HTTP_GET, handleScan);       
  server.on("/apconfig", HTTP_GET, handleAPConfig); 
  server.on("/sensor", HTTP_GET, handleSensor);     
  
  // Static files
  server.on("/script.js", HTTP_GET, []() {
    handleStaticFile("/script.js", "application/javascript");
  });
  server.on("/styles.css", HTTP_GET, []() {
    handleStaticFile("/styles.css", "text/css");
  });
  
  server.onNotFound([]() {
    server.send(404, "text/plain", "404 Not Found");
  });
  
  server.begin();
  Serial.println("✅ HTTP server started");
  
  // Main loop
  for (;;) {
    server.handleClient();
    
    if (connecting) {
      wl_status_t st = WiFi.status();
      
      if (st == WL_CONNECTED) {
        Serial.println("\n✅ WiFi STA connected!");
        Serial.print("IP: ");
        Serial.println(WiFi.localIP());
        
        isWifiConnected = true;
        connecting = false;
        isAPMode = false;
        
        if (xBinarySemaphoreInternet != NULL) {
          xSemaphoreGive(xBinarySemaphoreInternet);
        }
      }
      else if (millis() - connect_start_ms > 15000) {
        Serial.println("\n❌ WiFi timeout! Back to AP mode");
        connecting = false;
        isWifiConnected = false;
        WiFi.disconnect(true);
        startAP();
      }
    }
    
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}