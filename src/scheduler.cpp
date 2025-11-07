#include "scheduler.h"
#include "modules/module_interface.h"
#include "config.h"

Scheduler::Scheduler() {
    context.state = IDLE;
    context.lastFetchTime = 0;
    context.nextAllowedFetch = 0;
    context.retryCount = 0;
    context.retryDelay = 0;
    lastGlobalFetch = 0;
}

Scheduler::~Scheduler() {
    // Clean up registered modules
    for (auto& pair : modules) {
        delete pair.second;
    }
    modules.clear();
}

void Scheduler::init() {
    Serial.println("Scheduler initialized");
}

void Scheduler::registerModule(ModuleInterface* module) {
    if (module && module->id) {
        modules[String(module->id)] = module;
        Serial.print("Registered module: ");
        Serial.println(module->id);
    }
}

void Scheduler::tick() {
    unsigned long now = millis() / 1000;

    // If currently fetching, let it complete
    if (context.state == FETCHING) {
        return;
    }

    // Check if it's time to auto-refresh the active module
    if (context.state == IDLE) {
        String activeModule = config["device"]["activeModule"] | "bitcoin";
        uint16_t refreshInterval = config["device"]["refreshInterval"] | 300;

        JsonObject moduleData = config["modules"][activeModule];
        unsigned long lastUpdate = moduleData["lastUpdate"] | 0;

        if (lastUpdate == 0 || (now - lastUpdate) >= refreshInterval) {
            // Time to refresh
            requestFetch(activeModule.c_str(), false);
        }
    }
}

void Scheduler::requestFetch(const char* moduleId, bool forced) {
    unsigned long now = millis() / 1000;

    // Check if module exists
    if (modules.find(String(moduleId)) == modules.end()) {
        Serial.print("ERROR: Module not found: ");
        Serial.println(moduleId);
        return;
    }

    ModuleInterface* module = modules[String(moduleId)];

    // Check global cooldown
    if (!forced && (now - lastGlobalFetch) < GLOBAL_MIN_INTERVAL) {
        Serial.println("Fetch denied: global cooldown active");
        return;
    }

    // Check module-specific cooldown
    JsonObject moduleData = config["modules"][moduleId];
    unsigned long lastUpdate = moduleData["lastUpdate"] | 0;
    if (!forced && (now - lastUpdate) < module->minRefreshInterval) {
        Serial.print("Fetch denied: module cooldown (last update ");
        Serial.print(now - lastUpdate);
        Serial.print("s ago, min interval ");
        Serial.print(module->minRefreshInterval);
        Serial.println("s)");
        return;
    }

    // Check retry backoff (unless forced)
    if (!forced && context.retryDelay > 0 && (now - context.lastFetchTime) < context.retryDelay) {
        Serial.print("Fetch denied: retry backoff (");
        Serial.print(context.retryDelay - (now - context.lastFetchTime));
        Serial.println("s remaining)");
        return;
    }

    // Approve fetch
    if (forced) {
        Serial.println("FORCED FETCH - bypassing all cooldowns");
    }
    context.currentModule = String(moduleId);
    context.state = FETCHING;
    executeFetch();
}

void Scheduler::executeFetch() {
    if (modules.find(context.currentModule) == modules.end()) {
        context.state = IDLE;
        return;
    }

    ModuleInterface* module = modules[context.currentModule];

    Serial.print("Fetching data for: ");
    Serial.println(context.currentModule);

    String errorMsg;
    bool success = module->fetch(errorMsg);

    unsigned long now = millis() / 1000;
    context.lastFetchTime = now;
    lastGlobalFetch = now;

    if (success) {
        Serial.println("Fetch successful");
        context.retryCount = 0;
        context.retryDelay = 0;

        JsonObject moduleData = config["modules"][context.currentModule];
        moduleData["lastSuccess"] = true;
    } else {
        Serial.print("Fetch failed: ");
        Serial.println(errorMsg);

        context.retryCount++;
        context.retryDelay = calculateBackoff(context.retryCount);

        JsonObject moduleData = config["modules"][context.currentModule];
        moduleData["lastSuccess"] = false;

        Serial.print("Retry count: ");
        Serial.print(context.retryCount);
        Serial.print(", next retry in ");
        Serial.print(context.retryDelay);
        Serial.println(" seconds");
    }

    context.state = IDLE;
}

uint16_t Scheduler::calculateBackoff(uint8_t retryCount) {
    // Exponential backoff: min(2^n × 60s, 3600s)
    uint16_t delay = 60 * (1 << retryCount);  // 2^n × 60
    return min(delay, (uint16_t)3600);
}
