#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include "config_coreiot.h"

WiFiClient mqttClient;
PubSubClient client(mqttClient);

unsigned long lastReconnectAttempt = 0;
String topicCommand;
String topicTelemetry;

void mqttCallback(char* topic, byte* payload, unsigned int length) {
    Serial.printf("📩 MQTT [%s] => ", topic);
    
    String message = "";
    for (int i = 0; i < length; i++) {
        message += (char)payload[i];
    }
    Serial.println(message);
    
    // TODO: Xử lý commands từ CoreIOT
}

bool mqttReconnect() {
    // ✅ Validate config đầy đủ
    if (coreiot_server == "" || coreiot_port == 0) {
        static bool logged = false;
        if (!logged) {
            Serial.println("❌ CoreIOT: Thiếu server/port");
            logged = true;
        }
        return false;
    }

    if (coreiot_client_id == "" || coreiot_username == "") {
        static bool logged = false;
        if (!logged) {
            Serial.println("❌ CoreIOT: Thiếu Client ID hoặc Username");
            Serial.println("💡 Vui lòng vào Settings để cấu hình");
            logged = true;
        }
        return false;
    }

    // ✅ Check WiFi
    if (!WiFi.isConnected() || WiFi.getMode() != WIFI_STA) {
        return false;
    }

    Serial.println("\n========================================");
    Serial.printf("🔌 MQTT connecting to %s:%d\n", coreiot_server.c_str(), coreiot_port);

    // ✅ Setup MQTT
    client.setServer(coreiot_server.c_str(), coreiot_port);
    client.setCallback(mqttCallback);

    // ✅ Topics based on username
    topicCommand = coreiot_username + "/commands";
    topicTelemetry = coreiot_username + "/telemetry";

    Serial.println("📋 MQTT Credentials:");
    Serial.println("   Client ID: " + coreiot_client_id);
    Serial.println("   Username: " + coreiot_username);
    Serial.println("   Password: " + String(coreiot_password.length() > 0 ? "***" : "(empty)"));
    Serial.println("   Command: " + topicCommand);
    Serial.println("   Telemetry: " + topicTelemetry);

    // ✅ MQTT BASIC AUTHENTICATION
    bool connected = false;
    
    if (coreiot_password.length() > 0) {
        // Có password
        connected = client.connect(coreiot_client_id.c_str(), 
                                   coreiot_username.c_str(), 
                                   coreiot_password.c_str(), 
                                   NULL, 0, false, NULL);
    } else {
        // Không có password (anonymous with username)
        Serial.println("⚠️ Connecting without password...");
        connected = client.connect(coreiot_client_id.c_str(), 
                                   coreiot_username.c_str(), 
                                   "", 
                                   NULL, 0, false, NULL);
    }

    if (connected) {
        Serial.println("✅ MQTT connected!");
        
        // ✅ Subscribe
        if (client.subscribe(topicCommand.c_str())) {
            Serial.println("✅ Subscribed: " + topicCommand);
        } else {
            Serial.println("⚠️ Subscribe failed");
        }
        
        Serial.println("========================================\n");
        return true;
    }

    // ✅ Connection failed
    int rc = client.state();
    Serial.printf("❌ MQTT failed rc=%d\n", rc);
    Serial.println("\n📋 Error codes:");
    Serial.println("   rc=-4: Timeout");
    Serial.println("   rc=-3: Connection lost");
    Serial.println("   rc=-2: Connection failed");
    Serial.println("   rc=1: Wrong protocol");
    Serial.println("   rc=2: Client ID rejected");
    Serial.println("   rc=3: Server unavailable");
    Serial.println("   rc=4: Bad username/password");
    Serial.println("   rc=5: Not authorized");
    Serial.println("\n💡 Check:");
    Serial.println("   1. Client ID, Username, Password correct?");
    Serial.println("   2. Device activated on CoreIOT?");
    Serial.println("   3. Server & Port correct?");
    Serial.println("========================================\n");
    
    return false;
}

void coreiot_loop() {
    if (!client.connected()) {
        unsigned long now = millis();
        if (now - lastReconnectAttempt > 5000) {
            lastReconnectAttempt = now;
            if (mqttReconnect()) {
                lastReconnectAttempt = 0;
            }
        }
        return;
    }

    client.loop();
}

void publishData(String json) {
    if (!client.connected()) {
        Serial.println("⚠️ MQTT not connected");
        return;
    }
    
    if (topicTelemetry.length() == 0) {
        topicTelemetry = coreiot_username + "/telemetry";
    }
    
    if (client.publish(topicTelemetry.c_str(), json.c_str(), false)) {
        Serial.println("✅ Published: " + json);
    } else {
        Serial.println("❌ Publish failed");
    }
}

bool isMQTTConnected() {
    return client.connected();
}

void CORE_IOT_reconnect() {
    if (!isMQTTConnected()) {
        mqttReconnect();
    }
}