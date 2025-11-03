#ifndef NETWORK_H
#define NETWORK_H

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <WebServer.h>

class NetworkManager {
private:
    WebServer* server;
    String apName;
    String apPassword;
    String animalName;
    bool isAPMode;
    unsigned long lastReconnectAttempt;

    // WiFi scan caching
    String cachedScanResults;
    unsigned long lastScanTime;
    bool scanInProgress;

    // Client connection tracking
    bool clientWasConnected;

    // Web server handlers
    void setupWebServer();
    void handleRoot();
    void handleScan();
    void handleSave();

    // WiFi scanning
    void startWiFiScan();
    void updateScanResults();

    // Animal name and password generation
    String generateAnimalName();
    String generateSSID();
    String generatePassword();

public:
    NetworkManager();
    ~NetworkManager();

    // WiFi management
    bool connectWiFi(const char* ssid, const char* password, uint16_t timeout = 15000);
    void startConfigAP();
    void stopConfigAP();
    bool isConnected();
    void reconnect();

    // HTTP requests
    bool httpGet(const char* url, String& response, String& errorMsg);

    // Accessors
    String getAPName() { return apName; }
    String getAPPassword() { return apPassword; }
    String getAnimalName() { return animalName; }
    bool isInAPMode() { return isAPMode; }
    bool hasClientConnected();

    // Server handling
    void handleClient();
};

#endif // NETWORK_H
