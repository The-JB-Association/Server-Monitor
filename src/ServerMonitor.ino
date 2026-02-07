#include <WiFi.h>
#include <WebSocketsClient.h>
#include <ArduinoJson.h>
#include <TFT_eSPI.h>
#include <SPI.h>

const char* ssid = "YOUR SSID";
const char* password = "YOUR WIFI PASSWORD";
const char* server_ip = "YOUR WEBSOCKET SERVER IP";
const uint16_t server_port = 8080;

TFT_eSPI tft = TFT_eSPI(); 
WebSocketsClient webSocket;

bool isConnected = false;
unsigned long lastUpdate = 0;

void setup() {
    Serial.begin(115200);

    tft.init();
    tft.setRotation(1);
    tft.invertDisplay(true);
    tft.fillScreen(TFT_BLACK);

    tft.setTextColor(TFT_WHITE);
    tft.setTextSize(2);
    tft.setCursor(20, 100);
    tft.print("Initializing Link...");

    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    tft.fillScreen(TFT_BLACK);
    tft.setCursor(20, 100);
    tft.print("Establishing WebSocket...");

    webSocket.begin(server_ip, server_port, "/");
    webSocket.onEvent(webSocketEvent);
    webSocket.setReconnectInterval(5000);
}

void loop() {
    webSocket.loop();
    drawConnectionStatus();
}

void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
    switch(type) {
        case WStype_DISCONNECTED:
            isConnected = false;
            break;
        case WStype_CONNECTED:
            isConnected = true;
            tft.fillScreen(TFT_BLACK);
            drawHeader();
            break;
        case WStype_TEXT:
            updateDashboard((char*)payload);
            break;
    }
}

void drawHeader() {
    tft.setTextColor(TFT_DARKGREY);
    tft.setTextSize(1);
    tft.setCursor(20, 15);
    tft.print("SERVER MONITOR");
    tft.drawFastHLine(20, 30, 280, TFT_DARKGREY);
}

void updateDashboard(char* json) {
    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, json);
    if (error) return;

    int cpu = doc["cpu"];
    int mem = doc["mem"];
    int temp = doc["temp"];
    long uptime = doc["uptime"];

    drawModernBar(20, 65, 200, 14, cpu, "PROCESSOR LOAD");
    drawModernBar(20, 125, 200, 14, mem, "MEMORY USAGE");

    tft.setCursor(20, 175);
    tft.setTextSize(1);
    tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
    tft.print("CORE TEMP");
    
    tft.setCursor(20, 190);
    tft.setTextSize(3);
    uint16_t tColor = (temp > 70) ? TFT_RED : (temp > 55 ? TFT_ORANGE : TFT_CYAN);
    tft.setTextColor(tColor, TFT_BLACK); 
    
    if (temp < 10) tft.print(" ");
    tft.print(temp);
    tft.setTextSize(1);
    tft.print(" oC "); 

    tft.setCursor(160, 175);
    tft.setTextSize(1);
    tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
    tft.print("SYS UPTIME");
    
    tft.setCursor(160, 190);
    tft.setTextSize(2);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);

    int d = uptime / 86400;
    int h = (uptime % 86400) / 3600;
    int m = (uptime % 3600) / 60;

    char uptimeStr[16];
    snprintf(uptimeStr, sizeof(uptimeStr), "%dd %dh %02dm", d, h, m);
    tft.print(uptimeStr);
}

void drawModernBar(int x, int y, int w, int h, int percent, String label) {
    tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
    tft.setTextSize(1);
    tft.setCursor(x, y - 15);
    tft.print(label);

    uint16_t color = 0x07E0;
    if (percent > 75) color = 0xFDE0;
    if (percent > 90) color = 0xF800;

    tft.fillRect(x, y, w, h, 0x2104);

    int fillWidth = map(percent, 0, 100, 0, w);
    if (fillWidth > 0) {
        tft.fillRect(x, y, fillWidth, h, color);
    }

    tft.setTextSize(2);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setCursor(x + w + 12, y - 2);
    if (percent < 100) tft.print(" "); 
    if (percent < 10) tft.print(" "); 
    tft.print(String(percent) + "%");
}

void drawConnectionStatus() {
    static unsigned long lastPulse = 0;
    static bool pulseState = false;
    
    if (millis() - lastPulse > 800) {
        lastPulse = millis();
        pulseState = !pulseState;
        
        uint16_t dotColor = isConnected ? (pulseState ? TFT_GREEN : 0x03E0) : TFT_RED;
        tft.fillSmoothCircle(300, 15, 3, dotColor);
    }
}
