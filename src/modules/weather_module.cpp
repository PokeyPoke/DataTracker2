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

        // Read latitude and longitude from config (set by settings page)
        float lat = weatherData["latitude"] | 37.7749;   // Default: San Francisco
        float lon = weatherData["longitude"] | -122.4194;
        String location = weatherData["location"] | "San Francisco";
        String apiKey = weatherData["apiKey"] | "";  // OpenWeatherMap API key

        Serial.print("Weather: Fetching for location: ");
        Serial.println(location);
        Serial.print("Weather: Coordinates: lat=");
        Serial.print(lat, 4);
        Serial.print(", lon=");
        Serial.println(lon, 4);

        // Use OpenWeatherMap API (requires free API key)
        // Get key at: https://openweathermap.org/api
        String url = "https://api.openweathermap.org/data/2.5/weather?lat=" + String(lat, 4) +
                     "&lon=" + String(lon, 4) + "&units=metric&appid=" + apiKey;

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

        // OpenWeatherMap API format
        if (!doc.containsKey("main")) {
            errorMsg = "Missing main data";
            Serial.println("Weather: ERROR - Missing 'main' in response");
            return false;
        }

        JsonObject main = doc["main"];
        if (!main.containsKey("temp")) {
            errorMsg = "Missing temperature data";
            Serial.println("Weather: ERROR - Missing 'temp' in main");
            return false;
        }

        float temp = main["temp"] | 0.0;

        // Get weather condition
        String condition = "Unknown";
        if (doc.containsKey("weather") && doc["weather"].size() > 0) {
            condition = doc["weather"][0]["main"].as<String>();
        }

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
