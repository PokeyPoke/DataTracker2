#include "config.h"

// Global configuration document (StaticJsonDocument allocated in .bss, not heap)
// Increased to 2KB to accommodate all crypto fields
StaticJsonDocument<2048> config;

// Track last save time to reduce flash wear
static unsigned long lastSaveTime = 0;
#define MIN_SAVE_INTERVAL 30000  // Minimum 30s between saves

bool initStorage() {
    Serial.println("Initializing LittleFS...");

    // Try to mount first
    if (!LittleFS.begin(false)) {
        Serial.println("LittleFS mount failed, formatting...");
        // Format if mount fails
        if (!LittleFS.begin(true)) {
            Serial.println("ERROR: LittleFS format failed");
            return false;
        }
        Serial.println("LittleFS formatted successfully");
    }

    Serial.println("LittleFS mounted successfully");

    // Create config file if it doesn't exist
    if (!LittleFS.exists(CONFIG_FILE)) {
        Serial.println("Creating default config file...");
        File file = LittleFS.open(CONFIG_FILE, "w");
        if (file) {
            file.print("{}");  // Empty JSON object
            file.close();
            Serial.println("Default config file created");
        }
    }

    return true;
}

bool loadConfiguration() {
    File file = LittleFS.open(CONFIG_FILE, "r");
    if (!file) {
        Serial.println("Config file not found, creating default config");
        setDefaultConfig();
        saveConfiguration();
        return false;
    }

    DeserializationError error = deserializeJson(config, file);
    file.close();

    if (error) {
        Serial.print("Failed to parse config: ");
        Serial.println(error.c_str());
        setDefaultConfig();
        saveConfiguration();
        return false;
    }

    Serial.println("Configuration loaded successfully");

    // Debug: Show what was loaded for crypto modules
    Serial.println("=== Loaded Config (Crypto Modules) ===");
    JsonObject bitcoin = config["modules"]["bitcoin"];
    Serial.print("Bitcoin - ID: ");
    Serial.print(bitcoin["cryptoId"] | "NOT SET");
    Serial.print(", Name: ");
    Serial.print(bitcoin["cryptoName"] | "NOT SET");
    Serial.print(", Symbol: ");
    Serial.println(bitcoin["cryptoSymbol"] | "NOT SET");

    JsonObject ethereum = config["modules"]["ethereum"];
    Serial.print("Ethereum - ID: ");
    Serial.print(ethereum["cryptoId"] | "NOT SET");
    Serial.print(", Name: ");
    Serial.print(ethereum["cryptoName"] | "NOT SET");
    Serial.print(", Symbol: ");
    Serial.println(ethereum["cryptoSymbol"] | "NOT SET");
    Serial.println("=======================================");

    // Check if config is overflowing
    Serial.print("Config memory usage: ");
    Serial.print(config.memoryUsage());
    Serial.print(" / ");
    Serial.print(config.capacity());
    Serial.println(" bytes");
    if (config.overflowed()) {
        Serial.println("WARNING: Config document overflowed! Some data may be lost!");
    }

    return true;
}

bool saveConfiguration(bool force) {
    // Throttle saves to reduce flash wear (unless forced)
    unsigned long now = millis();
    if (!force && now - lastSaveTime < MIN_SAVE_INTERVAL) {
        Serial.println("Skipping save (too soon since last save)");
        return true;  // Not an error, just throttled
    }

    if (force) {
        Serial.println("FORCED SAVE - bypassing throttle");
    }

    File file = LittleFS.open(CONFIG_FILE, "w");
    if (!file) {
        Serial.println("ERROR: Failed to open config file for writing");
        return false;
    }

    if (serializeJson(config, file) == 0) {
        Serial.println("ERROR: Failed to write config");
        file.close();
        return false;
    }

    file.close();
    lastSaveTime = now;
    Serial.println("Configuration saved successfully");

    // Debug: Show what was saved for crypto modules
    Serial.println("=== Saved Config (Crypto Modules) ===");
    JsonObject bitcoin = config["modules"]["bitcoin"];
    Serial.print("Bitcoin - ID: ");
    Serial.print(bitcoin["cryptoId"] | "NOT SET");
    Serial.print(", Name: ");
    Serial.print(bitcoin["cryptoName"] | "NOT SET");
    Serial.print(", Symbol: ");
    Serial.println(bitcoin["cryptoSymbol"] | "NOT SET");

    JsonObject ethereum = config["modules"]["ethereum"];
    Serial.print("Ethereum - ID: ");
    Serial.print(ethereum["cryptoId"] | "NOT SET");
    Serial.print(", Name: ");
    Serial.print(ethereum["cryptoName"] | "NOT SET");
    Serial.print(", Symbol: ");
    Serial.println(ethereum["cryptoSymbol"] | "NOT SET");
    Serial.println("======================================");

    // Check if config is overflowing
    Serial.print("Config memory usage: ");
    Serial.print(config.memoryUsage());
    Serial.print(" / ");
    Serial.print(config.capacity());
    Serial.println(" bytes");
    if (config.overflowed()) {
        Serial.println("ERROR: Config document overflowed during save! Data loss occurred!");
    }

    return true;
}

void setDefaultConfig() {
    // Clear existing config
    config.clear();

    // WiFi settings
    config["wifi"]["ssid"] = "";
    config["wifi"]["password"] = "";

    // Device settings
    config["device"]["activeModule"] = "bitcoin";
    config["device"]["enableButton"] = true;
    config["device"]["refreshInterval"] = 300;  // 5 minutes default

    // Initialize empty module data
    JsonObject bitcoin = config["modules"]["bitcoin"].to<JsonObject>();
    bitcoin["cryptoId"] = "bitcoin";
    bitcoin["cryptoSymbol"] = "BTC";
    bitcoin["cryptoName"] = "Bitcoin";
    bitcoin["value"] = 0.0;
    bitcoin["change24h"] = 0.0;
    bitcoin["lastUpdate"] = 0;
    bitcoin["lastSuccess"] = false;

    JsonObject ethereum = config["modules"]["ethereum"].to<JsonObject>();
    ethereum["cryptoId"] = "ethereum";
    ethereum["cryptoSymbol"] = "ETH";
    ethereum["cryptoName"] = "Ethereum";
    ethereum["value"] = 0.0;
    ethereum["change24h"] = 0.0;
    ethereum["lastUpdate"] = 0;
    ethereum["lastSuccess"] = false;

    JsonObject stock = config["modules"]["stock"].to<JsonObject>();
    stock["ticker"] = "AAPL";
    stock["value"] = 0.0;
    stock["change"] = 0.0;
    stock["lastUpdate"] = 0;
    stock["lastSuccess"] = false;

    JsonObject weather = config["modules"]["weather"].to<JsonObject>();
    weather["latitude"] = 37.7749;
    weather["longitude"] = -122.4194;
    weather["location"] = "San Francisco";
    weather["temperature"] = 0.0;
    weather["condition"] = "Unknown";
    weather["lastUpdate"] = 0;
    weather["lastSuccess"] = false;

    JsonObject custom = config["modules"]["custom"].to<JsonObject>();
    custom["value"] = 0.0;
    custom["label"] = "My Metric";
    custom["unit"] = "units";
    custom["lastUpdate"] = 0;
    custom["lastSuccess"] = true;

    Serial.println("Default configuration created");
}

void updateModuleCache(const char* moduleId, JsonObject data) {
    JsonObject module = config["modules"][moduleId];

    // Update timestamp
    module["lastUpdate"] = millis() / 1000;
    module["lastSuccess"] = true;

    // Copy all data fields
    for (JsonPair kv : data) {
        module[kv.key()] = kv.value();
    }

    // Request save (will be throttled if too frequent)
    saveConfiguration();
}

bool isCacheStale(const char* moduleId) {
    JsonObject module = config["modules"][moduleId];
    unsigned long lastUpdate = module["lastUpdate"] | 0;
    unsigned long now = millis() / 1000;
    uint16_t refreshInterval = config["device"]["refreshInterval"] | 300;

    // Cache is stale if older than 2× refresh interval
    return (now - lastUpdate) > (refreshInterval * 2);
}

unsigned long getCacheAge(const char* moduleId) {
    JsonObject module = config["modules"][moduleId];
    unsigned long lastUpdate = module["lastUpdate"] | 0;
    unsigned long now = millis() / 1000;
    return now - lastUpdate;
}

String getTimeAgo(unsigned long timestamp) {
    if (timestamp == 0) return "Never";

    unsigned long now = millis() / 1000;
    unsigned long diff = now - timestamp;

    if (diff < 60) return String(diff) + "s ago";
    if (diff < 3600) return String(diff / 60) + "m ago";
    if (diff < 86400) return String(diff / 3600) + "h ago";
    return String(diff / 86400) + "d ago";
}
