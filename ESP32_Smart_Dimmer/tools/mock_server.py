import http.server
import socketserver
import json
import time
import os
from urllib.parse import urlparse, parse_qs

PORT = 8080 # You can change this port if 8080 is already in use
DEVICE_DISCOVERY_PATH = "/discover_devices" # This will now simulate BT scanning results
CONTROL_PATH_PREFIX = "/control?"
GET_ALL_DEVICES_PATH = "/get_all_devices" # Returns configured devices (from registered_devices)
ADD_DEVICE_PATH_PREFIX = "/add_device?" # Adds a new device to registered_devices
REMOVE_DEVICE_PATH_PREFIX = "/remove_device?" # Removes a device from registered_devices


# Simulated Bluetooth discovered devices (these do NOT have control parameters initially)
mock_discovered_devices = [
    {
        "name": "Living Room BT",
        "mac_address": "AA:BB:CC:DD:EE:F1",
        "rssi": -55
    },
    {
        "name": "Bedroom BT",
        "mac_address": "A1:B2:C3:D4:E5:F2",
        "rssi": -70
    },
    {
        "name": "Work Room BT",
        "mac_address": "11:22:33:44:55:F3",
        "rssi": -60
    }
]

# This dictionary will simulate the 'StorageHandler's allManagedDevices map
# Key: MAC address (String), Value: Dictionary representing DeviceConfig
# Start with an empty set, or pre-populate some for initial testing
registered_devices = {
    "AA:BB:CC:DD:EE:F1": { # Example of a pre-registered device
        "mac_address": "AA:BB:CC:DD:EE:F1",
        "name": "Living Room Light (Configured)", # Name can be more descriptive here
        "fan_speed": 0,
        "light_mode": "main",
        "main_brightness": 100,
        "main_warmness": 150,
        "ring_hue": 0,
        "ring_brightness": 0,
        "is_on": True
    },
    "A1:B2:C3:D4:E5:F2": { # Another example
        "mac_address": "A1:B2:C3:D4:E5:F2",
        "name": "Bedroom Fan (Configured)",
        "fan_speed": 2,
        "light_mode": "rgb",
        "main_brightness": 0,
        "main_warmness": 0,
        "ring_hue": 120,
        "ring_brightness": 80,
        "is_on": False
    }
}


# --- IMPORTANT CHANGE HERE ---
# Get the absolute path to the directory where this script (mock_server.py) is located
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
        # /discover_devices: Simulate Bluetooth scan results
        if self.path == DEVICE_DISCOVERY_PATH:
            time.sleep(4) # simulate 4 seconds search time
            self.send_response(200)
            self.send_header('Content-type', 'application/json')
            self.send_header('Access-Control-Allow-Origin', '*') # Allow CORS for your HTML page
            self.end_headers()
            # Add 'is_configured' flag to mock discovered devices based on registered_devices
            response_devices = []
            for device in mock_discovered_devices:
                device_copy = device.copy() # Avoid modifying the original mock_discovered_devices
                device_copy["is_configured"] = device_copy["mac_address"] in registered_devices
                response_devices.append(device_copy)

            self.wfile.write(json.dumps(response_devices).encode('utf-8'))
            print(f"[{time.ctime()}] Responded to {self.path} with {len(response_devices)} discovered devices (incl. configured status).")

        # /control?: Simulate controlling a specific device
        elif self.path.startswith(CONTROL_PATH_PREFIX):
            self.send_response(200)
            self.send_header('Content-type', 'text/plain')
            self.send_header('Access-Control-Allow-Origin', '*') # Allow CORS
            self.end_headers()

            query_string = urlparse(self.path).query
            params = parse_qs(query_string)

            address = params.get('address', [None])[0] # Assuming 'device_id' is the MAC address

            log_message = f"[{time.ctime()}] Received control request for device_id: {address}\n"
            log_message += "  Parameters:\n"
            for key, value in params.items():
                log_message += f"    {key}: {value[0] if value else 'N/A'}\n"

            lightMode = params['mode'][0]
            if address and address in registered_devices:
                device_config = registered_devices[address]
                
                # Update mock device state based on received parameters
                if 'brightness' in params:
                    device_config['main_brightness'] = int(params['brightness'][0])
                if 'mode' in params:
                    if (lightMode == "off"):
                        device_config['is_on'] = "false"
                    else:
                        device_config['is_on'] = "true"
                        device_config['light_mode'] = lightMode
                if 'fan_speed' in params:
                    device_config['fan_speed'] = int(params['fan_speed'][0])
                if 'warmness' in params:
                    device_config['main_warmness'] = int(params['warmness'][0])
                if 'hue' in params:
                    device_config['ring_hue'] = int(params['hue'][0])
                if 'rgb_brightness' in params:
                    device_config['ring_brightness'] = int(params['rgb_brightness'][0])
                
                print(log_message + f"  Updated state for {address}: {device_config}")
                self.wfile.write(b"OK")
            else:
                print(log_message + f"  Error: Device {address} not found in registered devices.")
                self.wfile.write(b"ERROR: Device not found.")

        # /get_all_devices: Return all registered (configured) devices
        elif self.path == GET_ALL_DEVICES_PATH:
            self.send_response(200)
            self.send_header('Content-type', 'application/json') # Corrected to application/json
            self.send_header('Access-Control-Allow-Origin', '*')
            self.end_headers()
            
            # Convert the dictionary values to a list for JSON array response
            # devices_list = list(registered_devices.values())
            self.wfile.write(json.dumps(registered_devices).encode('utf-8'))
            print(f"[{time.ctime()}] Responded to {self.path} with {len(registered_devices)} registered devices.")

        # /add_device?: Register a new device with default settings
        elif self.path.startswith(ADD_DEVICE_PATH_PREFIX):
            self.send_response(200)
            self.send_header('Content-type', 'text/plain') # Can be application/json if you want to return device details
            self.send_header('Access-Control-Allow-Origin', '*')
            self.end_headers()

            query_string = urlparse(self.path).query
            params = parse_qs(query_string)

            new_name = params.get('name', [None])[0]
            new_address = params.get('address', [None])[0]

            if new_address and new_name:
                if new_address in registered_devices:
                    response_msg = f"Device {new_name} ({new_address}) already registered."
                    print(f"[{time.ctime()}] {response_msg}")
                    self.wfile.write(response_msg.encode('utf-8'))
                else:
                    # Create a new device config with default values
                    registered_devices[new_address] = {
                        "mac_address": new_address,
                        "name": new_name, # Use the name provided
                        "fan_speed": 0,
                        "light_mode": "main",
                        "main_brightness": 0, # Start off? Or give some default
                        "main_warmness": 150,
                        "ring_hue": 0,
                        "ring_brightness": 0,
                        "is_on": False # Start off by default
                    }
                    response_msg = f"Device {new_name} ({new_address}) added successfully."
                    print(f"[{time.ctime()}] {response_msg}")
                    self.wfile.write(response_msg.encode('utf-8'))
            else:
                response_msg = "Error: Missing 'name' or 'address' parameters for add_device."
                print(f"[{time.ctime()}] {response_msg}")
                self.wfile.write(response_msg.encode('utf-8'))
        elif self.path.startswith(REMOVE_DEVICE_PATH_PREFIX):
            self.send_response(200)
            self.send_header('Content-type', 'text/plain') # Can be application/json if you want to return device details
            self.send_header('Access-Control-Allow-Origin', '*')
            self.end_headers()

            query_string = urlparse(self.path).query
            params = parse_qs(query_string)

            address = params.get('address', [None])[0]
            if address and address in registered_devices:
                del registered_devices[address]
            self.wfile.write(b"OK")
        else:
            # For all other GET requests, serve static files from the WEB_ROOT_DIR ('data' folder)
            print(f"[{time.ctime()}] Serving static file from '{WEB_ROOT_DIR}': {self.path}")
            super().do_GET()

# Create the server
Handler = CustomHandler
with socketserver.TCPServer(("", PORT), Handler) as httpd:
    httpd.allow_reuse_address = True
    print(f"Serving mock device server from '{SCRIPT_DIR}' at port {PORT}")
    print(f"Serving web assets from '{WEB_ROOT_DIR}'")
    print(f"Access device discovery at http://localhost:{PORT}{DEVICE_DISCOVERY_PATH}")
    print(f"Access all registered devices at http://localhost:{PORT}{GET_ALL_DEVICES_PATH}")
    print(f"Test add device: http://localhost:{PORT}{ADD_DEVICE_PATH_PREFIX}name=TestDevice&address=00:11:22:AA:BB:CC")
    print(f"Access your HTML at http://localhost:{PORT}/index.html")
    httpd.serve_forever()