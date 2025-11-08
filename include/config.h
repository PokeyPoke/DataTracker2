#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <LittleFS.h>

// Configuration file path
#define CONFIG_FILE "/config.json"

// Global configuration document (StaticJsonDocument allocated in .bss, not heap)
// Increased to 2KB to accommodate all crypto fields
extern StaticJsonDocument<3072> config;  // 3KB to handle long crypto/stock/location names

// Configuration management functions
bool initStorage();
bool loadConfiguration();
bool saveConfiguration(bool force = false);
void setDefaultConfig();

// Module cache functions
void updateModuleCache(const char* moduleId, JsonObject data);
bool isCacheStale(const char* moduleId);
unsigned long getCacheAge(const char* moduleId);

// Helper functions
String getTimeAgo(unsigned long timestamp);

#endif // CONFIG_H
