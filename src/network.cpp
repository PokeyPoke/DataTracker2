#include "network.h"
#include "config.h"
#include "security.h"
#include "scheduler.h"
#include <ESPmDNS.h>
#include <LittleFS.h>

// External objects (initialized in main)
extern SecurityManager security;
extern Scheduler scheduler;

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

// Settings page HTML (login + configuration)
const char SETTINGS_HTML[] PROGMEM = R"rawliteral(<!DOCTYPE html>
<html><head><meta name="viewport" content="width=device-width,initial-scale=1"><title>Settings</title>
<style>body{font-family:Arial;max-width:500px;margin:40px auto;padding:20px}
.hidden{display:none}input,select{width:100%;padding:10px;margin:8px 0;box-sizing:border-box}
button{background:#4CAF50;color:#fff;padding:12px;border:none;width:100%;margin-top:10px;cursor:pointer}
button:hover{background:#45a049}.danger{background:#f44336}.danger:hover{background:#da190b}
.code-input{font-size:24px;text-align:center;letter-spacing:10px}
.error{color:#f44336;margin:10px 0}.success{color:#4CAF50;margin:10px 0}
label{display:block;margin-top:15px;font-weight:bold}h2{text-align:center}
.timer{text-align:center;color:#666;font-size:14px}
.search-container{position:relative}.search-results{border:1px solid #ddd;max-height:200px;overflow-y:auto;margin-top:5px;display:none}
.search-item{padding:10px;cursor:pointer;border-bottom:1px solid #eee;display:flex;align-items:center}
.search-item:hover{background:#f5f5f5}.search-item img{width:24px;height:24px;margin-right:10px}
.current-value{font-size:14px;color:#666;margin-top:5px}</style></head><body>
<div id="login-view"><h2>DataTracker Settings</h2><p>Enter code from device display:</p>
<input type="text" id="code" class="code-input" maxlength="6" placeholder="000000" pattern="[0-9]{6}">
<button onclick="validateCode()">Unlock Settings</button><div id="error" class="error"></div>
<p class="timer" id="timer"></p></div>
<div id="settings-view" class="hidden"><h2>Settings</h2>
<div style="text-align:right;margin-bottom:10px"><button onclick="logout()" style="width:auto;padding:8px 16px;font-size:12px">Logout</button></div>
<label>Active Module:</label><select id="activeModule">
<option value="bitcoin">Crypto 1</option><option value="ethereum">Crypto 2</option>
<option value="stock">Stock</option><option value="weather">Weather</option>
<option value="custom">Custom</option><option value="settings">Settings</option></select>
<label>Crypto 1 (Bitcoin Module):</label>
<div class="search-container">
<input type="text" id="btcSearch" placeholder="Search cryptocurrency..." oninput="searchCrypto('bitcoin',this.value)">
<div id="btcResults" class="search-results"></div>
<div id="btcCurrent" class="current-value"></div>
</div>
<label>Crypto 2 (Ethereum Module):</label>
<div class="search-container">
<input type="text" id="ethSearch" placeholder="Search cryptocurrency..." oninput="searchCrypto('ethereum',this.value)">
<div id="ethResults" class="search-results"></div>
<div id="ethCurrent" class="current-value"></div>
</div>
<label>Stock Ticker:</label><input type="text" id="stockTicker" placeholder="AAPL" maxlength="10">
<label>Weather Location:</label><input type="text" id="weatherLoc" placeholder="lat,lon (e.g. 37.7749,-122.4194)">
<label>Custom Label:</label><input type="text" id="customLabel" placeholder="Label" maxlength="20">
<label>Custom Value:</label><input type="number" id="customValue" step="0.01">
<label>Custom Unit:</label><input type="text" id="customUnit" placeholder="Unit" maxlength="10">
<div id="msg" style="margin:15px 0;padding:12px;border-radius:4px;text-align:center;font-weight:bold;display:none"></div>
<button onclick="saveSettings()">Save Changes</button>
<button onclick="restartDevice()" class="danger">Restart Device</button>
<button onclick="factoryReset()" class="danger">Factory Reset</button>
</div>
<script>var token='';var searchTimeouts={};
function validateCode(){var c=document.getElementById('code').value;
if(c.length!=6){showError('Enter 6-digit code');return;}
fetch('/api/validate',{method:'POST',body:JSON.stringify({code:parseInt(c)})})
.then(r=>r.json()).then(d=>{if(d.valid){token=d.token;
showSettings();}else{showError(d.error||'Invalid code');}}).catch(()=>showError('Connection error'));}
function showSettings(){document.getElementById('login-view').className='hidden';
document.getElementById('settings-view').className='';loadConfig();}
function logout(){localStorage.removeItem('token');token='';
document.getElementById('settings-view').className='hidden';
document.getElementById('login-view').className='';
document.getElementById('code').value='';
document.getElementById('error').innerText='';}
function handleUnauthorized(){logout();showError('Session expired. Please enter the code from your device.');}
function loadConfig(){fetch('/api/config',{headers:{'Authorization':token}})
.then(r=>{if(r.status===401){handleUnauthorized();return null;}if(!r.ok){throw new Error('Failed to load config');}return r.json();})
.then(d=>{if(!d)return;document.getElementById('activeModule').value=d.device.activeModule||'bitcoin';
document.getElementById('stockTicker').value=d.modules.stock.ticker||'';
document.getElementById('weatherLoc').value=d.modules.weather.location||'';
document.getElementById('customLabel').value=d.modules.custom.label||'';
document.getElementById('customValue').value=d.modules.custom.value||0;
document.getElementById('customUnit').value=d.modules.custom.unit||'';
window.bitcoin_config={cryptoId:d.modules.bitcoin.cryptoId||'bitcoin',cryptoSymbol:d.modules.bitcoin.cryptoSymbol||'BTC',cryptoName:d.modules.bitcoin.cryptoName||'Bitcoin'};
window.ethereum_config={cryptoId:d.modules.ethereum.cryptoId||'ethereum',cryptoSymbol:d.modules.ethereum.cryptoSymbol||'ETH',cryptoName:d.modules.ethereum.cryptoName||'Ethereum'};
updateCryptoDisplay('bitcoin',d.modules.bitcoin);updateCryptoDisplay('ethereum',d.modules.ethereum);})
.catch(e=>{console.error('Load config error:',e);showMsg('Failed to load settings. Please try again.','error');});}
function updateCryptoDisplay(module,data){var prefix=module==='bitcoin'?'btc':'eth';
var cur=document.getElementById(prefix+'Current');
if(data.cryptoName){cur.innerHTML='Current: '+data.cryptoName+' ('+data.cryptoSymbol.toUpperCase()+')';}else{cur.innerHTML='Current: '+(module==='bitcoin'?'Bitcoin (BTC)':'Ethereum (ETH)');}}
function searchCrypto(module,query){clearTimeout(searchTimeouts[module]);
if(query.length<2){hideResults(module);return;}
var prefix=module==='bitcoin'?'btc':'eth';
var results=document.getElementById(prefix+'Results');
results.innerHTML='<div class="search-item">Searching...</div>';
results.style.display='block';
searchTimeouts[module]=setTimeout(()=>{
fetch('https://api.coingecko.com/api/v3/search?query='+encodeURIComponent(query))
.then(r=>r.json()).then(d=>{var html='';
d.coins.slice(0,5).forEach(coin=>{html+='<div class="search-item" onclick="selectCrypto(\''+module+'\',\''+coin.id+'\',\''+coin.symbol+'\',\''+coin.name.replace(/'/g,"\\'")+'\')">';
if(coin.thumb){html+='<img src="'+coin.thumb+'">';}
html+=coin.name+' ('+coin.symbol.toUpperCase()+')</div>';});
results.innerHTML=html||'<div class="search-item">No results</div>';}).catch(()=>{results.innerHTML='<div class="search-item">Error searching</div>';});},300);}
function hideResults(module){var prefix=module==='bitcoin'?'btc':'eth';
setTimeout(()=>{document.getElementById(prefix+'Results').style.display='none';},200);}
function selectCrypto(module,id,symbol,name){var prefix=module==='bitcoin'?'btc':'eth';
window[module+'_config']={cryptoId:id,cryptoSymbol:symbol,cryptoName:name};
document.getElementById(prefix+'Search').value='';
hideResults(module);updateCryptoDisplay(module,{cryptoName:name,cryptoSymbol:symbol});}
function saveSettings(){var cfg={device:{activeModule:document.getElementById('activeModule').value},
modules:{bitcoin:window.bitcoin_config||{},ethereum:window.ethereum_config||{},
stock:{ticker:document.getElementById('stockTicker').value},
weather:{location:document.getElementById('weatherLoc').value},
custom:{label:document.getElementById('customLabel').value,value:parseFloat(document.getElementById('customValue').value)||0,
unit:document.getElementById('customUnit').value}}};
console.log('Saving config:',cfg);
fetch('/api/config',{method:'POST',headers:{'Authorization':token},body:JSON.stringify(cfg)})
.then(r=>{if(r.status===401){handleUnauthorized();return null;}return r.json();})
.then(d=>{if(!d)return;console.log('Save response:',d);if(d.success){showMsg('Settings Saved Successfully!','success');}else{showMsg('Save failed: '+(d.error||'Unknown error'),'error');}})
.catch(e=>{console.error('Save error:',e);showMsg('Connection error','error');});}
function restartDevice(){if(confirm('Restart device?')){fetch('/api/restart',{method:'POST',headers:{'Authorization':token}})
.then(r=>{if(r.status===401){handleUnauthorized();return;}showMsg('Restarting...','success');});}}
function factoryReset(){if(confirm('Factory reset? This will erase all settings!')){
fetch('/api/factory-reset',{method:'POST',headers:{'Authorization':token}})
.then(r=>{if(r.status===401){handleUnauthorized();return;}showMsg('Resetting...','success');});}}
function showError(msg){document.getElementById('error').innerText=msg;}
function showMsg(msg,type){var el=document.getElementById('msg');el.innerText=msg;
el.style.display='block';
if(type==='success'){el.style.background='#4CAF50';el.style.color='#fff';}else{el.style.background='#f44336';el.style.color='#fff';}
setTimeout(()=>{el.style.display='none';el.innerText='';},3000);}
localStorage.removeItem('token');</script></body></html>)rawliteral";

NetworkManager::NetworkManager()
    : server(nullptr), isAPMode(false), isSettingsMode(false), lastReconnectAttempt(0),
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

// ============================================
// Settings Server (for normal operation mode)
// ============================================

void NetworkManager::startSettingsServer() {
    if (server || isAPMode) {
        Serial.println("Cannot start settings server: server already running or in AP mode");
        return;
    }

    Serial.println("\n=== Starting Settings Server ===");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());

    isSettingsMode = true;
    setupSettingsServer();

    Serial.println("Settings server started");
    Serial.println("================================\n");
}

void NetworkManager::stopSettingsServer() {
    if (server && isSettingsMode) {
        delete server;
        server = nullptr;
        isSettingsMode = false;
        Serial.println("Settings server stopped");
    }
}

bool NetworkManager::isSettingsServerRunning() {
    return isSettingsMode && (server != nullptr);
}

String NetworkManager::getLocalIP() {
    if (WiFi.status() == WL_CONNECTED) {
        return WiFi.localIP().toString();
    }
    return "0.0.0.0";
}

void NetworkManager::setupSettingsServer() {
    server = new WebServer(80);

    // Settings page root
    server->on("/", [this]() {
        handleSettingsRoot();
    });

    // API endpoints
    server->on("/api/validate", HTTP_POST, [this]() {
        handleValidateCode();
    });

    server->on("/api/config", HTTP_GET, [this]() {
        handleGetConfig();
    });

    server->on("/api/config", HTTP_POST, [this]() {
        handleUpdateConfig();
    });

    server->on("/api/restart", HTTP_POST, [this]() {
        handleRestart();
    });

    server->on("/api/factory-reset", HTTP_POST, [this]() {
        handleFactoryReset();
    });

    // Debug endpoint (no auth required) - shows crypto module config only
    server->on("/debug", [this]() {
        String html = "<!DOCTYPE html><html><head><title>Debug Config</title>";
        html += "<style>body{font-family:monospace;padding:20px;background:#1e1e1e;color:#d4d4d4}";
        html += "table{border-collapse:collapse;margin:20px 0}";
        html += "td,th{border:1px solid #666;padding:8px 12px;text-align:left}";
        html += "th{background:#2d2d2d}</style></head><body>";
        html += "<h2>Crypto Module Configuration</h2>";
        html += "<p style='color:#888'>v2.4.3 - Full Debug | Auto-refreshes every 3 seconds</p>";
        html += "<table><tr><th>Module</th><th>Field</th><th>Value</th></tr>";

        // Bitcoin module
        JsonObject bitcoin = config["modules"]["bitcoin"];
        html += "<tr><td rowspan='6'>Crypto 1 (bitcoin)</td>";
        html += "<td>cryptoId</td><td>" + String(bitcoin["cryptoId"] | "NOT SET") + "</td></tr>";
        html += "<tr><td>cryptoSymbol</td><td>" + String(bitcoin["cryptoSymbol"] | "NOT SET") + "</td></tr>";
        html += "<tr><td>cryptoName</td><td>" + String(bitcoin["cryptoName"] | "NOT SET") + "</td></tr>";
        html += "<tr><td>value</td><td>$" + String(bitcoin["value"] | 0.0, 2) + "</td></tr>";
        html += "<tr><td>lastUpdate</td><td>" + String(bitcoin["lastUpdate"] | 0) + "</td></tr>";
        html += "<tr><td>lastSuccess</td><td>" + String(bitcoin["lastSuccess"] | false ? "true" : "false") + "</td></tr>";

        // Ethereum module
        JsonObject ethereum = config["modules"]["ethereum"];
        html += "<tr><td rowspan='6'>Crypto 2 (ethereum)</td>";
        html += "<td>cryptoId</td><td>" + String(ethereum["cryptoId"] | "NOT SET") + "</td></tr>";
        html += "<tr><td>cryptoSymbol</td><td>" + String(ethereum["cryptoSymbol"] | "NOT SET") + "</td></tr>";
        html += "<tr><td>cryptoName</td><td>" + String(ethereum["cryptoName"] | "NOT SET") + "</td></tr>";
        html += "<tr><td>value</td><td>$" + String(ethereum["value"] | 0.0, 2) + "</td></tr>";
        html += "<tr><td>lastUpdate</td><td>" + String(ethereum["lastUpdate"] | 0) + "</td></tr>";
        html += "<tr><td>lastSuccess</td><td>" + String(ethereum["lastSuccess"] | false ? "true" : "false") + "</td></tr>";

        html += "</table>";

        html += "<h3>Raw JSON Dump</h3>";
        html += "<pre style='background:#2d2d2d;padding:10px;overflow:auto;max-height:400px'>";
        String configJson;
        serializeJsonPretty(config, configJson);
        configJson.replace("<", "&lt;");
        configJson.replace(">", "&gt;");
        html += configJson;
        html += "</pre>";

        html += "<p>Config memory: " + String(config.memoryUsage()) + " / " + String(config.capacity()) + " bytes</p>";
        if (config.overflowed()) {
            html += "<p style='color:#f44336;font-weight:bold'>⚠ WARNING: Config overflowed!</p>";
        }
        html += "<script>setTimeout(function(){location.reload()},3000);</script>";
        html += "</body></html>";
        server->send(200, "text/html", html);
    });

    server->begin();
}

// Settings page handlers
void NetworkManager::handleSettingsRoot() {
    server->send_P(200, "text/html", SETTINGS_HTML);
}

void NetworkManager::handleValidateCode() {
    if (!server->hasArg("plain")) {
        server->send(400, "application/json", "{\"valid\":false,\"error\":\"No data\"}");
        return;
    }

    String body = server->arg("plain");
    StaticJsonDocument<128> doc;
    DeserializationError error = deserializeJson(doc, body);

    if (error) {
        server->send(400, "application/json", "{\"valid\":false,\"error\":\"Invalid JSON\"}");
        return;
    }

    uint32_t enteredCode = doc["code"];

    // Check if locked out
    if (security.isLockedOut()) {
        unsigned long remaining = security.getLockoutTimeRemaining();
        char response[128];
        snprintf(response, sizeof(response),
                 "{\"valid\":false,\"error\":\"Locked out for %lu seconds\"}",
                 remaining / 1000);
        server->send(403, "application/json", response);
        return;
    }

    // Validate code
    if (security.validateCode(enteredCode)) {
        // Create session
        String token = security.createSession();

        char response[128];
        snprintf(response, sizeof(response),
                 "{\"valid\":true,\"token\":\"%s\"}", token.c_str());
        server->send(200, "application/json", response);
    } else {
        server->send(200, "application/json", "{\"valid\":false,\"error\":\"Invalid or expired code\"}");
    }
}

void NetworkManager::handleGetConfig() {
    // Check authorization
    String token = server->header("Authorization");
    if (!security.validateSession(token)) {
        server->send(401, "application/json", "{\"error\":\"Unauthorized\"}");
        return;
    }

    // Serialize current configuration
    String response;
    serializeJson(config, response);
    server->send(200, "application/json", response);
}

void NetworkManager::handleUpdateConfig() {
    // Check authorization
    String token = server->header("Authorization");
    if (!security.validateSession(token)) {
        server->send(401, "application/json", "{\"error\":\"Unauthorized\"}");
        return;
    }

    if (!server->hasArg("plain")) {
        server->send(400, "application/json", "{\"success\":false,\"error\":\"No data\"}");
        return;
    }

    String body = server->arg("plain");
    StaticJsonDocument<2048> doc;
    DeserializationError error = deserializeJson(doc, body);

    if (error) {
        String errorMsg = "{\"success\":false,\"error\":\"Invalid JSON: ";
        errorMsg += error.c_str();
        errorMsg += "\"}";
        server->send(400, "application/json", errorMsg);
        return;
    }

    // Update configuration
    if (doc.containsKey("device")) {
        if (doc["device"].containsKey("activeModule")) {
            config["device"]["activeModule"] = doc["device"]["activeModule"].as<String>();
        }
    }

    if (doc.containsKey("modules")) {
        JsonObject modules = doc["modules"];

        // Update bitcoin crypto config
        if (modules.containsKey("bitcoin")) {
            bool cryptoChanged = false;
            if (modules["bitcoin"].containsKey("cryptoId")) {
                String newId = modules["bitcoin"]["cryptoId"].as<String>();
                String oldId = config["modules"]["bitcoin"]["cryptoId"] | "bitcoin";
                if (newId != oldId) {
                    cryptoChanged = true;
                }
                config["modules"]["bitcoin"]["cryptoId"] = newId;
                Serial.print("Bitcoin cryptoId updated to: ");
                Serial.println(newId);
            }
            if (modules["bitcoin"].containsKey("cryptoSymbol")) {
                String symbol = modules["bitcoin"]["cryptoSymbol"].as<String>();
                config["modules"]["bitcoin"]["cryptoSymbol"] = symbol;
                Serial.print("Bitcoin cryptoSymbol updated to: ");
                Serial.println(symbol);
            }
            if (modules["bitcoin"].containsKey("cryptoName")) {
                String name = modules["bitcoin"]["cryptoName"].as<String>();
                config["modules"]["bitcoin"]["cryptoName"] = name;
                Serial.print("Bitcoin cryptoName updated to: ");
                Serial.println(name);
            }
            // Clear cached data if crypto changed
            if (cryptoChanged) {
                config["modules"]["bitcoin"]["value"] = 0.0;
                config["modules"]["bitcoin"]["change24h"] = 0.0;
                config["modules"]["bitcoin"]["lastUpdate"] = 0;
                Serial.println("Bitcoin crypto changed - cleared cache");
            }
        }

        // Update ethereum crypto config
        if (modules.containsKey("ethereum")) {
            bool cryptoChanged = false;
            if (modules["ethereum"].containsKey("cryptoId")) {
                String newId = modules["ethereum"]["cryptoId"].as<String>();
                String oldId = config["modules"]["ethereum"]["cryptoId"] | "ethereum";
                if (newId != oldId) {
                    cryptoChanged = true;
                }
                config["modules"]["ethereum"]["cryptoId"] = newId;
            }
            if (modules["ethereum"].containsKey("cryptoSymbol")) {
                config["modules"]["ethereum"]["cryptoSymbol"] = modules["ethereum"]["cryptoSymbol"].as<String>();
            }
            if (modules["ethereum"].containsKey("cryptoName")) {
                config["modules"]["ethereum"]["cryptoName"] = modules["ethereum"]["cryptoName"].as<String>();
            }
            // Clear cached data if crypto changed
            if (cryptoChanged) {
                config["modules"]["ethereum"]["value"] = 0.0;
                config["modules"]["ethereum"]["change24h"] = 0.0;
                config["modules"]["ethereum"]["lastUpdate"] = 0;
                Serial.println("Ethereum crypto changed - cleared cache");
            }
        }

        // Update stock config
        if (modules.containsKey("stock") && modules["stock"].containsKey("ticker")) {
            config["modules"]["stock"]["ticker"] = modules["stock"]["ticker"].as<String>();
        }

        // Update weather config
        if (modules.containsKey("weather") && modules["weather"].containsKey("location")) {
            config["modules"]["weather"]["location"] = modules["weather"]["location"].as<String>();
        }

        // Update custom config
        if (modules.containsKey("custom")) {
            if (modules["custom"].containsKey("label")) {
                config["modules"]["custom"]["label"] = modules["custom"]["label"].as<String>();
            }
            if (modules["custom"].containsKey("value")) {
                config["modules"]["custom"]["value"] = modules["custom"]["value"].as<float>();
            }
            if (modules["custom"].containsKey("unit")) {
                config["modules"]["custom"]["unit"] = modules["custom"]["unit"].as<String>();
            }
        }
    }

    // Save to file FIRST before triggering fetches (force=true to bypass throttle)
    saveConfiguration(true);

    // Reload config to ensure everything is fresh in memory
    Serial.println("Reloading configuration after save...");
    loadConfiguration();

    // Trigger forced fetches for modules that changed
    if (doc.containsKey("modules")) {
        JsonObject modules = doc["modules"];
        if (modules.containsKey("bitcoin") && modules["bitcoin"].containsKey("cryptoId")) {
            Serial.println("Requesting forced fetch for bitcoin module");
            scheduler.requestFetch("bitcoin", true);
        }
        if (modules.containsKey("ethereum") && modules["ethereum"].containsKey("cryptoId")) {
            Serial.println("Requesting forced fetch for ethereum module");
            scheduler.requestFetch("ethereum", true);
        }
    }

    server->send(200, "application/json", "{\"success\":true,\"message\":\"Settings saved. New data will load within 10 seconds.\"}");
}

void NetworkManager::handleRestart() {
    // Check authorization
    String token = server->header("Authorization");
    if (!security.validateSession(token)) {
        server->send(401, "application/json", "{\"error\":\"Unauthorized\"}");
        return;
    }

    server->send(200, "application/json", "{\"success\":true}");
    delay(500);
    ESP.restart();
}

void NetworkManager::handleFactoryReset() {
    // Check authorization
    String token = server->header("Authorization");
    if (!security.validateSession(token)) {
        server->send(401, "application/json", "{\"error\":\"Unauthorized\"}");
        return;
    }

    server->send(200, "application/json", "{\"success\":true}");
    delay(500);

    Serial.println("Factory reset via web interface");
    LittleFS.format();
    delay(1000);
    ESP.restart();
}
