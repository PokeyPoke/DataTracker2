#include "display.h"
#include "config.h"
#include <WiFi.h>

DisplayManager::DisplayManager()
    : u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE), currentState(SPLASH),
      currentBrightness(255), brightnessIncreasing(false) {
}

void DisplayManager::init() {
    // Initialize I2C with custom pins if defined
    #ifdef SDA_PIN
    Wire.begin(SDA_PIN, SCL_PIN);
    #endif

    u8g2.begin();
    u8g2.enableUTF8Print();
    Serial.println("Display initialized");
}

void DisplayManager::clear() {
    u8g2.clearBuffer();
}

void DisplayManager::drawCenteredText(const char* text, int y, const uint8_t* font) {
    u8g2.setFont(font);
    int width = u8g2.getStrWidth(text);
    u8g2.drawStr((128 - width) / 2, y, text);
}

void DisplayManager::drawCenteredValue(const char* value, int y) {
    u8g2.setFont(u8g2_font_logisoso24_tn);  // Large numeric font
    int width = u8g2.getStrWidth(value);
    u8g2.drawStr((128 - width) / 2, y, value);
}

void DisplayManager::drawHeader(const char* title) {
    u8g2.setFont(u8g2_font_helvB08_tr);
    u8g2.drawStr(2, 10, title);
}

void DisplayManager::drawStatusBar(bool wifiConnected, unsigned long lastUpdate, bool isStale) {
    u8g2.setFont(u8g2_font_6x10_tr);

    // WiFi indicator
    if (wifiConnected) {
        u8g2.drawStr(2, 62, "W");  // WiFi connected
    } else {
        u8g2.drawStr(2, 62, "X");  // WiFi disconnected
    }

    // Timestamp
    String timeAgo = getTimeAgo(lastUpdate);
    u8g2.drawStr(15, 62, timeAgo.c_str());

    // Stale indicator
    if (isStale) {
        u8g2.drawStr(115, 62, "!");
    }
}

void DisplayManager::showSplash() {
    u8g2.clearBuffer();

    drawCenteredText("DATA TRACKER", 28, u8g2_font_helvB10_tr);
    drawCenteredText("v2.6.4", 42, u8g2_font_6x10_tr);
    drawCenteredText("Auto-Fetch", 54, u8g2_font_5x7_tr);

    u8g2.sendBuffer();
    currentState = SPLASH;
}

void DisplayManager::showConnecting(const char* ssid) {
    u8g2.clearBuffer();

    drawCenteredText("CONNECTING", 25, u8g2_font_helvB08_tr);
    drawCenteredText(ssid, 40, u8g2_font_6x10_tr);
    drawCenteredText("Please wait...", 55, u8g2_font_6x10_tr);

    u8g2.sendBuffer();
    currentState = CONNECTING;
}

void DisplayManager::showConfigMode(const char* apName) {
    u8g2.clearBuffer();

    u8g2.setFont(u8g2_font_helvB08_tr);
    u8g2.drawStr(25, 15, "SETUP MODE");

    u8g2.setFont(u8g2_font_6x10_tr);
    u8g2.drawStr(5, 30, "1. Connect to WiFi:");
    u8g2.drawStr(10, 42, apName);

    u8g2.drawStr(5, 57, "2. Open browser");

    u8g2.sendBuffer();
    currentState = CONFIG_MODE;
}

void DisplayManager::showError(const char* message) {
    u8g2.clearBuffer();

    u8g2.setFont(u8g2_font_helvB08_tr);
    u8g2.drawStr(40, 20, "ERROR");

    u8g2.setFont(u8g2_font_6x10_tr);
    int width = u8g2.getStrWidth(message);
    u8g2.drawStr((128 - width) / 2, 40, message);

    u8g2.sendBuffer();
    currentState = ERROR_STATE;
}

void DisplayManager::showBitcoin(float price, float change24h, unsigned long lastUpdate, bool stale) {
    // Get crypto name from config and convert to uppercase
    JsonObject module = config["modules"]["bitcoin"];
    String cryptoNameStr = module["cryptoName"] | "Bitcoin";
    cryptoNameStr.toUpperCase();

    u8g2.clearBuffer();

    // Header with actual crypto name
    drawHeader(cryptoNameStr.c_str());

    // Price
    char priceStr[16];
    if (price >= 10000) {
        snprintf(priceStr, sizeof(priceStr), "$%.0f", price);
    } else if (price >= 1000) {
        snprintf(priceStr, sizeof(priceStr), "$%.1f", price);
    } else {
        snprintf(priceStr, sizeof(priceStr), "$%.2f", price);
    }
    drawCenteredValue(priceStr, 38);

    // Change percentage
    u8g2.setFont(u8g2_font_helvB08_tr);
    char changeStr[20];
    const char* sign = (change24h >= 0) ? "+" : "-";
    snprintf(changeStr, sizeof(changeStr), "%s%.1f%% (24h)", sign, fabs(change24h));
    int changeWidth = u8g2.getStrWidth(changeStr);
    u8g2.drawStr((128 - changeWidth) / 2, 50, changeStr);

    // Status bar
    drawStatusBar(WiFi.isConnected(), lastUpdate, stale);

    u8g2.sendBuffer();
    currentState = NORMAL;
}

void DisplayManager::showEthereum(float price, float change24h, unsigned long lastUpdate, bool stale) {
    // Get crypto name from config and convert to uppercase
    JsonObject module = config["modules"]["ethereum"];
    String cryptoNameStr = module["cryptoName"] | "Ethereum";
    cryptoNameStr.toUpperCase();

    u8g2.clearBuffer();

    // Header with actual crypto name
    drawHeader(cryptoNameStr.c_str());

    // Price
    char priceStr[16];
    if (price >= 1000) {
        snprintf(priceStr, sizeof(priceStr), "$%.0f", price);
    } else {
        snprintf(priceStr, sizeof(priceStr), "$%.2f", price);
    }
    drawCenteredValue(priceStr, 38);

    // Change percentage
    u8g2.setFont(u8g2_font_helvB08_tr);
    char changeStr[20];
    const char* sign = (change24h >= 0) ? "+" : "-";
    snprintf(changeStr, sizeof(changeStr), "%s%.1f%% (24h)", sign, fabs(change24h));
    int changeWidth = u8g2.getStrWidth(changeStr);
    u8g2.drawStr((128 - changeWidth) / 2, 50, changeStr);

    // Status bar
    drawStatusBar(WiFi.isConnected(), lastUpdate, stale);

    u8g2.sendBuffer();
    currentState = NORMAL;
}

void DisplayManager::showStock(const char* ticker, float price, float change, unsigned long lastUpdate, bool stale) {
    u8g2.clearBuffer();

    // Header with ticker
    drawHeader(ticker);

    // Price
    char priceStr[16];
    snprintf(priceStr, sizeof(priceStr), "$%.2f", price);
    drawCenteredValue(priceStr, 38);

    // Change percentage
    u8g2.setFont(u8g2_font_helvB08_tr);
    char changeStr[20];
    const char* sign = (change >= 0) ? "+" : "-";
    snprintf(changeStr, sizeof(changeStr), "%s%.1f%% (today)", sign, fabs(change));
    int changeWidth = u8g2.getStrWidth(changeStr);
    u8g2.drawStr((128 - changeWidth) / 2, 50, changeStr);

    // Status bar
    drawStatusBar(WiFi.isConnected(), lastUpdate, stale);

    u8g2.sendBuffer();
    currentState = NORMAL;
}

void DisplayManager::showWeather(float temp, const char* condition, const char* location, unsigned long lastUpdate, bool stale) {
    u8g2.clearBuffer();

    // Header
    drawHeader("WEATHER");

    // Temperature
    char tempStr[16];
    snprintf(tempStr, sizeof(tempStr), "%.1f", temp);

    u8g2.setFont(u8g2_font_logisoso24_tn);
    int tempWidth = u8g2.getStrWidth(tempStr);
    u8g2.drawStr((128 - tempWidth - 24) / 2, 35, tempStr);

    // Degree symbol and C (add more spacing to prevent overlap)
    u8g2.setFont(u8g2_font_helvB10_tr);
    u8g2.drawStr((128 - tempWidth - 24) / 2 + tempWidth + 6, 35, "C");
    u8g2.drawCircle((128 - tempWidth - 24) / 2 + tempWidth + 2, 18, 2);

    // Condition
    u8g2.setFont(u8g2_font_helvB08_tr);
    int condWidth = u8g2.getStrWidth(condition);
    u8g2.drawStr((128 - condWidth) / 2, 46, condition);

    // Location
    u8g2.setFont(u8g2_font_6x10_tr);
    int locWidth = u8g2.getStrWidth(location);
    u8g2.drawStr((128 - locWidth) / 2, 56, location);

    // Status bar
    drawStatusBar(WiFi.isConnected(), lastUpdate, stale);

    u8g2.sendBuffer();
    currentState = NORMAL;
}

void DisplayManager::showCustom(float value, const char* label, const char* unit, unsigned long lastUpdate) {
    u8g2.clearBuffer();

    // Header with label
    drawHeader(label);

    // Value
    char valueStr[16];
    snprintf(valueStr, sizeof(valueStr), "%.2f", value);
    drawCenteredValue(valueStr, 38);

    // Unit
    if (strlen(unit) > 0) {
        u8g2.setFont(u8g2_font_helvB08_tr);
        int unitWidth = u8g2.getStrWidth(unit);
        u8g2.drawStr((128 - unitWidth) / 2, 50, unit);
    }

    // Status bar (never stale for manual entry)
    drawStatusBar(WiFi.isConnected(), lastUpdate, false);

    u8g2.sendBuffer();
    currentState = NORMAL;
}

void DisplayManager::showModule(const char* moduleId) {
    JsonObject module = config["modules"][moduleId];
    unsigned long lastUpdate = module["lastUpdate"] | 0;
    bool stale = isCacheStale(moduleId);

    if (strcmp(moduleId, "bitcoin") == 0) {
        float price = module["value"] | 0.0;
        float change = module["change24h"] | 0.0;
        const char* cryptoName = module["cryptoName"] | "Bitcoin";
        Serial.print("Bitcoin module - Name: ");
        Serial.print(cryptoName);
        Serial.print(", Price: $");
        Serial.println(price);
        showBitcoin(price, change, lastUpdate, stale);
    }
    else if (strcmp(moduleId, "ethereum") == 0) {
        float price = module["value"] | 0.0;
        float change = module["change24h"] | 0.0;
        showEthereum(price, change, lastUpdate, stale);
    }
    else if (strcmp(moduleId, "stock") == 0) {
        const char* ticker = module["ticker"] | "STOCK";
        float price = module["value"] | 0.0;
        float change = module["change"] | 0.0;
        showStock(ticker, price, change, lastUpdate, stale);
    }
    else if (strcmp(moduleId, "weather") == 0) {
        float temp = module["temperature"] | 0.0;
        const char* condition = module["condition"] | "Unknown";
        const char* location = module["location"] | "Unknown";
        showWeather(temp, condition, location, lastUpdate, stale);
    }
    else if (strcmp(moduleId, "custom") == 0) {
        float value = module["value"] | 0.0;
        const char* label = module["label"] | "CUSTOM";
        const char* unit = module["unit"] | "";
        showCustom(value, label, unit, lastUpdate);
    }
    else if (strcmp(moduleId, "settings") == 0) {
        uint32_t code = module["securityCode"] | 0;
        unsigned long timeRemaining = module["codeTimeRemaining"] | 0;
        unsigned long lastUpdate = module["lastUpdate"] | 0;
        String ip = WiFi.localIP().toString();

        Serial.print("Showing settings module - Code: ");
        Serial.print(code);
        Serial.print(", Time remaining: ");
        Serial.print(timeRemaining / 1000);
        Serial.print("s, Last update: ");
        Serial.print(lastUpdate);
        Serial.print(", IP: ");
        Serial.println(ip);

        // If code is 0 or expired, trigger a fetch
        if (code == 0 || timeRemaining == 0) {
            Serial.println("WARNING: Security code not generated or expired! Code should be generated by scheduler.");
        }

        showSettings(code, ip.c_str(), timeRemaining);
    }
    else {
        showError("Unknown module");
    }
}

void DisplayManager::showButtonStatus(bool isPressed, int digitalValue, int analogValue) {
    u8g2.clearBuffer();

    // Title
    u8g2.setFont(u8g2_font_helvB08_tr);
    u8g2.drawStr(20, 10, "BUTTON DEBUG");

    // Button status indicator
    u8g2.setFont(u8g2_font_logisoso24_tn);
    if (isPressed) {
        u8g2.drawStr(40, 35, "ON");
    } else {
        u8g2.drawStr(30, 35, "OFF");
    }

    // Digital value
    u8g2.setFont(u8g2_font_6x10_tr);
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "Digital: %d (%s)", digitalValue, digitalValue ? "HIGH" : "LOW");
    u8g2.drawStr(2, 48, buffer);

    // Analog value
    snprintf(buffer, sizeof(buffer), "Analog: %d", analogValue);
    u8g2.drawStr(2, 60, buffer);

    u8g2.sendBuffer();
}

void DisplayManager::drawQRCode(const char* data, int x, int y, int scale) {
    // Create QR code
    QRCode qrcode;
    uint8_t qrcodeData[qrcode_getBufferSize(3)];  // Version 3
    qrcode_initText(&qrcode, qrcodeData, 3, ECC_LOW, data);

    // Draw each module as a filled box
    for (uint8_t qy = 0; qy < qrcode.size; qy++) {
        for (uint8_t qx = 0; qx < qrcode.size; qx++) {
            if (qrcode_getModule(&qrcode, qx, qy)) {
                u8g2.drawBox(x + (qx * scale), y + (qy * scale), scale, scale);
            }
        }
    }
}

void DisplayManager::showWiFiQR(const char* ssid, const char* password) {
    u8g2.clearBuffer();

    // Create WiFi QR code data
    String qrData = "WIFI:T:WPA;S:" + String(ssid) + ";P:" + String(password) + ";;";

    // Draw QR code on RIGHT side
    // QR Version 3 = 29x29 modules, scale 2 = 58x58 pixels
    // Position: x=70 (right side), y=3 (vertically centered)
    drawQRCode(qrData.c_str(), 70, 3, 2);

    // Draw info on LEFT side with plenty of space
    u8g2.setFont(u8g2_font_helvB08_tr);
    u8g2.drawStr(2, 10, "Step 1/3");

    u8g2.setFont(u8g2_font_6x10_tr);
    u8g2.drawStr(2, 24, "Scan QR");
    u8g2.drawStr(2, 34, "to join");

    u8g2.setFont(u8g2_font_5x7_tr);
    u8g2.drawStr(2, 48, String(ssid).c_str());
    u8g2.drawStr(2, 58, String(password).c_str());

    u8g2.sendBuffer();
}

void DisplayManager::showURLQR() {
    u8g2.clearBuffer();

    // Create URL QR code - shorter URL
    String qrData = "http://dt.local";

    // Draw QR code on RIGHT side - same size as Step 1
    // QR Version 3 = 29x29 modules, scale 2 = 58x58 pixels (same as WiFi QR)
    // Position: x=70 (right side, same as WiFi QR), y=3 (vertically centered)
    QRCode qrcode;
    uint8_t qrcodeData[qrcode_getBufferSize(3)];  // Version 3 (same as WiFi QR)
    qrcode_initText(&qrcode, qrcodeData, 3, ECC_LOW, qrData.c_str());

    // Draw QR on right side - same position and scale as WiFi QR
    int qrX = 70;  // Right side (same as Step 1)
    int qrY = 3;   // Vertically centered (same as Step 1)
    int scale = 2; // Same scale as Step 1
    for (uint8_t qy = 0; qy < qrcode.size; qy++) {
        for (uint8_t qx = 0; qx < qrcode.size; qx++) {
            if (qrcode_getModule(&qrcode, qx, qy)) {
                u8g2.drawBox(qrX + (qx * scale), qrY + (qy * scale), scale, scale);
            }
        }
    }

    // Draw info on LEFT side with plenty of space
    u8g2.setFont(u8g2_font_helvB08_tr);
    u8g2.drawStr(2, 10, "Step 2/3");

    u8g2.setFont(u8g2_font_6x10_tr);
    u8g2.drawStr(2, 24, "Scan QR");
    u8g2.drawStr(2, 34, "to open");
    u8g2.drawStr(2, 44, "setup");

    u8g2.setFont(u8g2_font_6x10_tr);
    u8g2.drawStr(2, 58, "dt.local");

    u8g2.sendBuffer();
}

void DisplayManager::showSettings(uint32_t securityCode, const char* deviceIP, unsigned long timeRemaining) {
    u8g2.clearBuffer();

    // Create URL QR code with device IP
    String qrData = "http://" + String(deviceIP);

    // Draw QR code on RIGHT side
    // QR Version 3 = 29x29 modules, scale 2 = 58x58 pixels
    // Position: x=70 (right side), y=3 (vertically centered)
    drawQRCode(qrData.c_str(), 70, 3, 2);

    // Draw settings info on LEFT side
    u8g2.setFont(u8g2_font_helvB08_tr);
    u8g2.drawStr(2, 10, "SETTINGS");

    u8g2.setFont(u8g2_font_6x10_tr);
    u8g2.drawStr(2, 22, "Scan QR");
    u8g2.drawStr(2, 32, "to open");

    // Show security code prominently
    u8g2.setFont(u8g2_font_helvB08_tr);
    u8g2.drawStr(2, 44, "Code:");
    u8g2.setFont(u8g2_font_helvB10_tr);
    char codeStr[8];
    snprintf(codeStr, sizeof(codeStr), "%06u", securityCode);
    u8g2.drawStr(2, 56, codeStr);

    // Note: Removed time countdown to prevent display flickering
    // QR codes need to be completely static for scanning

    u8g2.sendBuffer();
}

void DisplayManager::showModuleLoading(const char* moduleName, int progress) {
    u8g2.clearBuffer();

    // Module name at top
    drawHeader(moduleName);

    // "Loading..." text
    u8g2.setFont(u8g2_font_helvB08_tr);
    int textWidth = u8g2.getStrWidth("Loading...");
    u8g2.drawStr((128 - textWidth) / 2, 30, "Loading...");

    // Progress bar
    int barWidth = 100;
    int barHeight = 12;
    int barX = (128 - barWidth) / 2;
    int barY = 38;

    // Draw progress bar outline
    u8g2.drawFrame(barX, barY, barWidth, barHeight);

    // Fill progress bar
    int fillWidth = (barWidth - 4) * progress / 100;
    if (fillWidth > 0) {
        u8g2.drawBox(barX + 2, barY + 2, fillWidth, barHeight - 4);
    }

    // Progress percentage
    u8g2.setFont(u8g2_font_6x10_tr);
    char percentStr[8];
    snprintf(percentStr, sizeof(percentStr), "%d%%", progress);
    int percentWidth = u8g2.getStrWidth(percentStr);
    u8g2.drawStr((128 - percentWidth) / 2, 60, percentStr);

    u8g2.sendBuffer();
}

void DisplayManager::setBrightness(uint8_t level) {
    currentBrightness = level;
    u8g2.setContrast(level);
    Serial.print("Display brightness set to: ");
    Serial.println(level);
}

void DisplayManager::cycleBrightness() {
    // Define 5 brightness levels from low to high
    const uint8_t levels[] = {51, 102, 153, 204, 255};  // 20%, 40%, 60%, 80%, 100%
    const int levelCount = 5;

    // Find current level index
    int currentIndex = -1;
    for (int i = 0; i < levelCount; i++) {
        if (currentBrightness == levels[i]) {
            currentIndex = i;
            break;
        }
    }

    // If not at a predefined level, find closest
    if (currentIndex == -1) {
        int minDiff = 9999;
        for (int i = 0; i < levelCount; i++) {
            int diff = abs(currentBrightness - levels[i]);
            if (diff < minDiff) {
                minDiff = diff;
                currentIndex = i;
            }
        }
    }

    // Cycle to next level (wrap around)
    currentIndex = (currentIndex + 1) % levelCount;
    currentBrightness = levels[currentIndex];

    setBrightness(currentBrightness);

    // Show brightness level on screen briefly
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_helvB08_tr);
    u8g2.drawStr(30, 20, "BRIGHTNESS");

    // Draw brightness bar
    int barWidth = 100;
    int barHeight = 20;
    int barX = 14;
    int barY = 30;

    u8g2.drawFrame(barX, barY, barWidth, barHeight);
    int fillWidth = (barWidth - 4) * currentBrightness / 255;
    u8g2.drawBox(barX + 2, barY + 2, fillWidth, barHeight - 4);

    // Show percentage
    u8g2.setFont(u8g2_font_6x10_tr);
    char percentStr[8];
    snprintf(percentStr, sizeof(percentStr), "%d%%", (currentBrightness * 100) / 255);
    int percentWidth = u8g2.getStrWidth(percentStr);
    u8g2.drawStr((128 - percentWidth) / 2, 62, percentStr);

    u8g2.sendBuffer();
}

uint8_t DisplayManager::getBrightness() {
    return currentBrightness;
}
