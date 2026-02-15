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
int currentScreen = 0;
unsigned long lastTouchTime = 0;

void setup() {
    Serial.begin(115200);

    tft.init();
    tft.setRotation(1); 
    tft.invertDisplay(true); 
    tft.fillScreen(TFT_BLACK);

    uint16_t calData[5] = { 346, 3546, 273, 3487, 7 };
    tft.setTouch(calData);

    tft.setTextColor(TFT_WHITE);
    tft.setTextSize(2);
    tft.setCursor(20, 100);
    tft.print("Starting Monitor...");

    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    tft.fillScreen(TFT_BLACK);
    webSocket.begin(server_host, server_port, "/");
    webSocket.onEvent(webSocketEvent);
    webSocket.setReconnectInterval(5000);
}

void loop() {
    webSocket.loop();
    handleTouch();
    drawConnectionStatus();
}

void handleTouch() {
    uint16_t t_x = 0, t_y = 0;
    bool pressed = tft.getTouch(&t_x, &t_y);

    if (pressed && (millis() - lastTouchTime > 500)) {
        lastTouchTime = millis();
        currentScreen = (currentScreen + 1) % 3; 
        tft.fillScreen(TFT_BLACK);
        drawHeader();
    }
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
    tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
    tft.setTextSize(1);
    tft.setCursor(20, 15);
    
    if (currentScreen == 0) tft.print("SYSTEM OVERVIEW    [1/3]");
    else if (currentScreen == 1) tft.print("STORAGE USAGE      [2/3]");
    else if (currentScreen == 2) tft.print("NETWORK THROUGHPUT  [3/3]");
    
    tft.drawFastHLine(20, 30, 280, 0x2104);
}

void updateDashboard(char* json) {
    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, json);
    if (error) return;

    if (currentScreen == 0) {
        drawModernBar(20, 65, 200, 14, doc["cpu"], "PROCESSOR LOAD");
        drawModernBar(20, 125, 200, 14, doc["mem"], "MEMORY USAGE");
        
        int temp = doc["temp"];
        tft.setCursor(20, 175); tft.setTextSize(1); tft.setTextColor(TFT_DARKGREY, TFT_BLACK); tft.print("CORE TEMP");
        tft.setCursor(20, 190); tft.setTextSize(3);
        tft.setTextColor(temp > 70 ? TFT_RED : TFT_CYAN, TFT_BLACK);
        
        if (temp < 10) tft.print(" ");
        tft.print(temp); tft.setTextSize(1); tft.print(" oC ");
        
        long uptime = doc["uptime"];
        tft.setCursor(160, 175); tft.setTextSize(1); tft.setTextColor(TFT_DARKGREY, TFT_BLACK); tft.print("SYS UPTIME");
        tft.setCursor(160, 190); tft.setTextSize(2); tft.setTextColor(TFT_WHITE, TFT_BLACK);
        int d = uptime / 86400; int h = (uptime % 86400) / 3600;
        char uptimeStr[16]; snprintf(uptimeStr, sizeof(uptimeStr), "%dd %dh %02dm ", d, h, (int)((uptime % 3600) / 60));
        tft.print(uptimeStr);

    } else if (currentScreen == 1) {
        drawModernBar(20, 100, 220, 30, doc["disk"], "ROOT PARTITION (/)");
        tft.setCursor(20, 160);

    } else if (currentScreen == 2) {
        tft.setTextSize(1); tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
        tft.setCursor(20, 60); tft.print("DOWNLOAD");
        
        tft.setCursor(20, 75); 
        tft.setTextSize(3); 
        tft.setTextColor(TFT_GREEN, TFT_BLACK);
        
        int nIn = doc["netIn"];
        char inStr[15];
        snprintf(inStr, sizeof(inStr), "%-4d", nIn);
        tft.print(inStr);
        tft.setTextSize(1); tft.print(" KB/s    ");

        tft.setCursor(20, 130); tft.setTextSize(1); tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
        tft.print("UPLOAD");
        
        tft.setCursor(20, 145); 
        tft.setTextSize(3); 
        tft.setTextColor(TFT_ORANGE, TFT_BLACK);
        
        int nOut = doc["netOut"];
        char outStr[15];
        snprintf(outStr, sizeof(outStr), "%-4d", nOut); 
        tft.print(outStr);
        tft.setTextSize(1); tft.print(" KB/s    ");
    }
}

void drawModernBar(int x, int y, int w, int h, int percent, String label) {
    tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
    tft.setTextSize(1);
    tft.setCursor(x, y - 15);
    tft.print(label);

    uint16_t color = (percent > 90) ? TFT_RED : (percent > 75 ? TFT_YELLOW : TFT_GREEN);
    tft.fillRect(x, y, w, h, 0x2104); 
    int fillWidth = map(percent, 0, 100, 0, w);
    if (fillWidth > 0) tft.fillRect(x, y, fillWidth, h, color);

    tft.setTextSize(2);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setCursor(x + w + 12, y - 2);
    if (percent < 100) tft.print(" ");
    if (percent < 10) tft.print(" ");
    tft.print(percent); tft.print("%");
}

void drawConnectionStatus() {
    static unsigned long lastPulse = 0;
    static bool pulseState = false;
    if (millis() - lastPulse > 800) {
        lastPulse = millis();
        pulseState = !pulseState;
        uint16_t dotColor = isConnected ? (pulseState ? TFT_GREEN : 0x03E0) : TFT_RED;
        tft.fillCircle(310, 10, 3, dotColor);
    }
}
