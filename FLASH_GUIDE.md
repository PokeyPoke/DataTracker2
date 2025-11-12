# Local Web Flasher Guide - DataTracker Firmware

This guide will walk you through flashing the DataTracker firmware to your ESP32-C3 device using a local web server.

## Quick Start (3 steps)

### Step 1: Start the Web Flasher Server

Open a terminal in the DataTracker2 directory and run:

```bash
python3 webflasher.py
```

You should see:
```
╔══════════════════════════════════════════════════════════════╗
║          DataTracker Web Flasher - Local Server              ║
╚══════════════════════════════════════════════════════════════╝

📁 Serving from: /path/to/DataTracker2/docs
🚀 Open your browser and go to:
   http://localhost:8000
```

The server is now running on your computer.

### Step 2: Open the Web Flasher Interface

1. **Open your browser** (Chrome or Edge - must be v89+)
2. **Go to**: http://localhost:8000
3. You should see the DataTracker flasher page with two buttons:
   - **Install Firmware** (green button) - Flash firmware while keeping existing config
   - **Factory Reset & Install** (red button) - Erase everything and start fresh

### Step 3: Flash Your Device

#### Before Flashing:
1. **Connect your ESP32-C3** to your computer via USB-C cable
   - ⚠️ Must be a data cable, not just a charging cable
2. **Make sure the device is powered on**
3. **Use a supported browser**: Chrome 89+ or Edge 89+

#### Flash the Firmware:

**Option A: Install Firmware (Keep Existing Config)**
```
1. Click the green "Install Firmware" button
2. Click "Install" in the pop-up dialog
3. Select your ESP32-C3 device from the list
4. Click "Connect"
5. Wait for the progress to complete (1-2 minutes)
6. You'll see "Done!" when finished
```

**Option B: Factory Reset & Install (Erase Everything)**
```
1. Click the red "Factory Reset & Install" button
2. Click "Install" in the pop-up dialog
3. Select your ESP32-C3 device from the list
4. Click "Connect"
5. Wait for the progress to complete (1-2 minutes)
6. You'll see "Done!" when finished
```

#### After Flashing:
```
1. Disconnect the USB cable
2. Wait 5 seconds
3. Reconnect the USB cable to restart the device
4. Device will boot and show the splash screen on the OLED display
5. Device will create a WiFi network named: DataTracker-XXXX
6. Continue with the WiFi setup instructions (see below)
```

## WiFi Setup After Flashing

Once the firmware is flashed, you need to configure WiFi:

### Initial Setup Steps:

1. **Look for the WiFi network**: `DataTracker-XXXX` (where XXXX is random)
2. **Connect to it** from your phone or computer
3. **Open a browser** and go to: `192.168.4.1`
4. **You'll see the setup page** with:
   - WiFi network selection
   - Password entry
   - Module selection (Bitcoin, Ethereum, Stock, Weather, Custom)
   - Stock ticker or weather location configuration
5. **Configure your settings** and click "Complete Setup"
6. **Device will restart** and connect to your WiFi

### After WiFi Configuration:

Once connected to your WiFi network, you can access the settings at:
- **http://dt.local** (mDNS - recommended)
- **http://192.168.x.x** (if you know the IP address)

On the settings page:
- Scan the QR code or enter the 6-digit security code from the device display
- Configure cryptocurrency, stock, or weather settings
- Select which module to display

## Troubleshooting

### Port Not Showing Up
```
❌ Issue: Device not appearing in port selection
✅ Solutions:
   1. Try a different USB cable (must be data cable, not charging only)
   2. Try a different USB port on your computer
   3. Make sure device is powered on
   4. Hold the BOOT button while connecting USB
```

### Flash Failed / Connection Error
```
❌ Issue: Flash failed or connection dropped
✅ Solutions:
   1. Close any other programs using the serial port
      (Arduino IDE, PlatformIO, serial monitor, etc.)
   2. Try the "Factory Reset & Install" button instead
   3. Try holding BOOT button during the flash
   4. Restart your browser and try again
   5. Try a different browser (Chrome vs Edge)
```

### Browser Not Supported
```
❌ Issue: "Your browser doesn't support Web Serial API"
✅ Solutions:
   1. Use Chrome 89+ or Edge 89+ (others don't support Web Serial)
   2. Check your browser version:
      - Chrome: Menu → About Google Chrome (checks for updates)
      - Edge: Menu → Help and feedback → About Microsoft Edge
   3. Safari and Firefox don't support Web Serial API yet
```

### Device Won't Create WiFi Network After Flash
```
❌ Issue: Device booted but no WiFi network visible
✅ Solutions:
   1. Wait 30 seconds after flashing for device to boot
   2. Disconnect USB and reconnect to restart device
   3. Try the "Factory Reset & Install" to clear old config
   4. Check OLED display - should show "SETUP MODE"
   5. If display is blank, try holding BOOT button and power cycling
```

### Settings Code Shows 000000
```
❌ Issue: Security code displays as 000000 on device
✅ Solutions:
   1. This is now FIXED in v2.6.4 - code should display correctly
   2. If still occurs: Click "Factory Reset & Install" and reflash
   3. Verify WiFi is connected (check status bar on display)
```

### Can't Connect to http://dt.local
```
❌ Issue: Can't reach http://dt.local after WiFi setup
✅ Solutions:
   1. Device must be connected to WiFi (check display status bar)
   2. Try using IP address instead:
      - Check your router's connected devices list
      - Or look at OLED display when in settings module
   3. Make sure you're on the same WiFi network as the device
   4. Try restarting the device (power cycle)
```

## Server Alternatives

If you prefer not to use the Python script, here are alternatives:

### Option 1: Python Built-in (Python 3.7+)
```bash
# Navigate to the docs directory
cd DataTracker2/docs

# Start HTTP server
python3 -m http.server 8000

# Then open: http://localhost:8000
```

### Option 2: Node.js / npm
```bash
# Install http-server globally
npm install -g http-server

# Navigate to docs directory and start
cd DataTracker2/docs
http-server -p 8000

# Then open: http://localhost:8000
```

### Option 3: PHP
```bash
# Navigate to docs directory
cd DataTracker2/docs

# Start PHP built-in server
php -S localhost:8000

# Then open: http://localhost:8000
```

### Option 4: Ruby
```bash
# Navigate to docs directory
cd DataTracker2/docs

# Start HTTP server
ruby -run -ehttpd . -p8000

# Then open: http://localhost:8000
```

### Option 5: Live Server (VS Code Extension)
1. Install "Live Server" extension in VS Code
2. Right-click on `docs/flash.html`
3. Select "Open with Live Server"
4. Browser opens automatically

## What Gets Flashed?

The firmware includes:
- ✅ **Main firmware** (application code) - 1.1 MB
- ✅ **Bootloader** - Flash bootloader
- ✅ **Partitions** - Flash layout definition
- ✅ **Boot app** - Secondary bootloader

All files are in: `DataTracker2/docs/firmware/`

## Firmware Version Information

- **Version**: 2.6.4 (Stable)
- **Build**: Auto-Fetch Fix
- **Platform**: ESP32-C3
- **Features**: Bitcoin, Ethereum, Stock Prices, Weather, Custom Values, Settings

## Need Help?

If you encounter issues:

1. **Check the Troubleshooting section above**
2. **Check the device display** for status messages
3. **Check serial console** output:
   - After flashing, open Arduino IDE → Tools → Serial Monitor (115200 baud)
   - This shows detailed debug information
4. **Review the README.md** in the DataTracker2 folder

## Next Steps After Successful Flash

Once your device is flashing successfully:

1. ✅ Device creates `DataTracker-XXXX` WiFi network
2. ✅ Connect and configure WiFi credentials
3. ✅ Select your initial module (Bitcoin, Stock, Weather, etc.)
4. ✅ Device connects to your WiFi and starts fetching data
5. ✅ Use the button to cycle through modules
6. ✅ Visit `http://dt.local` to change settings

Enjoy your DataTracker! 🎉

---

**Questions or Issues?**
Check the full documentation in the DataTracker2 folder or the repository README.
