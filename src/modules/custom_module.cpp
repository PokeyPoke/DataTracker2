#include "module_interface.h"
#include "config.h"
#include <ArduinoJson.h>

class CustomModule : public ModuleInterface {
public:
    CustomModule() {
        id = "custom";
        displayName = "Custom Metric";
        defaultRefreshInterval = 0;    // No auto-refresh
        minRefreshInterval = 0;        // No restrictions
    }

    bool fetch(String& errorMsg) override {
        // Custom module doesn't fetch from external API
        // Value is set directly by user via config portal

        JsonObject data = config["modules"]["custom"];
        data["lastUpdate"] = millis() / 1000;
        data["lastSuccess"] = true;

        Serial.println("Custom module: No fetch needed (manual entry)");
        return true;
    }

    String formatDisplay() override {
        JsonObject data = config["modules"]["custom"];
        float value = data["value"] | 0.0;
        String label = data["label"] | "Custom";
        String unit = data["unit"] | "";

        char buffer[64];
        if (unit.length() > 0) {
            snprintf(buffer, sizeof(buffer), "%.2f %s", value, unit.c_str());
        } else {
            snprintf(buffer, sizeof(buffer), "%.2f", value);
        }
        return String(buffer);
    }
};
