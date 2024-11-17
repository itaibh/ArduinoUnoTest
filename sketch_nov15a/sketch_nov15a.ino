#include <SoftwareSerial.h>

// Define SoftwareSerial pins for AT-09
SoftwareSerial BLE(10, 11); // RX, TX

const int ledPin = 13;    // LED connected to pin 13
const int buttonPin = 2;  // Button connected to pin 2

bool ledState = false;    // Tracks whether the LED is on or off
bool lastButtonState = LOW;

void setup() {
  pinMode(ledPin, OUTPUT);
  pinMode(buttonPin, INPUT_PULLUP);  // Enable internal pull-up resistor

  Serial.begin(9600); // Serial Monitor

  //testForBaudRate();
  
  BLE.begin(9600);    // Default AT command baud rate for AT-09
  Serial.println("AT Command Mode Ready");

  // Check if AT-09 is responding
  BLE.println("AT");
  delay(1000);
  while (BLE.available()) {
    String response = BLE.readString();
    Serial.println("Response (1): " + response);  // Check the module's response
  }

  BLE.println("AT+VERSION");
  waitForBleResponse("Version");

  sendATCommand("AT+HELP?"); // Set to master mode
  waitForBleResponse("help");
  
  sendATCommand("AT+ROLE1"); // Set to master mode
  waitForBleResponse("Role");
  
  delay(1000);
  
  sendATCommand("AT+IMME1"); // turn off automatic connection
  waitForBleResponse("Immediate");
  
  delay(1000);

  discoverBleDevices();

  // delay(3000);

  // BLE.println("AT+MODE?");
  // waitForBleResponse("mode");

  // delay(1000);

  // BLE.println("AT+STATE");
  // waitForBleResponse("State");
  
  // BLE.println("AT+ROLE?");
  // waitForBleResponse("Role");

  // sendATCommand("AT+ADDR?");
  // waitForBleResponse("MAC Address");

  // delay(1000);

  // BLE.println("AT+DISC?");
  // waitForBleResponse("Devices");
  
  // delay(1000);
  // discoverBleDevices();
  //getBleFirmwareVersion();  


  // // Try to connect to a BLE device by its MAC address
  // String macAddress = "09A30536C472";  // Example MAC address
  // String command = "CON" + macAddress;  // Concatenate the command
  // Serial.println("Sending command: " + command);
  // BLE.println(command);  // Send the connection command
  // waitForBleResponse("Connect");

  // Wait for response and print it
  // // while (BLE.available()) {
  // //   String response = BLE.readString();
  // //   Serial.println("Response (2): " + response);
  // // }

  // sendATCommand("AT+BIND=09,A3,05,36,C4,72");
  // waitForBleResponse("Bind");
  // sendATCommand("AT+INIT"); // Initialize the connection
  // waitForBleResponse("Init");
}

int baudRates[] = {9600, 38400, 57600, 115200}; // Common baud rates
int currentRate = 0;

void testForBaudRate() {
  Serial.begin(9600);  // For debugging
  Serial.println("Testing baud rates...");

  for (int i = 0; i < 4; i++) {
    currentRate = baudRates[i];
    Serial.print("Testing baud rate: ");
    Serial.println(currentRate);

    BLE.begin(currentRate);
    BLE.println("AT");
    delay(1000);

    if (BLE.available()) {
      String response = BLE.readString();
      Serial.println("Response: " + response);
      if (response.indexOf("OK") >= 0) {
        Serial.print("Supported baud rate is: ");
        Serial.println(currentRate);
        //break;
      }
    }
  }
}

void getBleFirmwareVersion() {
  BLE.println("AT+VERSION");
  waitForBleResponse("Firmware Version");
}

void discoverBleDevices() {
  Serial.println("Discoverring Devices");
  BLE.println("AT+DISC?");
  waitForBleResponse("Devices");
}

void waitForBleResponse(const String &label)
{
  unsigned long timeout = millis() + 2000;  // 2-second timeout
  while (millis() < timeout) {
    if (BLE.available()) {
      String response = BLE.readString();
      Serial.println(label + ": " + response);
      return;  // Exit once a response is received
    }
  }
  Serial.println(label + ": No Response (Timeout)");
}

void loop() {
  // bool currentButtonState = digitalRead(buttonPin);

  // // Check for button press
  // if (currentButtonState == LOW && lastButtonState == HIGH) {  // Button press detected
  //   ledState = !ledState;  // Toggle LED state
  //   delay(50);             // Debounce delay
  // }

  // lastButtonState = currentButtonState;

  // if (ledState) {
  //   digitalWrite(ledPin, HIGH);
  //   delay(500);
  //   digitalWrite(ledPin, LOW);
  //   delay(500);
  // } else {
  //   digitalWrite(ledPin, LOW);  // Turn off the LED if the loop is paused
  // }

  // // Forward data from Serial Monitor to BLE device (if any)
  // if (Serial.available()) {
  //   String data = Serial.readStringUntil('\n');
  //   BLE.println(data);  // Send data to BLE device
  //   Serial.println("Sent to BLE: " + data);
  // }

  // // Forward data from BLE device to Serial Monitor (if any)
  // if (BLE.available()) {
  //   String response = BLE.readString();
  //   Serial.println("Received from BLE: " + response);
  // }
}

// Function to send AT commands
void sendATCommand(String command) {
  Serial.print("Sending: ");
  Serial.println(command);
  BLE.println(command);     // Send the command to HC-05
  delay(1000);              // Wait for the response
}
