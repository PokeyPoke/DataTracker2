# Settings Module Implementation Plan

## Overview
Add a "Settings" module that displays a QR code + 6-digit security code, allowing users to configure the device via web interface with physical access security.

## Security Specifications (Confirmed)
- ✅ 6-digit random code (100,000-999,999)
- ✅ 5-minute expiration
- ✅ Lock for 1 minute after 3 failed attempts
- ✅ Code expires after first successful use

---

## Feature Components

### 1. Settings Module Display
```
┌──────────────────────────┐
│ SETTINGS      ┌────────┐ │
│               │████████│ │
│ Scan QR       │██  ████│ │
│ Code: 724891  │████  ██│ │
│               │██████ █│ │
│ 192.168.1.123 └────────┘ │
└──────────────────────────┘
```

**Implementation:**
- Create `SettingsModule` class (similar to other modules)
- Display QR code with device's local IP address
- Show 6-digit code prominently
- Include IP address as text fallback

### 2. Code Generation & Management

**When code is generated:**
- User cycles to Settings module (button press)
- Generate new random 6-digit code
- Store: code, generation timestamp, used flag, failed attempts

**Code state tracking:**
```cpp
struct SecurityCode {
    uint32_t code;               // 6-digit number
    unsigned long generatedAt;   // millis() when created
    bool used;                   // true after successful auth
    uint8_t failedAttempts;      // count of failed attempts
    unsigned long lockoutUntil;  // millis() when lockout ends
};
```

**Validation rules:**
- Code valid for 5 minutes (300,000 ms)
- Code invalid if already used
- Max 3 attempts before 1-minute lockout
- Code regenerated when user cycles back to settings

### 3. Always-On Web Server

**Current state:** Web server only runs during initial setup (AP mode)

**Required changes:**
- Start web server after connecting to home WiFi
- Listen on port 80 (home network)
- Serve settings page at `/settings`
- API endpoints for validation and configuration

**Endpoints:**
```
GET  /                    → Settings login page
POST /api/validate        → Validate 6-digit code
GET  /api/config          → Get current configuration (requires auth)
POST /api/config          → Update configuration (requires auth)
POST /api/factory-reset   → Factory reset (requires auth)
GET  /api/modules         → List available modules
```

### 4. Settings Web Page

**Login Screen:**
```html
┌─────────────────────────────────────┐
│  DataTracker Settings               │
├─────────────────────────────────────┤
│  Enter code from device display:    │
│                                     │
│  [_ _ _ _ _ _]  6-digit code       │
│                                     │
│  [Unlock Settings]                  │
│                                     │
│  ⚠ Code expires in 4:23             │
└─────────────────────────────────────┘
```

**After Authentication:**
```html
┌─────────────────────────────────────┐
│  DataTracker Settings               │
├─────────────────────────────────────┤
│  Enabled Modules:                   │
│  ☑ Bitcoin Price                    │
│  ☑ Ethereum Price                   │
│  ☑ Stock Price                      │
│    → Ticker: [AAPL____] [Update]   │
│  ☐ Weather                          │
│    → Location: [______] [Search]   │
│    → Lat/Lon: [_____] [_____]      │
│  ☑ Settings QR (always enabled)     │
│                                     │
│  Display Mode:                      │
│  ⚫ Auto-rotate every [10] seconds  │
│  ○ Manual (button only)             │
│                                     │
│  WiFi Settings:                     │
│  Network: MyNetwork                 │
│  [Change WiFi] [Reconnect]          │
│                                     │
│  Danger Zone:                       │
│  [Factory Reset] [Restart Device]   │
│                                     │
│  [Save All Changes]                 │
└─────────────────────────────────────┘
```

### 5. Session Management

**After successful code validation:**
- Generate session token (random string)
- Store token with expiration (30 minutes)
- Return token to browser (cookie or localStorage)
- All subsequent API calls require valid token

**Session token:**
```cpp
struct Session {
    char token[33];              // 32-char hex string + null
    unsigned long expiresAt;     // millis() when expires
};
```

---

## Implementation Steps

### Phase 1: Infrastructure
- [ ] Add SettingsModule class to module system
- [ ] Keep web server running after WiFi connects
- [ ] Add code generation/validation functions
- [ ] Implement session management
- [ ] Add rate limiting for failed attempts

### Phase 2: Display
- [ ] Settings module display with QR code
- [ ] Show 6-digit code on screen
- [ ] Display device IP address
- [ ] Handle code regeneration on module entry

### Phase 3: Web Interface
- [ ] Settings login page HTML/CSS/JS
- [ ] Code validation API endpoint
- [ ] Settings page HTML (module config)
- [ ] Save configuration API endpoint
- [ ] Real-time validation feedback

### Phase 4: Module Configuration
- [ ] Stock ticker input/validation
- [ ] Weather location search/input
- [ ] Module enable/disable toggles
- [ ] Display mode selection (auto/manual)
- [ ] WiFi reconfiguration

### Phase 5: Testing & Polish
- [ ] Test code expiration (5 min)
- [ ] Test failed attempt lockout
- [ ] Test session expiration
- [ ] Test all module configurations
- [ ] Mobile responsive design

---

## Technical Specifications

### Code Generation
```cpp
uint32_t generateSecurityCode() {
    return random(100000, 999999);  // 6 digits
}
```

### Code Validation Logic
```cpp
bool validateCode(uint32_t enteredCode) {
    // Check if locked out
    if (millis() < securityCode.lockoutUntil) {
        return false;  // Still locked
    }

    // Check expiration (5 minutes)
    if (millis() - securityCode.generatedAt > 300000) {
        return false;  // Expired
    }

    // Check if already used
    if (securityCode.used) {
        return false;  // Already authenticated
    }

    // Validate code
    if (enteredCode == securityCode.code) {
        securityCode.used = true;
        return true;  // Success!
    }

    // Failed attempt
    securityCode.failedAttempts++;

    // Lock after 3 failures
    if (securityCode.failedAttempts >= 3) {
        securityCode.lockoutUntil = millis() + 60000;  // 1 minute
    }

    return false;
}
```

### Session Token Generation
```cpp
String generateSessionToken() {
    char token[33];
    for (int i = 0; i < 32; i++) {
        token[i] = "0123456789abcdef"[random(0, 16)];
    }
    token[32] = '\0';
    return String(token);
}
```

---

## Memory Impact Estimate

**Additional Flash:**
- Settings module code: ~3-4 KB
- Settings HTML page: ~8-10 KB (compressed)
- Session management: ~2 KB
- **Total:** ~13-16 KB

**Additional RAM:**
- Security code struct: ~20 bytes
- Session struct: ~50 bytes
- Web server (always on): ~15-20 KB
- **Total:** ~15-20 KB

**Current usage:** RAM 13.8%, Flash 55.8%
**After changes:** RAM ~20%, Flash ~56.5%
**Still safe!** ✅

---

## User Experience Flow

1. **User wants to change settings:**
   - Press button to cycle through modules
   - Cycle to "Settings" module
   - See QR code + 6-digit code on screen

2. **User scans QR code:**
   - Opens browser to http://192.168.1.123
   - Sees login page requesting code

3. **User enters code:**
   - Types 6-digit code from screen
   - Click "Unlock Settings"
   - Code validated, session created

4. **User configures settings:**
   - Enable/disable modules
   - Configure stock ticker, weather location
   - Set rotation mode
   - Save changes

5. **Device updates:**
   - Configuration saved to LittleFS
   - Modules re-initialized
   - Display updates with new settings

---

## Security Benefits

✅ **Physical access required** - Must press button to see code
✅ **Time-limited** - Code expires in 5 minutes
✅ **One-time use** - Code invalid after successful auth
✅ **Rate limiting** - Locked out after 3 failures
✅ **Session-based** - Token expires after 30 minutes
✅ **Local network only** - Not exposed to internet
✅ **No default passwords** - Random code each time

---

## Next Steps

Ready to implement! Should I:
1. Start with Phase 1 (infrastructure)?
2. Create a new feature branch?
3. Something else?

Let me know and I'll begin building this system!
