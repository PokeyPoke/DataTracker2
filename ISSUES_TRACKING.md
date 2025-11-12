# DataTracker Issues Tracking

**Base Version**: v2.6.4-STABLE (Locked)
**Status**: Issues identified but not yet being worked on
**Approach**: Address one issue at a time after confirming stable base is working

---

## Overview

This document tracks known issues identified in the DataTracker v2.6.4-STABLE codebase. These are not bugs blocking functionality—all features work well—but rather optimization opportunities and potential improvements that should be addressed methodically.

**Policy**: Changes will be made incrementally, with each change tested before moving to the next issue.

---

## Issues List

### 🔴 Issue #1: Random Number Generation Seeding

**Status**: ⚠️ Low Priority
**Category**: Code Quality / Security
**Severity**: Low (works reliably)
**File**: `src/security.cpp:22`
**Type**: Enhancement

#### Description

Security code generation uses Arduino's `random()` function without explicit seeding. While the current implementation works reliably, best practice would be to explicitly seed the random number generator to ensure higher entropy.

#### Current Code

```cpp
// In SecurityManager::generateNewCode()
uint32_t code = random(100000, 999999);  // 6-digit code
```

#### Issue Details

- ESP32 seeds random() from hardware entropy on boot
- Current entropy is adequate for 6-digit codes (100,000 - 999,999 combinations)
- However, explicit seeding would follow cryptographic best practices
- No known security vulnerability with current approach

#### Proposed Solution

Add explicit randomSeed call in SecurityManager constructor:

```cpp
SecurityManager::SecurityManager() {
    randomSeed(millis() + random(32768));  // Additional entropy
    // ... rest of init
}
```

Or use hardware entropy:

```cpp
randomSeed(esp_random());  // Use ESP32 hardware RNG directly
```

#### Testing Required

- Verify codes still generate correctly
- Verify codes are unique across multiple generations
- Verify no performance impact

#### Notes

- Current implementation has been working since v2.5
- Not blocking any functionality
- This is a "nice to have" improvement

---

### 🟡 Issue #2: HTTPS Certificate Validation

**Status**: ⚠️ Medium Priority
**Category**: Security
**Severity**: Low (local network only)
**File**: `src/network.cpp` (HTTPClient setup)
**Type**: Enhancement

#### Description

HTTPS connections to external APIs do not verify SSL/TLS certificates. For an embedded device on a local network, this is acceptable, but proper certificate validation should be considered for production.

#### Current Status

- All external API calls use HTTPS
- SSL connections established but certificates not verified
- No sensitive data transmitted (only public prices)
- Device isolated to local network

#### APIs Affected

1. **Bitcoin (CoinGecko)** - Public data only
2. **Ethereum (CoinGecko)** - Public data only
3. **Stock Prices (Yahoo Finance)** - Public data only
4. **Weather (Open-Meteo)** - Public data only

#### Proposed Solution

Options:

**Option A**: Enable certificate validation (recommended long-term)
```cpp
client.setCACert(rootCA_cert);  // Load CA certificate bundle
```

**Option B**: Keep current approach (acceptable for now)
- Document that this is by design for embedded constraints
- Note that devices operate on local/trusted networks only

#### Testing Required

- Verify certificate validation works with all APIs
- Check memory impact (CA bundle is large)
- Verify no false positives from certificate changes

#### Notes

- Low risk because device operates on local networks
- APIs are read-only (no sensitive operations)
- Can defer this issue if memory becomes constrained

---

### 🟡 Issue #3: WiFi Reconnection Strategy

**Status**: ⚠️ Low Priority
**Category**: Reliability
**Severity**: Low (works but passive)
**File**: `src/network.cpp` WiFi connection logic
**Type**: Enhancement

#### Description

WiFi reconnection is passive—the device only checks connection on API fetch. A more aggressive reconnection strategy could improve reliability on unreliable networks.

#### Current Behavior

```
WiFi connected ──→ [Idle] ──→ API Fetch → Connection Lost
                                              ↓
                                    [Check & Reconnect]
```

#### Proposed Improvement

```
WiFi connected ──→ [Periodic Check] ──→ API Fetch → Connection Lost
                        ↓                              ↓
                   [Reconnect?]            [Immediate Reconnect]
```

#### Proposed Solution

Add periodic WiFi status check in main loop:

```cpp
#define WIFI_CHECK_INTERVAL 30000  // Check every 30 seconds

if (now - lastWiFiCheck > WIFI_CHECK_INTERVAL) {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi lost, reconnecting...");
        network.reconnectWiFi();
    }
    lastWiFiCheck = now;
}
```

#### Testing Required

- Test with WiFi router turned off
- Test with intermittent WiFi loss
- Verify no excessive reconnect attempts
- Check power consumption impact

#### Notes

- Current approach works well in stable networks
- Nice to have for reliability
- Low risk change

---

### 🟢 Issue #4: Settings Display Flicker

**Status**: ⚠️ Very Low Priority (Cosmetic)
**Category**: User Experience
**Severity**: Cosmetic
**File**: `src/display.cpp` showSettings()
**Type**: Cosmetic

#### Description

The settings module display updates frequently, which can cause minor flicker when QR code is being rendered.

#### Current Implementation

```cpp
void DisplayManager::showSettings(uint32_t securityCode, const char* deviceIP,
                                   unsigned long timeRemaining) {
    u8g2.clearBuffer();
    // ... render code and QR ...
    u8g2.sendBuffer();  // Updates every display cycle
}
```

#### Proposed Solution

Add display caching to avoid unnecessary redraws:

```cpp
// Cache last rendered state
static uint32_t lastCode = 0;
static unsigned long lastRenderTime = 0;

// Only redraw if code changed or time > 1 second
if (code != lastCode || (now - lastRenderTime) > 1000) {
    u8g2.clearBuffer();
    // ... render ...
    u8g2.sendBuffer();
    lastCode = code;
    lastRenderTime = now;
}
```

#### Testing Required

- Verify QR code still scans properly
- Verify no visual artifacts
- Verify code display updates smoothly

#### Notes

- Purely cosmetic—not affecting functionality
- Lowest priority improvement
- No user complaints about current behavior

---

### 🟢 Issue #5: Serial Console Commands Expansion

**Status**: ⚠️ Low Priority
**Category**: Developer Tools
**Severity**: None (debug feature)
**File**: `src/main.cpp` handleSerialCommand()
**Type**: Feature

#### Description

Current serial console has basic commands. Could add more debug commands for troubleshooting.

#### Current Commands

- `help` - Show help
- `wifi` - Show WiFi status
- `config` - Show configuration
- `modules` - List modules
- `fetch` - Force data fetch
- `reset` - Factory reset

#### Proposed Commands

```
info      - Show detailed device info
api       - Test API connectivity
time      - Show current time/uptime
memory    - Show memory usage
cache     - Show cached data
codes     - Show security codes (local only)
signal    - Show WiFi signal strength
logs      - Show last N events
```

#### Testing Required

- Verify all new commands work
- Verify output is useful
- Verify no security implications

#### Notes

- Debug feature only (serial console access)
- Nice to have for troubleshooting
- Low risk change

---

### 🟢 Issue #6: API Error Handling Improvements

**Status**: ⚠️ Low Priority
**Category**: Reliability
**Severity**: Low (exponential backoff works)
**File**: `src/scheduler.cpp` backoff logic
**Type**: Enhancement

#### Description

API error handling uses exponential backoff (60s → 3600s max). Could be smarter about handling different types of errors.

#### Current Strategy

```
Error → Wait 60s → Error → Wait 120s → Error → Wait 240s → ... → Max 3600s
```

#### Proposed Improvement

```
Timeout Error   → Wait 60s (transient)
Rate Limit (429) → Wait 300s (back off more)
Invalid Data (4xx) → Wait 3600s (permanent issue)
Connection Failed (5xx) → Wait 120s (server issue)
```

#### Proposed Solution

Categorize errors in each module:

```cpp
enum ErrorType {
    TRANSIENT,      // Timeout, temporary
    RATE_LIMITED,   // API rate limit
    INVALID_INPUT,  // Bad request
    SERVER_ERROR    // 5xx errors
};

// Different backoff for each type
switch(errorType) {
    case TRANSIENT: backoffSeconds = 60; break;
    case RATE_LIMITED: backoffSeconds = 300; break;
    case INVALID_INPUT: backoffSeconds = 3600; break;
    case SERVER_ERROR: backoffSeconds = 120; break;
}
```

#### Testing Required

- Test with various API errors
- Test with different timeout durations
- Verify backoff logic still works

#### Notes

- Current implementation works fine
- Enhancement for better error recovery
- Low risk change

---

### 🟢 Issue #7: Memory Usage Monitoring

**Status**: ✅ No Action Needed
**Category**: Future Planning
**Severity**: None (good headroom)
**File**: Config system, all modules
**Type**: Monitoring

#### Description

Memory usage should be monitored as new features are added to ensure we don't exceed ESP32 limits.

#### Current Status

```
RAM: 46 KB / 327 KB used (14.1%) ← Good headroom
Flash: 1.1 MB / 1.96 MB used (58.4%) ← Good headroom
```

#### Headroom Available

- RAM: 280 KB available (can add ~2.7× current usage)
- Flash: 800 KB available (can add ~0.7× current usage)

#### Proposal

Add memory usage logging command:

```cpp
// Serial command: "memory"
Serial.print("RAM: ");
Serial.print(freeHeap());
Serial.println(" bytes free");
```

#### Notes

- No action needed now
- Monitor but don't optimize prematurely
- Room for significant feature additions

---

### ✅ Issue #8: Documentation Completeness

**Status**: ✅ RESOLVED
**Category**: Documentation
**Type**: Complete

#### Resolved Items

- [x] STABLE_BASE.md - Locked version documentation
- [x] FLASH_GUIDE.md - Detailed flashing instructions
- [x] WEBFLASHER_QUICK_START.txt - Quick reference
- [x] FLASHER_STATUS.txt - Current status
- [x] ISSUES_TRACKING.md - This file
- [x] README.md - Project documentation
- [x] Build guide
- [x] Troubleshooting guide

#### Status

All documentation is now comprehensive and up to date.

---

## Issue Priority Matrix

| Issue | Priority | Effort | Risk | Impact |
|-------|----------|--------|------|--------|
| Random Seeding | Low | 30 min | Very Low | Low |
| HTTPS Certs | Medium | 2-3 hrs | Medium | Low |
| WiFi Reconnection | Low | 1-2 hrs | Low | Medium |
| Settings Flicker | Very Low | 30 min | Very Low | None |
| Serial Commands | Low | 1 hr | Very Low | None |
| API Error Handling | Low | 1-2 hrs | Low | Medium |
| Memory Monitoring | None | 20 min | None | None |

---

## Recommended Next Steps

### After v2.6.4-STABLE is Confirmed Stable

**Week 1**: Random Seeding (Quick, Safe)
```bash
git checkout stable-v2.6.4
git checkout -b improve/random-seeding
# Make changes, test, commit
git tag v2.6.5-seeding
```

**Week 2**: Serial Commands (Nice to Have)
```bash
git checkout stable-v2.6.4
git checkout -b feature/serial-commands
# Add new debug commands, test
git tag v2.6.5-debug
```

**Week 3**: WiFi Reconnection (Reliability)
```bash
git checkout stable-v2.6.4
git checkout -b improve/wifi-reconnection
# Add periodic WiFi check, test
git tag v2.6.6-reliability
```

**Later**: HTTPS Certs (Security)
```bash
git checkout stable-v2.6.4
git checkout -b improve/https-validation
# Add certificate validation, test
```

---

## Testing Checklist Template

For each issue being addressed:

```
Before Changes:
  [ ] Build succeeds
  [ ] Flash to device
  [ ] All modules fetch data
  [ ] Settings code generation works
  [ ] Web UI accessible

After Changes:
  [ ] Build succeeds
  [ ] Flash to device
  [ ] All modules fetch data (no regression)
  [ ] New feature works as designed
  [ ] Settings code generation still works
  [ ] Web UI still accessible
  [ ] No new compiler warnings
  [ ] Memory usage similar

Comparison:
  [ ] git diff shows expected changes
  [ ] No accidental modifications
  [ ] Matches proposed solution
```

---

## How to Report New Issues

1. **Test against v2.6.4-STABLE**
   ```bash
   git checkout stable-v2.6.4
   ```

2. **Reproduce the issue**

3. **Document in this file**:
   - Add new issue section
   - Note affected files
   - Propose solution
   - Estimate effort

4. **Create feature branch when ready to fix**
   ```bash
   git checkout stable-v2.6.4
   git checkout -b fix/issue-name
   ```

---

## Status Summary

| Category | Count | Status |
|----------|-------|--------|
| Total Issues | 8 | ✅ Documented |
| Blocking Issues | 0 | ✅ None |
| High Priority | 0 | ✅ None |
| Medium Priority | 1 | ⚠️ HTTPS Certs |
| Low Priority | 5 | ⚠️ Improvements |
| Very Low Priority | 1 | 🟢 Cosmetic |
| Resolved | 1 | ✅ Documentation |

---

**Last Updated**: November 12, 2024
**Base Version**: v2.6.4-STABLE
**Status**: Ready for incremental improvements

