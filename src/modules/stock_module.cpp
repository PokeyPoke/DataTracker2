#include "module_interface.h"
#include "config.h"
#include "network.h"
#include <ArduinoJson.h>

extern NetworkManager network;

class StockModule : public ModuleInterface {
public:
    StockModule() {
        id = "stock";
        displayName = "Stock Price";
        defaultRefreshInterval = 300;  // 5 minutes
        minRefreshInterval = 60;       // 1 minute
    }

    bool fetch(String& errorMsg) override {
        // Get ticker from config
        JsonObject stockData = config["modules"]["stock"];
        String ticker = stockData["ticker"] | "AAPL";

        Serial.print("Stock: Fetching price for ");
        Serial.println(ticker);

        if (ticker.length() == 0) {
            errorMsg = "Ticker is empty";
            Serial.println("Stock: ERROR - ticker is empty");
            return false;
        }

        String url = "https://query1.finance.yahoo.com/v7/finance/quote?symbols=" + ticker;
        Serial.print("Stock: URL = ");
        Serial.println(url);

        String response;
        if (!network.httpGetWithHeaders(url.c_str(), response, errorMsg)) {
            Serial.print("Stock: HTTP request failed - ");
            Serial.println(errorMsg);
            return false;
        }

        Serial.print("Stock: Response length = ");
        Serial.println(response.length());
        Serial.print("Stock: Response received, parsing... ");
        bool success = parseResponse(response, errorMsg);
        if (!success) {
            Serial.print("Stock: Parse failed - ");
            Serial.println(errorMsg);
        } else {
            Serial.println("Stock: Parse successful");
        }
        return success;
    }

    bool parseResponse(String payload, String& errorMsg) {
        Serial.print("Stock: parseResponse called with payload length: ");
        Serial.println(payload.length());

        if (payload.length() == 0) {
            errorMsg = "Empty response";
            Serial.println("Stock: ERROR - Empty response payload");
            return false;
        }

        // Print first 100 chars of response for debugging
        Serial.print("Stock: Response preview: ");
        Serial.println(payload.substring(0, 100));

        DynamicJsonDocument doc(2048);
        DeserializationError error = deserializeJson(doc, payload);

        if (error) {
            errorMsg = "JSON parse error: " + String(error.c_str());
            Serial.print("Stock: JSON parse error: ");
            Serial.println(error.c_str());
            return false;
        }

        Serial.println("Stock: JSON parsed successfully");

        if (!doc.containsKey("quoteResponse")) {
            errorMsg = "Missing quoteResponse";
            Serial.println("Stock: ERROR - Missing quoteResponse key");
            return false;
        }

        if (!doc["quoteResponse"].containsKey("result")) {
            errorMsg = "Missing result array";
            Serial.println("Stock: ERROR - Missing result array");
            return false;
        }

        JsonArray results = doc["quoteResponse"]["result"];
        Serial.print("Stock: Results array size: ");
        Serial.println(results.size());

        if (results.size() == 0) {
            errorMsg = "Invalid ticker symbol";
            Serial.println("Stock: ERROR - results array is empty");
            return false;
        }

        JsonObject quote = results[0];
        float price = quote["regularMarketPrice"] | 0.0;
        float change = quote["regularMarketChangePercent"] | 0.0;
        String symbol = quote["symbol"] | "N/A";

        Serial.print("Stock: Parsed - Symbol: ");
        Serial.print(symbol);
        Serial.print(", Price: ");
        Serial.print(price, 2);
        Serial.print(", Change: ");
        Serial.println(change, 2);

        // Update cache
        JsonObject data = config["modules"]["stock"].to<JsonObject>();
        data["value"] = price;
        data["change"] = change;
        data["ticker"] = symbol;
        data["lastUpdate"] = millis() / 1000;
        data["lastSuccess"] = true;

        Serial.print(symbol);
        Serial.print(" price: $");
        Serial.print(price, 2);
        Serial.print(" (");
        Serial.print(change, 2);
        Serial.println("%)");

        return true;
    }

    String formatDisplay() override {
        JsonObject data = config["modules"]["stock"];
        float price = data["value"] | 0.0;
        float change = data["change"] | 0.0;
        String ticker = data["ticker"] | "N/A";

        char buffer[64];
        snprintf(buffer, sizeof(buffer), "%s: $%.2f | %+.1f%%", ticker.c_str(), price, change);
        return String(buffer);
    }
};
