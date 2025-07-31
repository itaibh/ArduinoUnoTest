import http.server
import socketserver
import json
import time
import os # Import the os module
from urllib.parse import urlparse, parse_qs # Import urlparse and parse_qs

PORT = 8080 # You can change this port if 8080 is already in use
DEVICE_DISCOVERY_PATH = "/discover_devices"
CONTROL_PATH_PREFIX = "/control?"

# Simulated device data
mock_devices = [
    {
        "id": "light_living_room_1",
        "name": "Living Room Light",
        "ipAddress": "192.168.1.100", # Dummy IP for simulation
        "type": "Smart Light"
    },
    {
        "id": "light_kitchen_ring_1",
        "name": "Kitchen RGB Ring",
        "ipAddress": "192.168.1.101",
        "type": "Smart Light"
    },
    {
        "id": "fan_bedroom_1",
        "name": "Bedroom Fan",
        "ipAddress": "192.168.1.102",
        "type": "Smart Fan"
    }
]

# --- IMPORTANT CHANGE HERE ---
# Get the absolute path to the directory where this script (mock_device_server.py) is located
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
# Construct the path to the 'data' directory.
# '..' goes up one level from 'tools', then 'data' goes into the data folder.
WEB_ROOT_DIR = os.path.join(SCRIPT_DIR, '..', 'data')
# --- END IMPORTANT CHANGE ---

class CustomHandler(http.server.SimpleHTTPRequestHandler):
    def __init__(self, *args, **kwargs):
        # Pass the desired directory to serve static files from
        super().__init__(*args, directory=WEB_ROOT_DIR, **kwargs)

    def do_GET(self):
        if self.path == DEVICE_DISCOVERY_PATH:
            # Handle the specific device discovery API request
            self.send_response(200)
            self.send_header('Content-type', 'application/json')
            self.send_header('Access-Control-Allow-Origin', '*') # Allow CORS for your HTML page
            self.end_headers()
            self.wfile.write(json.dumps(mock_devices).encode('utf-8'))
            print(f"[{time.ctime()}] Responded to {self.path} with device list.")
        elif self.path.startswith(CONTROL_PATH_PREFIX):
            # Handle control API request
            self.send_response(200)
            self.send_header('Content-type', 'text/plain')
            self.send_header('Access-Control-Allow-Origin', '*') # Allow CORS
            self.end_headers()

            # Parse query parameters
            query_string = urlparse(self.path).query
            params = parse_qs(query_string)

            # Log received parameters
            log_message = f"[{time.ctime()}] Received control request: {self.path}\n"
            log_message += "  Parameters:\n"
            for key, value in params.items():
                log_message += f"    {key}: {value[0] if value else 'N/A'}\n" # Take first value if multiple

            print(log_message)

            # Send a simple 'OK' response to the client
            self.wfile.write(b"OK")
        else:
            # For all other GET requests, serve static files from the WEB_ROOT_DIR ('data' folder)
            # The super().do_GET() method automatically handles file serving based on the directory
            print(f"[{time.ctime()}] Serving static file from '{WEB_ROOT_DIR}': {self.path}")
            super().do_GET()

# Create the server
Handler = CustomHandler
with socketserver.TCPServer(("", PORT), Handler) as httpd:
    print(f"Serving mock device server from '{SCRIPT_DIR}' at port {PORT}")
    print(f"Serving web assets from '{WEB_ROOT_DIR}'")
    print(f"Access device discovery at http://localhost:{PORT}{DEVICE_DISCOVERY_PATH}")
    print(f"Access your HTML at http://localhost:{PORT}/index.html")
    httpd.serve_forever()