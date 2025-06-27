// This sketch combines scanning for a specific device and then connecting to it.
#include "BluetoothSerial.h"
#include "esp_gap_bt_api.h"

// Initialize the BluetoothSerial object
BluetoothSerial SerialBT;

// --- Configuration ---
// The name of the device you want to connect to.
const String TARGET_DEVICE_NAME = "חדר עבודה"; 
// The MAC address will be found automatically by the scanner.
BTAddress* targetDeviceAddress = nullptr; 

// Data payload to send to the light.
uint8_t command_to_send[] = {
  0x01, 0xfe, 0x00, 0x00, 0x51, 0x81, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x0d, 0x07, 0x01, 0x03, 0x03, 0xff, 0x0e, 0x00 // set intensity of main light to 0xff
};

bool device_found = false;
bool command_sent = false;

// Function to scan for the target device
void scanForTargetDevice() {
  Serial.println("------------------------------------");
  Serial.println("Scanning for devices (10 seconds)...");

  BTScanResults* scanResults = SerialBT.discover(10000);

  if (scanResults != nullptr && scanResults->getCount() > 0) {
    Serial.printf("Found %d devices:\n", scanResults->getCount());
    for (int i = 0; i < scanResults->getCount(); i++) {
      BTAdvertisedDevice* device_result = scanResults->getDevice(i);
      String name = device_result->getName().c_str();
      BTAddress address = device_result->getAddress();
      Serial.printf("  - Found Device: %s, Address: %s\n", name.c_str(), address.toString().c_str());

      // Check if this is the device we are looking for
      if (name.equals(TARGET_DEVICE_NAME)) {
        Serial.println(">>> Target device found! <<<");
        targetDeviceAddress = new BTAddress(address); // Store the address
        device_found = true;
        break; // Stop scanning once found
      }
    }
  } else {
    Serial.println("No classic Bluetooth devices found.");
  }

  if (scanResults != nullptr) {
    // delete scanResults;
  }

  if (!device_found) {
     Serial.println("Target device not found during scan. Will retry in 30 seconds.");
  }
}


void setup() {
  Serial.begin(115200);
  Serial.println("Starting ESP32 Bluetooth Client...");

  // Initialize Bluetooth in MASTER mode, which is required to initiate a connection.
  if (!SerialBT.begin("ESP32-Client", true)) {
    Serial.println("An error occurred initializing Bluetooth in master mode");
    while(1);
  }
  Serial.println("Bluetooth initialized successfully.");
  
  // Perform the first scan to find our target device.
  scanForTargetDevice();
}

void loop() {
  // If the device has been found but we are not connected, try to connect.
  if (device_found && !SerialBT.connected() && !command_sent) {
    Serial.print("Attempting to connect to ");
    Serial.println(TARGET_DEVICE_NAME);
    
    // The connect() method with a BTAddress object is often more reliable.
    if (SerialBT.connect(*targetDeviceAddress)) {
      Serial.println("Successfully connected!");

      // --- Send the command ---
      Serial.print("Sending command payload: ");
      for(int i = 0; i < sizeof(command_to_send); i++){
        Serial.printf("%02X ", command_to_send[i]);
      }
      Serial.println();
      
      SerialBT.write(command_to_send, sizeof(command_to_send));
      SerialBT.flush(); // Wait for the data to be sent
      Serial.println("Command sent.");
      
      command_sent = true; // Set a flag so we don't send it again
      delay(2000); // Wait a moment
      SerialBT.disconnect(); // Disconnect after sending
      Serial.println("Disconnected.");
    } else {
      Serial.println("Failed to connect. Will retry...");
      // The loop will automatically retry on the next iteration.
    }
  } 
  // If the device wasn't found, or if we have already sent the command, just wait and then rescan/restart.
  else {
    Serial.println("Device not found or command already sent. Restarting process in 30 seconds...");
    delay(30000);
    // Reset state and rescan
    device_found = false;
    command_sent = false;
    if(targetDeviceAddress != nullptr){
        delete targetDeviceAddress;
        targetDeviceAddress = nullptr;
    }
    scanForTargetDevice();
  }
  
  delay(5000); // Wait 5 seconds between connection attempts.
}
