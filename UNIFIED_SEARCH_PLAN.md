# Unified Search & Advanced Custom Module Plan
**Date:** November 3, 2024
**Version:** 3.0 Proposal

## Overview

This document outlines three major UX improvements:
1. **Unified Search/Autocomplete** for Crypto, Stock, and Weather
2. **Advanced Custom Module** with markup language for dynamic content
3. **Loading States** - Show "Updating..." instead of zeros/fake values

---

## Part 1: Unified Search Experience

### Goal
Provide a **consistent, user-friendly search interface** for all data modules:
- **Crypto:** Search 10,000+ cryptocurrencies by name
- **Stock:** Search stocks by company name or ticker
- **Weather:** Search cities worldwide

### Current State (Inconsistent)

| Module | Current Input | User Experience |
|--------|---------------|-----------------|
| Crypto | N/A (hardcoded) | ❌ Can't search |
| Stock | Text field "AAPL" | ⚠️ Must know ticker |
| Weather | Text field "lat,lon" | ❌ Must know coordinates |

### Proposed State (Unified)

| Module | New Input | User Experience |
|--------|-----------|-----------------|
| Crypto | Search "Bitcoin" | ✅ Autocomplete dropdown |
| Stock | Search "Apple" or "AAPL" | ✅ Autocomplete dropdown |
| Weather | Search "San Francisco" | ✅ Autocomplete dropdown |

---

## Design Mockups

### Settings Page - New Search Fields

```
┌─────────────────────────────────────────────────────┐
│  DataTracker Settings                               │
├─────────────────────────────────────────────────────┤
│                                                      │
│  🔍 Crypto 1                                        │
│  ┌──────────────────────────────────────────────┐  │
│  │ Search cryptocurrency...              [🔍]   │  │
│  └──────────────────────────────────────────────┘  │
│  ┌─ Suggestions ──────────────────────────────┐    │
│  │ • Bitcoin (BTC)                  $43,250    │    │
│  │ • Ethereum (ETH)                 $2,280     │    │
│  │ • Solana (SOL)                   $98.50     │    │
│  │ • Cardano (ADA)                  $0.45      │    │
│  └──────────────────────────────────────────────┘   │
│  Current: Bitcoin (BTC)                             │
│                                                      │
│  🔍 Stock                                            │
│  ┌──────────────────────────────────────────────┐  │
│  │ Search company or ticker...       [🔍]       │  │
│  └──────────────────────────────────────────────┘  │
│  ┌─ Suggestions ──────────────────────────────┐    │
│  │ • Apple Inc. (AAPL)              $178.25    │    │
│  │ • Tesla, Inc. (TSLA)             $242.84    │    │
│  │ • Microsoft Corp. (MSFT)         $368.50    │    │
│  └──────────────────────────────────────────────┘   │
│  Current: Apple Inc. (AAPL)                         │
│                                                      │
│  🔍 Weather                                          │
│  ┌──────────────────────────────────────────────┐  │
│  │ Search city...                    [🔍]       │  │
│  └──────────────────────────────────────────────┘  │
│  ┌─ Suggestions ──────────────────────────────┐    │
│  │ • San Francisco, CA, USA                    │    │
│  │ • San Francisco, CO, USA                    │    │
│  │ • San José, CA, USA                         │    │
│  └──────────────────────────────────────────────┘   │
│  Current: San Francisco, CA (37.77, -122.41)       │
│                                                      │
│  [Save Changes]                                     │
└─────────────────────────────────────────────────────┘
```

---

## Implementation Details

### A. Crypto Search

**API:** CoinGecko `/coins/list` and `/search`

**Flow:**
1. User types "bit" in search box
2. JavaScript debounces (300ms wait)
3. Fetch: `https://api.coingecko.com/api/v3/search?query=bit`
4. Show results:
   ```
   Bitcoin (BTC)
   Bitcoin Cash (BCH)
   BitTorrent (BTT)
   ```
5. User clicks "Bitcoin (BTC)"
6. Save to config: `{id: "bitcoin", symbol: "BTC", name: "Bitcoin"}`

**Code Snippet:**
```javascript
let searchTimeout;
document.getElementById('cryptoSearch').addEventListener('input', (e) => {
    clearTimeout(searchTimeout);
    searchTimeout = setTimeout(() => {
        searchCrypto(e.target.value);
    }, 300);
});

async function searchCrypto(query) {
    if (query.length < 2) return;

    const res = await fetch(`https://api.coingecko.com/api/v3/search?query=${query}`);
    const data = await res.json();

    const results = data.coins.slice(0, 5); // Top 5 results
    showCryptoSuggestions(results);
}

function showCryptoSuggestions(results) {
    const html = results.map(coin => `
        <div class="suggestion" onclick="selectCrypto('${coin.id}', '${coin.symbol}', '${coin.name}')">
            <img src="${coin.thumb}" width="24" height="24">
            ${coin.name} (${coin.symbol.toUpperCase()})
        </div>
    `).join('');
    document.getElementById('cryptoSuggestions').innerHTML = html;
}
```

**Backend Storage:**
```json
{
  "modules": {
    "crypto1": {
      "id": "bitcoin",
      "symbol": "BTC",
      "name": "Bitcoin"
    }
  }
}
```

---

### B. Stock Search

**API Option 1:** Yahoo Finance Autocomplete (Free, no key)
- Endpoint: `https://query1.finance.yahoo.com/v1/finance/search?q=QUERY`

**API Option 2:** Alpha Vantage (Free, requires key)
- Endpoint: `https://www.alphavantage.co/query?function=SYMBOL_SEARCH&keywords=QUERY&apikey=demo`

**Flow:**
1. User types "appl"
2. Fetch: `/v1/finance/search?q=appl`
3. Show results:
   ```
   Apple Inc. (AAPL)
   Applied Materials (AMAT)
   Applebee's International (APPB)
   ```
4. User clicks "Apple Inc. (AAPL)"
5. Save: `{ticker: "AAPL", name: "Apple Inc."}`

**Code Snippet:**
```javascript
async function searchStock(query) {
    if (query.length < 2) return;

    const res = await fetch(`https://query1.finance.yahoo.com/v1/finance/search?q=${query}`);
    const data = await res.json();

    const results = data.quotes.slice(0, 5);
    showStockSuggestions(results);
}

function showStockSuggestions(results) {
    const html = results.map(stock => `
        <div class="suggestion" onclick="selectStock('${stock.symbol}', '${stock.longname}')">
            ${stock.longname || stock.shortname} (${stock.symbol})
        </div>
    `).join('');
    document.getElementById('stockSuggestions').innerHTML = html;
}
```

---

### C. Weather City Search

**API Option 1:** OpenWeather Geocoding (Free, no key for basic)
- Endpoint: `http://api.openweathermap.org/geo/1.0/direct?q=CITY&limit=5`

**API Option 2:** Backend-hosted City Database
- Pre-load 500 major cities into a JSON file
- Client-side filtering (instant, no API calls)

**Flow:**
1. User types "san"
2. Fetch: `/geo/1.0/direct?q=san&limit=5`
3. Show results:
   ```
   San Francisco, CA, USA (37.77, -122.42)
   San Diego, CA, USA (32.71, -117.16)
   San Jose, CA, USA (37.34, -121.89)
   ```
4. User clicks "San Francisco, CA, USA"
5. Save: `{city: "San Francisco", country: "US", lat: 37.77, lon: -122.42}`

**Code Snippet:**
```javascript
async function searchCity(query) {
    if (query.length < 2) return;

    // Option 1: API call
    const res = await fetch(`http://api.openweathermap.org/geo/1.0/direct?q=${query}&limit=5`);
    const cities = await res.json();

    // Option 2: Local database
    const cities = cityDatabase.filter(c =>
        c.name.toLowerCase().includes(query.toLowerCase())
    ).slice(0, 5);

    showCitySuggestions(cities);
}

function showCitySuggestions(cities) {
    const html = cities.map(city => `
        <div class="suggestion" onclick="selectCity('${city.name}', ${city.lat}, ${city.lon})">
            ${city.name}, ${city.state || ''} ${city.country}
        </div>
    `).join('');
    document.getElementById('citySuggestions').innerHTML = html;
}
```

**Fallback:** If API fails, show manual lat/lon input:
```html
<small>
  Can't find your city? <a onclick="showManualCoords()">Enter coordinates manually</a>
</small>
```

---

### D. Unified Component CSS

```css
.search-container {
    position: relative;
    margin: 15px 0;
}

.search-input {
    width: 100%;
    padding: 12px;
    font-size: 16px;
    border: 2px solid #ddd;
    border-radius: 8px;
    transition: border-color 0.3s;
}

.search-input:focus {
    border-color: #4CAF50;
    outline: none;
}

.suggestions {
    position: absolute;
    top: 100%;
    left: 0;
    right: 0;
    background: white;
    border: 1px solid #ddd;
    border-top: none;
    border-radius: 0 0 8px 8px;
    max-height: 200px;
    overflow-y: auto;
    box-shadow: 0 4px 8px rgba(0,0,0,0.1);
    z-index: 1000;
    display: none;
}

.suggestions.active {
    display: block;
}

.suggestion {
    padding: 12px;
    cursor: pointer;
    display: flex;
    align-items: center;
    gap: 10px;
    transition: background 0.2s;
}

.suggestion:hover {
    background: #f5f5f5;
}

.suggestion img {
    border-radius: 50%;
}

.current-selection {
    margin-top: 8px;
    padding: 8px;
    background: #e8f5e9;
    border-radius: 4px;
    font-size: 14px;
    color: #2e7d32;
}
```

---

## Part 2: Advanced Custom Module with Markup Language

### Goal
Transform the Custom module from "static number display" to "dynamic content engine" supporting:
- Countdown timers
- Notes/text
- Progress bars
- Date calculations
- Conditional displays

### Custom Module Markup Language (CTML)

**Syntax:** Simple, readable, ESP32-friendly

#### Basic Syntax

```
type:value:options
```

**Examples:**
```
countdown:2024-12-25:Christmas
note:Remember to call Mom!
days:2024-01-01:Days in 2024
timer:00:30:00:Pomodoro
progress:750:1000:Steps
percent:75:Goal
value:42:Answer
```

---

### Markup Types

#### 1. `countdown` - Days Until Event

**Format:** `countdown:YYYY-MM-DD:Label`

**Examples:**
```
countdown:2024-12-25:Christmas
countdown:2025-07-04:July 4th
countdown:2024-11-28:Thanksgiving
```

**Display:**
```
┌──────────────────────────┐
│      Christmas           │
│         25               │  ← Large number
│       days               │
└──────────────────────────┘
```

**Code Logic:**
```cpp
if (markup.startsWith("countdown:")) {
    String[] parts = markup.split(":");
    String dateStr = parts[1];  // "2024-12-25"
    String label = parts[2];     // "Christmas"

    int daysUntil = calculateDaysUntil(dateStr);

    display.showCustomCountdown(label, daysUntil);
}
```

---

#### 2. `note` - Static Text Display

**Format:** `note:Your text here`

**Examples:**
```
note:Remember to smile!
note:Coffee break at 3pm
note:Deploy v2.0 today
```

**Display:**
```
┌──────────────────────────┐
│   Remember to smile!     │
│                          │
└──────────────────────────┘
```

---

#### 3. `days` - Days Since/Until

**Format:** `days:YYYY-MM-DD:Label`

**Examples:**
```
days:2024-01-01:Days in 2024
days:2023-06-15:Days since launch
```

**Display (if date is past):**
```
┌──────────────────────────┐
│   Days since launch      │
│         142              │
└──────────────────────────┘
```

**Display (if date is future):**
```
┌──────────────────────────┐
│   Days until launch      │
│         28               │
└──────────────────────────┘
```

---

#### 4. `timer` - Countdown Timer

**Format:** `timer:HH:MM:SS:Label`

**Examples:**
```
timer:00:25:00:Pomodoro
timer:01:00:00:Meeting
timer:00:05:00:Stretch break
```

**Display:**
```
┌──────────────────────────┐
│       Pomodoro           │
│        23:15             │  ← Counts down
│                          │
└──────────────────────────┘
```

**Behavior:**
- Starts when module is activated
- Counts down in real-time
- Beeps/flashes when complete? (optional)

---

#### 5. `progress` - Progress Bar

**Format:** `progress:current:total:Label`

**Examples:**
```
progress:750:1000:Steps today
progress:45:100:Project %
progress:8:10:Books read
```

**Display:**
```
┌──────────────────────────┐
│     Steps today          │
│   750 / 1000             │
│ ████████████░░░░░ 75%    │  ← Progress bar
└──────────────────────────┘
```

---

#### 6. `percent` - Simple Percentage

**Format:** `percent:value:Label`

**Examples:**
```
percent:85:Battery
percent:42:Humidity
percent:67:Progress
```

**Display:**
```
┌──────────────────────────┐
│       Battery            │
│         85%              │
│                          │
└──────────────────────────┘
```

---

#### 7. `value` - Traditional Number Display

**Format:** `value:number:Label:Unit`

**Examples:**
```
value:42:Temperature:°F
value:9.8:Speed:mph
value:1337:Score:pts
```

**Display:**
```
┌──────────────────────────┐
│     Temperature          │
│       42°F               │
│                          │
└──────────────────────────┘
```

---

### Settings Page UI for Custom Module

```html
<h3>🎨 Custom Module</h3>

<label>Display Type:</label>
<select id="customType" onchange="updateCustomPreview()">
  <option value="countdown">Countdown Timer</option>
  <option value="note">Text Note</option>
  <option value="days">Days Since/Until</option>
  <option value="timer">Timer</option>
  <option value="progress">Progress Bar</option>
  <option value="percent">Percentage</option>
  <option value="value">Number with Label</option>
</select>

<!-- Dynamic form fields based on selection -->
<div id="customFields">
  <!-- For countdown: -->
  <label>Event Date:</label>
  <input type="date" id="customDate">
  <label>Label:</label>
  <input type="text" id="customLabel" placeholder="Christmas">
</div>

<!-- Preview -->
<div class="preview-box">
  <strong>Preview:</strong>
  <div id="customPreview">
    <!-- Shows what will display -->
  </div>
</div>

<!-- Raw markup (for advanced users) -->
<details>
  <summary>Advanced: Edit Markup Directly</summary>
  <label>Markup Code:</label>
  <input type="text" id="customMarkup" placeholder="countdown:2024-12-25:Christmas">
  <small>Format: type:value:options</small>
</details>
```

---

### Backend Implementation

**Parse Markup in custom_module.cpp:**

```cpp
String formatDisplay() override {
    JsonObject data = config["modules"]["custom"];
    String markup = data["markup"] | "value:0:Custom:";

    // Parse markup type
    int firstColon = markup.indexOf(':');
    String type = markup.substring(0, firstColon);
    String params = markup.substring(firstColon + 1);

    if (type == "countdown") {
        return formatCountdown(params);
    } else if (type == "note") {
        return formatNote(params);
    } else if (type == "days") {
        return formatDays(params);
    } else if (type == "timer") {
        return formatTimer(params);
    } else if (type == "progress") {
        return formatProgress(params);
    } else if (type == "percent") {
        return formatPercent(params);
    } else {
        return formatValue(params);
    }
}

String formatCountdown(String params) {
    // Parse: "2024-12-25:Christmas"
    int colon = params.indexOf(':');
    String dateStr = params.substring(0, colon);
    String label = params.substring(colon + 1);

    int days = calculateDaysUntil(dateStr);

    char buffer[64];
    snprintf(buffer, sizeof(buffer), "%s: %d days", label.c_str(), days);
    return String(buffer);
}

int calculateDaysUntil(String dateStr) {
    // Parse "2024-12-25" into year, month, day
    int year = dateStr.substring(0, 4).toInt();
    int month = dateStr.substring(5, 7).toInt();
    int day = dateStr.substring(8, 10).toInt();

    // Get current time
    time_t now = time(nullptr);
    struct tm* current = localtime(&now);

    // Target date
    struct tm target = {0};
    target.tm_year = year - 1900;
    target.tm_mon = month - 1;
    target.tm_mday = day;

    // Calculate difference
    time_t targetTime = mktime(&target);
    double diff = difftime(targetTime, now);

    return (int)(diff / 86400); // Convert seconds to days
}
```

---

### Custom Module Examples

**Example 1: Christmas Countdown**
```
Markup: countdown:2024-12-25:Christmas
Display: "Christmas: 52 days"
```

**Example 2: Project Progress**
```
Markup: progress:45:100:Website redesign
Display: "Website redesign | 45/100 | [███████████░░░░░] 45%"
```

**Example 3: Daily Reminder**
```
Markup: note:Drink 8 glasses of water!
Display: "Drink 8 glasses of water!"
```

**Example 4: Pomodoro Timer**
```
Markup: timer:00:25:00:Focus time
Display: "Focus time | 23:45" (counting down)
```

**Example 5: Days Since Event**
```
Markup: days:2024-01-01:New Year
Display: "Days since New Year: 307"
```

---

## Part 3: Loading States - Show "Updating..." Instead of Zeros

### Problem

Currently, modules display `$0.00` or `0%` before data is fetched, which looks broken.

**Examples:**
```
Bitcoin: $0.00 | +0.0%      ← Looks like error
Stock: $0.00 | +0.0%        ← Confusing
Weather: 0.0°C | Unknown    ← Scary
```

### Solution

Show "Updating..." or module-specific loading states until real data arrives.

---

### Implementation

#### A. Add Loading State Check

**In module display logic:**

```cpp
String formatDisplay() override {
    JsonObject data = config["modules"]["bitcoin"];
    bool hasData = data["lastSuccess"] | false;

    if (!hasData) {
        return "Updating...";
    }

    float price = data["value"] | 0.0;
    float change = data["change24h"] | 0.0;

    char buffer[64];
    snprintf(buffer, sizeof(buffer), "$%.2f | %+.1f%%", price, change);
    return String(buffer);
}
```

---

#### B. Module-Specific Loading Messages

Make loading states more informative:

| Module | Loading State |
|--------|---------------|
| Bitcoin | "Fetching BTC..." |
| Ethereum | "Fetching ETH..." |
| Stock | "Loading AAPL..." |
| Weather | "Getting weather..." |
| Custom | "Ready" or blank |

**Example:**
```cpp
String formatDisplay() override {
    JsonObject data = config["modules"]["stock"];
    bool hasData = data["lastSuccess"] | false;

    if (!hasData) {
        String ticker = data["ticker"] | "stock";
        return "Loading " + ticker + "...";
    }

    // ... normal display
}
```

---

#### C. Visual Loading Indicator

**Option 1: Animated Dots**
```
Bitcoin
Updating.
Updating..
Updating...
```

**Option 2: Spinner**
```
⠋ Loading Bitcoin
⠙ Loading Bitcoin
⠹ Loading Bitcoin
⠸ Loading Bitcoin
⠼ Loading Bitcoin
⠴ Loading Bitcoin
⠦ Loading Bitcoin
⠧ Loading Bitcoin
```

**Option 3: Progress Bar**
```
Loading BTC
[████░░░░░░] 40%
```

**Implementation (Animated Dots):**
```cpp
String getLoadingText(String label) {
    // Cycle through ., .., ... every 500ms
    unsigned long ms = millis();
    int dots = (ms / 500) % 4;  // 0, 1, 2, 3

    String text = "Loading " + label;
    for (int i = 0; i < dots; i++) {
        text += ".";
    }
    return text;
}
```

---

#### D. Show Last Update Time After Success

Once data loads, show when it was fetched:

```
Bitcoin
$43,250
Updated 2m ago
```

**Implementation:**
```cpp
String getTimeAgo(unsigned long lastUpdate) {
    unsigned long now = millis() / 1000;
    unsigned long elapsed = now - lastUpdate;

    if (elapsed < 60) {
        return String(elapsed) + "s ago";
    } else if (elapsed < 3600) {
        return String(elapsed / 60) + "m ago";
    } else {
        return String(elapsed / 3600) + "h ago";
    }
}
```

---

## Summary of Changes

### 1. Unified Search ✅
- **Crypto:** Autocomplete from CoinGecko (10,000+ coins)
- **Stock:** Autocomplete from Yahoo Finance
- **Weather:** City search with lat/lon auto-fill
- **Consistent UI:** Same search box style for all

### 2. Advanced Custom Module ✅
- **Markup Language:** `type:value:options`
- **Supported Types:**
  - `countdown` - Days until event
  - `note` - Text display
  - `days` - Days since/until
  - `timer` - Countdown timer
  - `progress` - Progress bar
  - `percent` - Percentage
  - `value` - Number with label
- **Visual Preview:** Settings page shows what will display

### 3. Loading States ✅
- **No more zeros:** Show "Updating..." until data arrives
- **Animated indicators:** Optional dots/spinner
- **Module-specific:** "Loading AAPL..." instead of generic
- **Last update time:** Show "Updated 2m ago"

---

## Implementation Priority

### Phase 1: Unified Search (High Priority)
**Effort:** 6-8 hours
**Impact:** Massive UX improvement

**Tasks:**
1. Add search API endpoints to settings server
2. Implement JavaScript autocomplete component
3. Update crypto/stock/weather configuration
4. Test with various search queries

---

### Phase 2: Loading States (Medium Priority)
**Effort:** 2-3 hours
**Impact:** Polished feel, reduces confusion

**Tasks:**
1. Add `hasData` checks to all module formatDisplay()
2. Implement loading text generation
3. Add optional animation (dots/spinner)
4. Test on slow connections

---

### Phase 3: Advanced Custom Module (Low Priority)
**Effort:** 8-12 hours
**Impact:** Power user feature

**Tasks:**
1. Design markup parser
2. Implement countdown/timer logic
3. Add progress bar rendering
4. Create settings page UI with preview
5. Test all markup types

---

## Next Steps

**Which would you like to implement first?**

1. **Unified Search** (biggest user impact, 6-8 hours)
2. **Loading States** (quick win, 2-3 hours)
3. **Advanced Custom** (fun feature, 8-12 hours)

**Or implement in order:** Loading States → Unified Search → Advanced Custom

Let me know and I'll start building! 🚀
