#include "network.h"
#include "config.h"
#include <ESPmDNS.h>

// Animal names for unique device identification (64 names)
const char* ANIMAL_NAMES[] = {
    "Panda", "Tiger", "Lion", "Bear",
    "Fox", "Wolf", "Cat", "Dog",
    "Bunny", "Otter", "Seal", "Koala",
    "Sloth", "Lemur", "Meerkat", "Badger",
    "Eagle", "Owl", "Hawk", "Falcon",
    "Parrot", "Penguin", "Toucan", "Peacock",
    "Dolphin", "Whale", "Shark", "Turtle",
    "Octopus", "Crab", "Lobster", "Starfish",
    "Giraffe", "Zebra", "Rhino", "Hippo",
    "Elephant", "Monkey", "Gorilla", "Chimp",
    "Hamster", "Squirrel", "Chipmunk", "Hedgehog",
    "Mouse", "Rabbit", "Ferret", "Gecko",
    "Dragon", "Phoenix", "Unicorn", "Griffin",
    "Sphinx", "Kraken", "Hydra", "Basilisk",
    "Cheetah", "Leopard", "Jaguar", "Cougar",
    "Lynx", "Panther", "Puma", "Ocelot"
};

// Minimal HTML for configuration (ultra-compact)
const char PORTAL_HTML[] PROGMEM = R"rawliteral(<!DOCTYPE html>
<html><head><meta name="viewport" content="width=device-width,initial-scale=1">
<title>Setup</title><style>body{font-family:Arial;margin:20px}input,select{width:100%;padding:8px;margin:5px 0}
button{background:#4CAF50;color:#fff;padding:12px;border:none;width:100%;margin-top:15px}</style></head>
<body><h2>DataTracker Setup</h2><label>WiFi:</label><select id="ssid"></select>
<label>Password:</label><input type="password" id="pwd"><label>Module:</label>
<select id="mod"><option value="bitcoin">Bitcoin</option><option value="ethereum">Ethereum</option>
<option value="stock">Stock</option><option value="weather">Weather</option></select>
<div id="cfg"></div><button onclick="save()">Complete Step 3/3</button><script>
function save(){var c={ssid:document.getElementById('ssid').value,password:document.getElementById('pwd').value,
module:document.getElementById('mod').value};fetch('/save',{method:'POST',body:JSON.stringify(c)}).then(()=>alert('Saved!'));}
fetch('/scan').then(r=>r.json()).then(n=>{var s=document.getElementById('ssid');n.forEach(x=>s.innerHTML+=
'<option value="'+x.ssid+'">'+x.ssid+'</option>')});
</script></body></html>)rawliteral";

NetworkManager::NetworkManager()
    : server(nullptr), isAPMode(false), lastReconnectAttempt(0),
      cachedScanResults("[]"), lastScanTime(0), scanInProgress(false),
      clientWasConnected(false) {
}

NetworkManager::~NetworkManager() {
    if (server) delete server;
}

String NetworkManager::generateAnimalName() {
    uint8_t mac[6];
    WiFi.macAddress(mac);
    // Use last byte to select from 64 animals
    uint8_t index = mac[5] % 64;
    return String(ANIMAL_NAMES[index]);
}

String NetworkManager::generateSSID() {
    uint8_t mac[6];
    WiFi.macAddress(mac);

    // Get animal name
    animalName = generateAnimalName();

    // Get 2-char suffix from MAC for uniqueness
    char suffix[3];
    snprintf(suffix, sizeof(suffix), "%02X", mac[4]);

    // Build SSID: DT-PandaA3 (shorter to fit password on screen)
    apName = "DT-" + animalName + String(suffix);
    return apName;
}

String NetworkManager::generatePassword() {
    uint8_t mac[6];
    WiFi.macAddress(mac);

    // Format: DT + last 3 MAC bytes + !
    // Example: DTA3F2E1!
    char macStr[7];
    snprintf(macStr, sizeof(macStr), "%02X%02X%02X",
             mac[3], mac[4], mac[5]);

    apPassword = "DT" + String(macStr) + "!";
    return apPassword;
}

bool NetworkManager::hasClientConnected() {
    return WiFi.softAPgetStationNum() > 0;
}

bool NetworkManager::connectWiFi(const char* ssid, const char* password, uint16_t timeout) {
    Serial.print("Connecting to WiFi: ");
    Serial.println(ssid);

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);

    unsigned long startTime = millis();
    while (WiFi.status() != WL_CONNECTED && (millis() - startTime) < timeout) {
        delay(500);
        Serial.print(".");
    }
    Serial.println();

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("WiFi connected!");
        Serial.print("IP: ");
        Serial.println(WiFi.localIP());
        isAPMode = false;
        return true;
    } else {
        Serial.println("WiFi connection failed");
        return false;
    }
}

void NetworkManager::startConfigAP() {
    // Generate unique SSID and password
    generateSSID();
    generatePassword();

    Serial.println("\n╔════════════════════════════════╗");
    Serial.println("║   CONFIG MODE STARTED          ║");
    Serial.println("╚════════════════════════════════╝");
    Serial.println();
    Serial.print("📡 Network: ");
    Serial.println(apName);
    Serial.print("🔐 Password: ");
    Serial.println(apPassword);
    Serial.print("🦁 Animal: ");
    Serial.println(animalName);
    Serial.println();

    // Start AP with password
    WiFi.mode(WIFI_AP);
    WiFi.softAP(apName.c_str(), apPassword.c_str());

    IPAddress IP = WiFi.softAPIP();
    Serial.print("🌐 AP IP: ");
    Serial.println(IP);

    delay(1000);

    // Switch to AP+STA for scanning
    WiFi.mode(WIFI_AP_STA);

    // Start mDNS responder
    if (!MDNS.begin("dt")) {
        Serial.println("⚠️  mDNS failed to start");
    } else {
        Serial.println("✓ mDNS started: dt.local");
        MDNS.addService("http", "tcp", 80);
    }

    // Setup web server
    setupWebServer();

    cachedScanResults = "[]";
    scanInProgress = false;
    lastScanTime = millis();
    clientWasConnected = false;

    Serial.println();
    Serial.println("📱 Scan QR code on display");
    Serial.println("   or connect manually");
    Serial.println();
    Serial.println("🌐 Once connected, open:");
    Serial.println("   http://dt.local");
    Serial.println("   http://192.168.4.1");
    Serial.println();

    isAPMode = true;
}

void NetworkManager::setupWebServer() {
    server = new WebServer(80);

    // Root page
    server->on("/", [this]() {
        server->send_P(200, "text/html", PORTAL_HTML);
    });

    // Scan endpoint
    server->on("/scan", [this]() {
        handleScan();
    });

    // Save endpoint
    server->on("/save", HTTP_POST, [this]() {
        handleSave();
    });

    server->begin();
    Serial.println("Web server started");
}

void NetworkManager::handleScan() {
    server->sendHeader("Access-Control-Allow-Origin", "*");
    server->send(200, "application/json", cachedScanResults);
}

void NetworkManager::handleSave() {
    if (server->hasArg("plain")) {
        String body = server->arg("plain");

        StaticJsonDocument<512> doc;
        DeserializationError error = deserializeJson(doc, body);

        if (!error) {
            config["wifi"]["ssid"] = doc["ssid"].as<String>();
            config["wifi"]["password"] = doc["password"].as<String>();
            config["device"]["activeModule"] = doc["module"].as<String>();

            saveConfiguration();

            server->send(200, "text/plain", "OK");

            delay(1000);
            ESP.restart();
        } else {
            server->send(400, "text/plain", "Invalid JSON");
        }
    } else {
        server->send(400, "text/plain", "No data");
    }
}

void NetworkManager::stopConfigAP() {
    if (server) {
        delete server;
        server = nullptr;
    }
    WiFi.softAPdisconnect(true);
    isAPMode = false;
}

bool NetworkManager::isConnected() {
    return WiFi.status() == WL_CONNECTED;
}

void NetworkManager::reconnect() {
    if (isAPMode) return;

    unsigned long now = millis();
    if (now - lastReconnectAttempt < 30000) return;

    lastReconnectAttempt = now;

    String ssid = config["wifi"]["ssid"] | "";
    String password = config["wifi"]["password"] | "";

    if (ssid.length() == 0) return;

    Serial.println("Reconnecting to WiFi...");
    WiFi.disconnect();
    WiFi.begin(ssid.c_str(), password.c_str());
}

void NetworkManager::handleClient() {
    if (server) {
        server->handleClient();
    }

    if (isAPMode) {
        updateScanResults();
    }
}

bool NetworkManager::httpGet(const char* url, String& response, String& errorMsg) {
    WiFiClientSecure* client = new WiFiClientSecure;
    if (!client) {
        errorMsg = "Out of memory";
        return false;
    }

    client->setInsecure();

    HTTPClient https;
    https.begin(*client, url);
    https.setTimeout(15000);

    int httpCode = https.GET();

    if (httpCode == HTTP_CODE_OK) {
        response = https.getString();
        https.end();
        delete client;
        return true;
    } else {
        errorMsg = "HTTP " + String(httpCode);
        https.end();
        delete client;
        return false;
    }
}

void NetworkManager::startWiFiScan() {
    Serial.println("Starting WiFi scan...");

    if (WiFi.getMode() != WIFI_AP_STA) {
        WiFi.mode(WIFI_AP_STA);
        delay(500);
    }

    WiFi.setTxPower(WIFI_POWER_19_5dBm);

    // Increased to 500ms per channel for more reliable scanning
    int result = WiFi.scanNetworks(true, false, false, 500);

    if (result == WIFI_SCAN_RUNNING) {
        scanInProgress = true;
        lastScanTime = millis();
        Serial.println("Scan started");
    } else {
        Serial.println("Scan failed to start");
    }
}

void NetworkManager::updateScanResults() {
    if (!scanInProgress) {
        unsigned long timeSinceLastScan = millis() - lastScanTime;
        // Wait 10 seconds after boot for WiFi to stabilize, then rescan every 30 seconds
        if ((lastScanTime < 10000 && millis() > 10000) || timeSinceLastScan > 30000) {
            startWiFiScan();
        }
        return;
    }

    int n = WiFi.scanComplete();

    if (n == WIFI_SCAN_RUNNING) {
        unsigned long scanDuration = millis() - lastScanTime;
        if (scanDuration > 15000) {
            Serial.println("Scan timeout");
            WiFi.scanDelete();
            cachedScanResults = "[]";
            scanInProgress = false;
        }
        return;
    }

    if (n == WIFI_SCAN_FAILED) {
        Serial.println("Scan failed");
        cachedScanResults = "[]";
        scanInProgress = false;
        WiFi.scanDelete();
        return;
    }

    if (n >= 0) {
        Serial.print("Found ");
        Serial.print(n);
        Serial.println(" networks");

        String json = "[";
        for (int i = 0; i < n && i < 20; i++) {
            if (i > 0) json += ",";
            String ssid = WiFi.SSID(i);
            ssid.replace("\"", "\\\"");
            json += "{\"ssid\":\"" + ssid + "\",";
            json += "\"rssi\":" + String(WiFi.RSSI(i)) + "}";
        }
        json += "]";

        cachedScanResults = json;
        scanInProgress = false;
        lastScanTime = millis();

        Serial.println("Scan complete");
        WiFi.scanDelete();
    }
}
