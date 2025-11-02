# Animal Names Implementation

## Concept
Each device gets a unique, memorable animal name based on its MAC address.
Makes multi-device setup easy and fun!

## Animal Name List

### Selection Criteria
- Easy to spell
- Recognizable across cultures
- Not too long (< 10 chars preferred)
- Fun and friendly
- Diverse (land, sea, air)

### The List (64 animals for 6-bit hash)

```cpp
const char* ANIMAL_NAMES[] = {
    // Popular & Cute (0-15)
    "Panda", "Tiger", "Lion", "Bear",
    "Fox", "Wolf", "Cat", "Dog",
    "Bunny", "Otter", "Seal", "Koala",
    "Sloth", "Lemur", "Meerkat", "Badger",

    // Birds (16-23)
    "Eagle", "Owl", "Hawk", "Falcon",
    "Parrot", "Penguin", "Toucan", "Peacock",

    // Sea Creatures (24-31)
    "Dolphin", "Whale", "Shark", "Turtle",
    "Octopus", "Crab", "Lobster", "Starfish",

    // Wild & Exotic (32-39)
    "Giraffe", "Zebra", "Rhino", "Hippo",
    "Elephant", "Monkey", "Gorilla", "Lemur",

    // Small & Cute (40-47)
    "Hamster", "Squirrel", "Chipmunk", "Hedgehog",
    "Mouse", "Rabbit", "Ferret", "Gecko",

    // Mythical/Fun (48-55)
    "Dragon", "Phoenix", "Unicorn", "Griffin",
    "Sphinx", "Kraken", "Hydra", "Basilisk",

    // More Wild (56-63)
    "Cheetah", "Leopard", "Jaguar", "Cougar",
    "Lynx", "Panther", "Puma", "Ocelot"
};
```

## Name Selection Algorithm

### Option 1: Simple Hash (Recommended)
```cpp
String getAnimalName() {
    // Get MAC address
    uint8_t mac[6];
    WiFi.macAddress(mac);

    // Use last byte as index (0-255)
    // Modulo 64 to get index in our array
    uint8_t index = mac[5] % 64;

    return String(ANIMAL_NAMES[index]);
}

String getSSID() {
    return "DataTracker-" + getAnimalName();
    // Examples:
    // "DataTracker-Panda"
    // "DataTracker-Giraffe"
    // "DataTracker-Dolphin"
}
```

**Pros:**
- Deterministic (same device always gets same name)
- Simple
- Even distribution

**Cons:**
- Small collision chance with 64 names

### Option 2: Compound Hash (More Unique)
```cpp
String getAnimalName() {
    uint8_t mac[6];
    WiFi.macAddress(mac);

    // Use last 2 bytes for more uniqueness
    uint8_t index1 = mac[4] % 64;
    uint8_t index2 = mac[5] % 64;

    if (index1 == index2) {
        // Avoid same name twice
        index2 = (index2 + 1) % 64;
    }

    return String(ANIMAL_NAMES[index1]);
    // Could also do compound: "PandaTiger" but gets long
}
```

### Option 3: Number Suffix for True Uniqueness
```cpp
String getSSID() {
    uint8_t mac[6];
    WiFi.macAddress(mac);

    String animal = ANIMAL_NAMES[mac[5] % 64];

    // Add 2-digit number from MAC for uniqueness
    char suffix[3];
    snprintf(suffix, sizeof(suffix), "%02X", mac[4]);

    return "DataTracker-" + animal + suffix;
    // Examples:
    // "DataTracker-PandaA3"
    // "DataTracker-GiraffeF2"
}
```

**Recommended:** Option 3 - Animal + 2-char suffix
- Most unique
- Still memorable
- Clear which device when multiple present

## Display Layout (Final Design)

### State 1: Waiting for Connection
```
┌──────────────────────────────┐
│                              │
│      [QR Code 50x50]         │
│                              │
├──────────────────────────────┤
│ DataTracker-PandaA3          │
│ Password: DTA3F2E1!          │
│                              │
│ Scan QR or connect manually  │
└──────────────────────────────┘
```

### Layout Code
```cpp
void showWiFiQR() {
    u8g2.clearBuffer();

    // Generate credentials
    String ssid = getSSID();  // "DataTracker-PandaA3"
    String password = generatePassword();  // "DTA3F2E1!"

    // QR code data
    String qrData = "WIFI:T:WPA;S:" + ssid + ";P:" + password + ";;";

    // Draw QR code at top (centered)
    drawQRCode(qrData, 39, 2, 2);  // 50x50 pixels, centered at x=39

    // Draw horizontal line separator
    u8g2.drawHLine(0, 54, 128);

    // Show SSID (small font to fit)
    u8g2.setFont(u8g2_font_5x7_tr);
    u8g2.drawStr(2, 60, ssid.c_str());

    // Show password on next line
    String pwdText = "Pass: " + password;
    u8g2.drawStr(2, 68, pwdText.c_str());

    u8g2.sendBuffer();
}
```

### Alternative: Scrolling Display (if text too long)
```cpp
void showWiFiQR() {
    u8g2.clearBuffer();

    // ... QR code ...

    // Alternate between SSID and Password every 2 seconds
    if ((millis() / 2000) % 2 == 0) {
        u8g2.setFont(u8g2_font_6x10_tr);
        u8g2.drawStr(2, 60, ssid.c_str());
    } else {
        u8g2.setFont(u8g2_font_6x10_tr);
        String pwdText = "Pass: " + password;
        u8g2.drawStr(2, 60, pwdText.c_str());
    }

    u8g2.sendBuffer();
}
```

## Password Display Options

### Option A: Full Password
```
Password: DTA3F2E1!
```
**Pro:** Complete info
**Con:** Takes space

### Option B: Abbreviated
```
Pass: DTA3F2E1!
```
**Pro:** Shorter label
**Con:** Still long

### Option C: Two Lines (Recommended)
```
Network: DataTracker-PandaA3
Password: DTA3F2E1!
```
**Pro:** Very clear
**Con:** Takes 2 lines

### Option D: Compact Single Line
```
PandaA3 | Pass: DTA3F2E1!
```
**Pro:** Fits in one line
**Con:** Might be cramped

**Recommended:** Option C if fits, else Option B

## Memory Considerations

```cpp
// Animal names array
const char* ANIMAL_NAMES[64] = {...};
// Average name length: 7 chars
// Total: 64 * 7 = 448 bytes

// Plus pointers: 64 * 4 = 256 bytes
// Total: ~700 bytes
```

Minimal impact!

## SSID Length Validation

```
WiFi SSID max length: 32 characters

Current format: "DataTracker-PandaA3"
- "DataTracker-" = 13 chars
- Longest animal = 9 chars (e.g., "Chipmunk")
- Suffix = 2 chars
- Total max = 13 + 9 + 2 = 24 chars ✓

Well under limit!
```

## Complete Implementation

```cpp
// In network.h
private:
    String animalName;
    String devicePassword;

// In network.cpp
const char* ANIMAL_NAMES[] = {
    "Panda", "Tiger", "Lion", "Bear",
    "Fox", "Wolf", "Cat", "Dog",
    "Bunny", "Otter", "Seal", "Koala",
    "Sloth", "Lemur", "Meerkat", "Badger",
    "Eagle", "Owl", "Hawk", "Falcon",
    "Parrot", "Penguin", "Toucan", "Peacock",
    "Dolphin", "Whale", "Shark", "Turtle",
    "Octopus", "Crab", "Lobster", "Starfish",
    "Giraffe", "Zebra", "Rhino", "Hippo",
    "Elephant", "Monkey", "Gorilla", "Chimpanzee",
    "Hamster", "Squirrel", "Chipmunk", "Hedgehog",
    "Mouse", "Rabbit", "Ferret", "Gecko",
    "Dragon", "Phoenix", "Unicorn", "Griffin",
    "Sphinx", "Kraken", "Hydra", "Basilisk",
    "Cheetah", "Leopard", "Jaguar", "Cougar",
    "Lynx", "Panther", "Puma", "Ocelot"
};

String NetworkManager::getAnimalName() {
    uint8_t mac[6];
    WiFi.macAddress(mac);
    uint8_t index = mac[5] % 64;
    return String(ANIMAL_NAMES[index]);
}

String NetworkManager::generateSSID() {
    uint8_t mac[6];
    WiFi.macAddress(mac);

    // Get animal name
    String animal = getAnimalName();

    // Get 2-char suffix from MAC
    char suffix[3];
    snprintf(suffix, sizeof(suffix), "%02X", mac[4]);

    // Build SSID
    apName = "DataTracker-" + animal + String(suffix);
    return apName;
}

String NetworkManager::generatePassword() {
    uint8_t mac[6];
    WiFi.macAddress(mac);

    // Format: DT + last 3 MAC bytes + !
    char macStr[7];
    snprintf(macStr, sizeof(macStr), "%02X%02X%02X",
             mac[3], mac[4], mac[5]);

    devicePassword = "DT" + String(macStr) + "!";
    return devicePassword;
}

void NetworkManager::startConfigAP() {
    // Generate unique name and password
    generateSSID();
    generatePassword();

    Serial.print("Starting AP: ");
    Serial.println(apName);
    Serial.print("Password: ");
    Serial.println(devicePassword);

    // Start AP with password
    WiFi.mode(WIFI_AP);
    WiFi.softAP(apName.c_str(), devicePassword.c_str());

    // ... rest of setup ...
}
```

## User Experience Examples

### Scenario 1: Single Device
**User sees:**
```
DataTracker-PandaA3
Password: DTA3F2E1!
```
**User thinks:** "Oh, it's the Panda one. Let me scan this QR code."

### Scenario 2: Multiple Devices
**User sees on device 1:**
```
DataTracker-TigerB7
```

**User sees on device 2:**
```
DataTracker-DolphinC2
```

**User thinks:** "Okay, I need to set up the Tiger and the Dolphin. Let me do Tiger first."

### Scenario 3: QR Code Fails
**User sees:**
```
DataTracker-GiraffeF3
Password: DTF3A2E1!
```
**User thinks:** "QR code won't scan. No problem, I see the network name and password right there. I'll type it in."

## Serial Console Output

```cpp
void startConfigAP() {
    generateSSID();
    generatePassword();

    Serial.println("\n╔════════════════════════════════╗");
    Serial.println("║   CONFIG MODE STARTED          ║");
    Serial.println("╚════════════════════════════════╝");
    Serial.println();
    Serial.print("📡 Network: ");
    Serial.println(apName);
    Serial.print("🔐 Password: ");
    Serial.println(devicePassword);
    Serial.println();
    Serial.println("📱 Scan QR code on display");
    Serial.println("   or connect manually");
    Serial.println();
    Serial.println("🌐 Once connected, open:");
    Serial.println("   http://datatracker.local");
    Serial.println("   http://192.168.4.1");
    Serial.println();
}
```

## Marketing Copy / Documentation

### Quick Start Guide
```markdown
1. Power on your DataTracker
2. Look at the display - you'll see an animal name like "Panda" or "Dolphin"
3. Scan the QR code with your phone
4. Your phone will connect automatically
5. Scan the second QR code to open setup
6. Select your WiFi and module
7. Done!

**Tip:** If the QR code doesn't work, you can manually connect:
- Network name and password are shown on the display
- After connecting, go to datatracker.local
```

### Multiple Device Setup
```markdown
Setting up multiple DataTrackers? Easy!

Each device gets its own animal name:
- "DataTracker-Panda"
- "DataTracker-Tiger"
- "DataTracker-Dolphin"

Just look at the display to know which one you're configuring!
```

## Testing Checklist

- [ ] Animal name displays correctly
- [ ] Different devices get different names (test with 2+ devices)
- [ ] Same device always gets same name (reboot test)
- [ ] SSID fits on display (test longest name)
- [ ] Password fits on display
- [ ] Both visible at same time as QR code
- [ ] QR code still scannable
- [ ] Manual connection works using displayed credentials
- [ ] Serial console shows name and password
- [ ] Fun factor (people smile when they see animal names!)

## Future Enhancements

### Customization via Config
```cpp
// Allow users to name their device
config["device"]["customName"] = "John's Tracker";

// Display as:
// "John's Tracker (Panda)"
```

### Emoji Support (if display allows)
```cpp
String getAnimalEmoji(String animal) {
    if (animal == "Panda") return "🐼";
    if (animal == "Tiger") return "🐯";
    // ...
}

// Display: "🐼 DataTracker-Panda"
```

### Regional Animal Lists
```cpp
// Western animals
const char* ANIMALS_WESTERN[] = {...};

// Asian animals
const char* ANIMALS_ASIAN[] = {"Panda", "RedPanda", ...};

// African animals
const char* ANIMALS_AFRICAN[] = {"Lion", "Elephant", ...};
```

## Conclusion

Adding animal names:
- ✅ Solves multi-device confusion
- ✅ Adds personality and fun
- ✅ Memorable (easier than "A3F2E1")
- ✅ Professional yet friendly
- ✅ Minimal code/memory impact
- ✅ Great for branding/marketing

This small change significantly improves the user experience!
