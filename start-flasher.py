#!/usr/bin/env python3
"""
ESP32 DataTracker2 - Local Web Server for Firmware Flashing
Double-click this file to start the web server
"""

import http.server
import socketserver
import os
import sys
import webbrowser
from pathlib import Path

def find_available_port(preferred_ports=[8000, 8080, 8888]):
    """Find an available port from the list of preferred ports."""
    import socket
    for port in preferred_ports:
        try:
            with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
                s.bind(('', port))
                return port
        except OSError:
            continue
    # If all preferred ports are taken, let the system choose
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.bind(('', 0))
        return s.getsockname()[1]

def main():
    # Change to the docs directory
    script_dir = Path(__file__).parent
    docs_dir = script_dir / 'docs'
    os.chdir(docs_dir)

    # Find available port
    port = find_available_port()

    print("=" * 50)
    print("  ESP32 DataTracker2 Firmware Flasher")
    print("=" * 50)
    print()
    print("Starting local web server...")
    print(f"Open your browser to:")
    print()
    print(f"  http://localhost:{port}")
    print()
    print("Press Ctrl+C to stop the server")
    print("=" * 50)
    print()

    # Try to open browser automatically
    try:
        webbrowser.open(f'http://localhost:{port}')
    except:
        pass

    # Start server
    Handler = http.server.SimpleHTTPRequestHandler

    try:
        with socketserver.TCPServer(("", port), Handler) as httpd:
            httpd.serve_forever()
    except KeyboardInterrupt:
        print("\n\nServer stopped.")
        input("Press Enter to exit...")

if __name__ == "__main__":
    main()
