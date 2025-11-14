#include "module_interface.h"
#include "config.h"
#include "network.h"
#include <ArduinoJson.h>

extern NetworkManager network;

class WeatherModule : public ModuleInterface {
public:
    WeatherModule() {
        id = "weather";
        displayName = "Weather";
        defaultRefreshInterval = 900;  // 15 minutes
        minRefreshInterval = 300;      // 5 minutes
    }

    bool fetch(String& errorMsg) override {
        Serial.println("\n*** WeatherModule::fetch() CALLED ***");

        // Get location from config - use separate latitude/longitude fields
        JsonObject weatherData = config["modules"]["weather"];

        // Read location from config (set by settings page)
        String location = weatherData["location"] | "San Francisco";

        Serial.print("Weather: Fetching for location: ");
        Serial.println(location);

        // Use wttr.in API (free, no auth required, accurate data)
        // Format: ?format=j1 returns JSON with current weather
        String url = "https://wttr.in/" + location + "?format=j1";

        Serial.print("Weather: URL = ");
        Serial.println(url);

        String response;
        if (!network.httpGet(url.c_str(), response, errorMsg)) {
            Serial.print("Weather: HTTP request failed - ");
            Serial.println(errorMsg);
            return false;
        }

        Serial.print("Weather: Response length = ");
        Serial.println(response.length());
        Serial.print("Weather: Parsing response...");
        bool success = parseResponse(response, errorMsg);
        if (!success) {
            Serial.print("Weather: Parse failed - ");
            Serial.println(errorMsg);
        } else {
            Serial.println("Weather: Parse successful");
        }
        return success;
    }

    bool parseResponse(String payload, String& errorMsg) {
        Serial.print("Weather: parseResponse called with payload length: ");
        Serial.println(payload.length());

        if (payload.length() == 0) {
            errorMsg = "Empty response";
            Serial.println("Weather: ERROR - Empty response payload");
            return false;
        }

        // Print first 200 chars of response for debugging
        Serial.print("Weather: Response preview: ");
        Serial.println(payload.substring(0, min(200, (int)payload.length())));

        StaticJsonDocument<1024> doc;
        DeserializationError error = deserializeJson(doc, payload);

        if (error) {
            errorMsg = "JSON parse error: " + String(error.c_str());
            Serial.print("Weather: JSON parse error: ");
            Serial.println(error.c_str());
            return false;
        }

        Serial.println("Weather: JSON parsed successfully");

        // wttr.in API format
        if (!doc.containsKey("current_condition")) {
            errorMsg = "Missing current_condition data";
            Serial.println("Weather: ERROR - Missing 'current_condition' in response");
            return false;
        }

        JsonArray currentCondition = doc["current_condition"];
        if (currentCondition.size() == 0) {
            errorMsg = "Empty current_condition array";
            Serial.println("Weather: ERROR - Empty 'current_condition' array");
            return false;
        }

        JsonObject current = currentCondition[0];
        if (!current.containsKey("temp_C")) {
            errorMsg = "Missing temperature data";
            Serial.println("Weather: ERROR - Missing 'temp_C' in current_condition");
            return false;
        }

        // wttr.in returns temperature as string
        float temp = atof(current["temp_C"] | "0");

        // Get weather condition
        String condition = current["weatherDesc"][0]["value"] | "Unknown";

        Serial.print("Weather: Parsed - Temp: ");
        Serial.print(temp, 1);
        Serial.print("°C, Condition: ");
        Serial.println(condition);

        // Update cache (use reference, NOT .to<JsonObject>() which clears everything!)
        JsonObject data = config["modules"]["weather"];
        data["temperature"] = temp;
        data["condition"] = condition;
        data["lastUpdate"] = millis() / 1000;
        data["lastSuccess"] = true;

        Serial.print("Weather: ");
        Serial.print(temp, 1);
        Serial.print("°C, ");
        Serial.println(condition);

        return true;
    }

    String formatDisplay() override {
        JsonObject data = config["modules"]["weather"];
        float temp = data["temperature"] | 0.0;
        String condition = data["condition"] | "Unknown";
        String location = data["location"] | "Unknown";

        char buffer[64];
        snprintf(buffer, sizeof(buffer), "%.1f°C | %s | %s", temp, condition.c_str(), location.c_str());
        return String(buffer);
    }
};
