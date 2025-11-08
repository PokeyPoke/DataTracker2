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
        // Get location from config (format: "lat,lon" or uses defaults)
        JsonObject weatherData = config["modules"]["weather"];
        String location = weatherData["location"] | "37.7749,-122.4194";

        // Parse lat,lon from location string
        int commaIndex = location.indexOf(',');
        float lat = 37.7749;  // Default: San Francisco
        float lon = -122.4194;

        if (commaIndex > 0) {
            lat = location.substring(0, commaIndex).toFloat();
            lon = location.substring(commaIndex + 1).toFloat();
        }

        String url = "https://api.open-meteo.com/v1/forecast?latitude=" + String(lat, 4) +
                     "&longitude=" + String(lon, 4) + "&current_weather=true";

        String response;
        if (!network.httpGet(url.c_str(), response, errorMsg)) {
            return false;
        }

        return parseResponse(response, errorMsg);
    }

    bool parseResponse(String payload, String& errorMsg) {
        StaticJsonDocument<1024> doc;
        DeserializationError error = deserializeJson(doc, payload);

        if (error) {
            errorMsg = "JSON parse error: " + String(error.c_str());
            return false;
        }

        if (!doc.containsKey("current_weather")) {
            errorMsg = "Invalid response structure";
            return false;
        }

        JsonObject currentWeather = doc["current_weather"];
        float temp = currentWeather["temperature"];
        int weatherCode = currentWeather["weathercode"];

        // Update cache (preserve existing fields like location, latitude, longitude)
        JsonObject data = config["modules"]["weather"];
        data["temperature"] = temp;
        data["condition"] = getWeatherCondition(weatherCode);
        data["lastUpdate"] = millis() / 1000;
        data["lastSuccess"] = true;

        Serial.print("Weather: ");
        Serial.print(temp, 1);
        Serial.print("°C, ");
        Serial.println(getWeatherCondition(weatherCode));

        return true;
    }

    const char* getWeatherCondition(int code) {
        if (code == 0) return "Clear";
        if (code <= 3) return "Cloudy";
        if (code <= 67) return "Rain";
        if (code <= 77) return "Snow";
        if (code <= 99) return "Storm";
        return "Unknown";
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
