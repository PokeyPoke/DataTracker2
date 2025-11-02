# Adaptive QR Code Setup - Technical Design

## Overview
Device shows WiFi QR first, then automatically switches to URL QR when client connects.

## User Flow
1. Display: WiFi QR + "Scan to Connect"
2. User scans → Phone connects to AP
3. Display detects connection → Shows URL QR + "Scan to Setup"
4. User scans → Browser opens to config page
5. Configure → Done!

## Display States

### State: NO_CLIENT_CONNECTED
```cpp
void showWiFiQR() {
    u8g2.clearBuffer();

    // Generate WiFi QR code
    String ssid = "DataTracker-" + macStr;
    String password = "DT" + macStr + "!";
    String qrData = "WIFI:T:WPA;S:" + ssid + ";P:" + password + ";;";

    // Draw large QR code (centered, 80x80 pixels)
    drawQRCode(qrData, 24, 0, 3);  // x=24, y=0, scale=3

    // Instructions at bottom
    u8g2.setFont(u8g2_font_6x10_tr);
    u8g2.drawStr(10, 60, "Scan to Connect");

    u8g2.sendBuffer();
}
```

### State: CLIENT_CONNECTED
```cpp
void showURLQR() {
    u8g2.clearBuffer();

    // Success indicator
    u8g2.setFont(u8g2_font_helvB08_tr);
    u8g2.drawStr(40, 10, "Connected!");

    // Generate URL QR code
    String qrData = "http://datatracker.local";

    // Draw large QR code (centered, 80x80 pixels)
    drawQRCode(qrData, 24, 12, 3);  // x=24, y=12, scale=3

    // Instructions at bottom
    u8g2.setFont(u8g2_font_6x10_tr);
    u8g2.drawStr(15, 60, "Scan to Setup");

    u8g2.sendBuffer();
}
```

## Client Detection

### Method 1: WiFi.softAPgetStationNum() (Recommended)
```cpp
bool hasConnectedClient() {
    return WiFi.softAPgetStationNum() > 0;
}

void loop() {
    if (configMode) {
        // Check for client connection
        bool clientConnected = hasConnectedClient();

        if (clientConnected && !wasClientConnected) {
            // Client just connected!
            Serial.println("Client connected to AP");
            showURLQR();
            wasClientConnected = true;
        }
        else if (!clientConnected && wasClientConnected) {
            // Client disconnected
            Serial.println("Client disconnected");
            showWiFiQR();
            wasClientConnected = false;
        }

        network.handleClient();
    }
}
```

### Method 2: DHCP Event Handler (Alternative)
```cpp
void WiFiEvent(WiFiEvent_t event) {
    switch(event) {
        case SYSTEM_EVENT_AP_STACONNECTED:
            Serial.println("Client connected!");
            showURLQR();
            break;

        case SYSTEM_EVENT_AP_STADISCONNECTED:
            Serial.println("Client disconnected");
            showWiFiQR();
            break;
    }
}

void setup() {
    WiFi.onEvent(WiFiEvent);
}
```

## QR Code Library

### Option 1: QRCode library (Lightweight)
```cpp
// platformio.ini
lib_deps =
    ricmoo/QRCode@^0.0.1

// Code
#include <qrcode.h>

void drawQRCode(String data, int x, int y, int scale) {
    QRCode qrcode;
    uint8_t qrcodeData[qrcode_getBufferSize(3)];  // Version 3
    qrcode_initText(&qrcode, qrcodeData, 3, ECC_LOW, data.c_str());

    for (uint8_t qy = 0; qy < qrcode.size; qy++) {
        for (uint8_t qx = 0; qx < qrcode.size; qx++) {
            if (qrcode_getModule(&qrcode, qx, qy)) {
                // Draw filled rectangle for each module
                u8g2.drawBox(x + (qx * scale), y + (qy * scale), scale, scale);
            }
        }
    }
}
```

**Memory cost:** ~2-3KB

### Option 2: qrcodegen library (More features)
```cpp
// platformio.ini
lib_deps =
    nayuki/QR-Code-generator@^1.8.0

// Similar implementation but more options
```

**Memory cost:** ~4-5KB

## mDNS Setup

```cpp
#include <ESPmDNS.h>

void startConfigAP() {
    // ... existing AP setup ...

    // Start mDNS responder
    if (!MDNS.begin("datatracker")) {
        Serial.println("Error starting mDNS");
    } else {
        Serial.println("mDNS responder started: datatracker.local");
        MDNS.addService("http", "tcp", 80);
    }
}
```

## WiFi Password Generation

```cpp
String generateSecurePassword() {
    // Use last 6 chars of MAC address
    uint8_t mac[6];
    WiFi.macAddress(mac);
    char macStr[7];
    snprintf(macStr, sizeof(macStr), "%02X%02X%02X", mac[3], mac[4], mac[5]);

    // Format: DT + MAC + !
    // Example: DTA3F2E1!
    return "DT" + String(macStr) + "!";
}
```

**Benefits:**
- Unique per device
- Easy to type if QR fails
- Reasonably secure (8 chars, mixed case, symbol)
- Memorable pattern

## Display Size Calculations

### 128x64 Display Layout
```
Available: 128x64 pixels
QR Code: 80x80 pixels (too big!)
QR Code: 70x70 pixels (fits with margin)

QR Version 3: 29x29 modules
Scale 2: 58x58 pixels ✓
Scale 3: 87x87 pixels (too big for short URLs)

QR Version 2: 25x25 modules
Scale 2: 50x50 pixels ✓
Scale 3: 75x75 pixels ✓ (best for readability)
```

**Recommendation:** Version 2, Scale 3 = 75x75 pixels

### Layout Math
```
Display height: 64 pixels
QR size: 75 pixels
Problem: Doesn't fit!

Solution: Use Version 2, Scale 2 = 50x50 pixels
Or: Compress long URL
```

### URL Shortening
```cpp
// Instead of: http://datatracker.local
// Use: http://dt.local

// Even shorter for IP:
// http://192.168.4.1
```

## Complete State Machine

```cpp
enum SetupState {
    WAITING_FOR_CLIENT,   // Show WiFi QR
    CLIENT_CONNECTED,     // Show URL QR
    CONFIGURING,          // User on config page
    COMPLETE              // Exit config mode
};

SetupState currentState = WAITING_FOR_CLIENT;

void loop() {
    if (!configMode) return;

    switch (currentState) {
        case WAITING_FOR_CLIENT:
            if (hasConnectedClient()) {
                currentState = CLIENT_CONNECTED;
                showURLQR();
            }
            break;

        case CLIENT_CONNECTED:
            if (!hasConnectedClient()) {
                // Client disconnected
                currentState = WAITING_FOR_CLIENT;
                showWiFiQR();
            }
            // Fall through to handle web requests
            network.handleClient();
            break;

        case CONFIGURING:
            // User is on config page
            network.handleClient();
            break;

        case COMPLETE:
            // Config saved, exit
            configMode = false;
            break;
    }
}
```

## Error Handling

### QR Code Won't Scan
```cpp
void showWiFiQR() {
    // ... QR code ...

    // Fallback text
    u8g2.setFont(u8g2_font_5x7_tr);
    u8g2.drawStr(0, 62, apName.c_str());  // Show network name
}
```

### mDNS Doesn't Work
```cpp
void showURLQR() {
    // ... QR code ...

    // Show both options
    u8g2.setFont(u8g2_font_5x7_tr);
    u8g2.drawStr(0, 57, "dt.local or");
    u8g2.drawStr(0, 64, "192.168.4.1");
}
```

### Client Connects But Doesn't Scan
```cpp
// Add timeout
if (currentState == CLIENT_CONNECTED) {
    if (millis() - stateChangeTime > 30000) {
        // 30 seconds, no activity
        display.clear();
        u8g2.setFont(u8g2_font_6x10_tr);
        u8g2.drawStr(10, 30, "Open browser to:");
        u8g2.drawStr(10, 45, "192.168.4.1");
        u8g2.sendBuffer();
    }
}
```

## Memory Budget

```
Current v2.0: 42,964 bytes (13.1%)
QR library:    ~3,000 bytes
mDNS:          ~2,000 bytes
State logic:   ~1,000 bytes
───────────────────────────────
Total:        48,964 bytes (14.9%)
```

Still well under 20% - plenty of room!

## Testing Checklist

- [ ] WiFi QR code scans correctly on iOS
- [ ] WiFi QR code scans correctly on Android
- [ ] Device detects client connection
- [ ] Display switches to URL QR automatically
- [ ] URL QR code scans correctly
- [ ] mDNS resolves datatracker.local
- [ ] Fallback to 192.168.4.1 works
- [ ] Client disconnect returns to WiFi QR
- [ ] Multiple connect/disconnect cycles work
- [ ] Password is unique per device
- [ ] Display is readable from 1 foot away
- [ ] Total setup time < 1 minute

## Implementation Steps

1. Add QR code library to platformio.ini
2. Create QR drawing function
3. Add client detection in loop()
4. Create two display states (WiFi QR, URL QR)
5. Add mDNS responder
6. Test on real hardware
7. Polish UX (timing, transitions)

## Expected Improvements

| Metric | Current | With QR |
|--------|---------|---------|
| Setup steps | 6 manual | 2 scans |
| User typing | IP + password | Just WiFi password |
| Time to setup | 2-3 min | 30-45 sec |
| User errors | High (typos) | Low (automated) |
| "Wow" factor | Low | High |
