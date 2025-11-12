#!/usr/bin/env python3
"""
Local Web Flasher for DataTracker Firmware
Serves the web flasher interface locally on port 8000
Usage: python3 webflasher.py
Then open: http://localhost:8000
"""

import http.server
import socketserver
import os
import sys
from pathlib import Path

PORT = 8000

class MyHTTPRequestHandler(http.server.SimpleHTTPRequestHandler):
    def end_headers(self):
        # Add CORS headers to allow cross-origin requests
        self.send_header('Access-Control-Allow-Origin', '*')
        self.send_header('Access-Control-Allow-Methods', 'GET, POST, OPTIONS')
        self.send_header('Access-Control-Allow-Headers', 'Content-Type')
        # Add cache control headers for manifest
        if self.path.endswith('.json'):
            self.send_header('Cache-Control', 'no-cache, must-revalidate')
        super().end_headers()

    def do_OPTIONS(self):
        self.send_response(200)
        self.end_headers()

# Change to docs directory where flash.html is located
docs_dir = Path(__file__).parent / "docs"
os.chdir(docs_dir)

print(f"""
╔══════════════════════════════════════════════════════════════╗
║          DataTracker Web Flasher - Local Server              ║
╚══════════════════════════════════════════════════════════════╝

📁 Serving from: {docs_dir}

🚀 Open your browser and go to:
   http://localhost:{PORT}

⚡ Instructions:
   1. Connect your ESP32-C3 via USB cable
   2. Click the "Install Firmware" button
   3. Select your device's USB port
   4. Wait for flashing to complete (1-2 minutes)
   5. Disconnect and reconnect to restart device

⚠️  Requirements:
   - Chrome 89+ or Edge 89+ (requires Web Serial API)
   - USB data cable (not charging cable)
   - ESP32-C3 device

💡 To stop the server, press Ctrl+C

Starting server...
""")

try:
    with socketserver.TCPServer(("", PORT), MyHTTPRequestHandler) as httpd:
        print(f"✅ Server running at http://localhost:{PORT}")
        print(f"✅ Press Ctrl+C to stop\n")
        httpd.serve_forever()
except KeyboardInterrupt:
    print("\n\n👋 Server stopped.")
    sys.exit(0)
except OSError as e:
    if "Address already in use" in str(e):
        print(f"\n❌ Error: Port {PORT} is already in use.")
        print(f"   Try: lsof -i :{PORT}  (to see what's using it)")
        print(f"   Or use a different port: python3 webflasher.py 9000")
    else:
        print(f"❌ Error: {e}")
    sys.exit(1)
