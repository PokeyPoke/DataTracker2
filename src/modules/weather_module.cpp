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

        Serial.print("Weather: Fetching for location: ");
        Serial.println(location);
        Serial.print("Weather: Coordinates: lat=");
        Serial.print(lat, 4);
        Serial.print(", lon=");
        Serial.println(lon, 4);

        // Use Open-Meteo API (free, no auth required)
        String url = "https://api.open-meteo.com/v1/forecast?latitude=" + String(lat, 4) +
                     "&longitude=" + String(lon, 4) + "&current_weather=true";

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

        if (!doc.containsKey("current_weather")) {
            errorMsg = "Missing current_weather data";
            Serial.println("Weather: ERROR - Missing 'current_weather' in response");
            return false;
        }

        JsonObject currentWeather = doc["current_weather"];
        if (!currentWeather.containsKey("temperature")) {
            errorMsg = "Missing temperature data";
            Serial.println("Weather: ERROR - Missing 'temperature' in current_weather");
            return false;
        }

        float temp = currentWeather["temperature"] | 0.0;
        int weatherCode = currentWeather["weathercode"] | 0;

        Serial.print("Weather: Parsed - Temp: ");
        Serial.print(temp, 1);
        Serial.print("°C, Code: ");
        Serial.print(weatherCode);
        Serial.print(" (");
        Serial.print(getWeatherCondition(weatherCode));
        Serial.println(")");

        // Update cache
        JsonObject data = config["modules"]["weather"].to<JsonObject>();
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
