#ifndef MAINSERVER_H
#define MAINSERVER_H

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include "global.h"

#define LED1_PIN 48
#define LED2_PIN 41
#define BOOT_PIN 0

extern bool isAPMode;

String mainPage();
String settingsPage();

// ✅ Khai báo hàm startAP để các file khác có thể dùng
void startAP();

void setupServer();
void connectToWiFi();

void main_server_task(void *pvParameters);

#endif