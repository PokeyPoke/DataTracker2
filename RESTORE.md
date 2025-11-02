# Restoring Stable Version v1.0

This document explains how to restore the working v1.0-stable version if future modifications break the firmware.

## Repository Information

**GitHub Repository**: https://github.com/PokeyPoke/DataTracker
**Stable Tag**: `v1.0-stable`
**Release**: https://github.com/PokeyPoke/DataTracker/releases/tag/v1.0-stable

## Method 1: Restore from Git Tag (Recommended)

If you're in the DataTracker directory and want to restore the code:

```bash
# Save any current work (optional)
git stash

# Checkout the stable version
git checkout v1.0-stable

# Create a new branch from stable (optional, to continue working)
git checkout -b my-new-feature

# Or stay on the tag to build the exact stable version
/home/happyllama/.local/pipx/venvs/platformio/bin/pio run

# Flash to device
cp .pio/build/esp32-c3-devkitm-1/firmware.bin docs/firmware/firmware.bin
```

## Method 2: Download Pre-compiled Firmware

Download the pre-compiled firmware from the GitHub release:

1. Go to: https://github.com/PokeyPoke/DataTracker/releases/tag/v1.0-stable
2. Download `firmware.bin`
3. Use web flasher to install:
   - Open: https://pokeypoke.github.io/DataTracker/flash.html
   - Or local: `http://localhost:8080/flash.html`
   - Use **Quick Flash** to update firmware only
   - Use **Factory Reset & Install** for full clean install

## Method 3: Clone Fresh from GitHub

Start completely fresh:

```bash
# Move current directory (backup)
cd ~
mv DataTracker DataTracker.backup

# Clone from GitHub
git clone https://github.com/PokeyPoke/DataTracker.git
cd DataTracker

# Checkout stable version
git checkout v1.0-stable

# Build
/home/happyllama/.local/pipx/venvs/platformio/bin/pio run
```

## What's in v1.0-stable

### Memory Footprint
- **RAM**: 13.1% (42,836 bytes)
- **Flash**: 53.7% (1.0MB)

### Key Features
✅ Display enabled and working
✅ WiFi configuration portal
✅ Synchronous WebServer (no heap corruption)
✅ All modules functional
✅ Optimized WiFi scanning

### Architecture
- **No AsyncWebServer** - Uses synchronous WebServer
- **No DNSServer** - Direct IP access only (192.168.4.1)
- **StaticJsonDocument<1024>** - Stack allocation, no heap fragmentation
- **Ultra-compact HTML** - <1KB configuration interface
- **Display enabled** - OLED working without issues

### Files Modified Since Original Async Version
1. `platformio.ini` - Removed async dependencies
2. `include/network.h` - Changed to WebServer
3. `src/network.cpp` - Complete rewrite for synchronous operation
4. `include/config.h` - Reduced JSON buffer to 1024 bytes
5. `src/config.cpp` - Reduced JSON buffer to 1024 bytes
6. `src/main.cpp` - Re-enabled display after stabilization

## Verification

After restoring, verify you have the correct version:

```bash
# Check git tag
git describe --tags
# Should output: v1.0-stable

# Check memory usage after build
/home/happyllama/.local/pipx/venvs/platformio/bin/pio run | grep "RAM:"
# Should show: RAM:   [=         ]  13.1% (used 42836 bytes from 327680 bytes)

# Check for synchronous WebServer (not async)
grep "WebServer.h" include/network.h
# Should find: #include <WebServer.h>

grep "AsyncWebServer" include/network.h
# Should find nothing (removed)
```

## If Things Go Wrong

### Device won't boot / heap corruption
This means you're not on v1.0-stable. The async version had heap corruption issues.

**Solution**:
- Checkout `v1.0-stable` tag
- Rebuild and flash
- Use **Factory Reset & Install** button in web flasher

### Display is blank
This is normal for versions before v1.0-stable final. The display was disabled during development.

**Solution**:
- Make sure you're on the latest commit of v1.0-stable
- Check `src/main.cpp` line 57 should be: `display.init();`
- Not: `// display.init();`

### Can't find networks in config portal
**Solution**:
- Wait 10 seconds after connecting to AP for first scan
- Refresh the webpage
- Check serial console for "Scan complete" message

## Support

For issues with the stable version:
- Check serial console output (115200 baud)
- Review TROUBLESHOOTING.md
- Open issue on GitHub: https://github.com/PokeyPoke/DataTracker/issues

## Making New Features

To work on new features while preserving the stable version:

```bash
# Start from stable
git checkout v1.0-stable

# Create feature branch
git checkout -b feature/my-new-feature

# Make changes...
# Test thoroughly...

# Commit
git add .
git commit -m "Add new feature"

# Push to GitHub
git push origin feature/my-new-feature

# If it breaks, restore stable:
git checkout v1.0-stable
```

## Version History

- **v1.0-stable** (Nov 2, 2025) - First stable release
  - Display enabled
  - Heap corruption fixed
  - All features tested and working
  - Memory optimized
  - WiFi scanning improved

---

**Remember**: Always test on the actual hardware before considering a new version stable!
