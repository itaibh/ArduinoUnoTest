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
  BLE.begin(115200);    // Default AT command baud rate for AT-09

  Serial.println("AT Command Mode Ready");

  // Check if AT-09 is responding
  BLE.println("AT+BAUD115200");
  delay(1000);
  while (BLE.available()) {
    String response = BLE.readString();
    Serial.println("Response: " + response);  // Check the module's response
  }

  // Try to connect to a BLE device by its MAC address
  String macAddress = "09A30536C472";  // Example MAC address
  String command = "AT+CON" + macAddress;  // Concatenate the command
  BLE.println(command);  // Send the connection command

  // Wait for response and print it
  while (BLE.available()) {
    String response = BLE.readString();
    Serial.println("Response: " + response);
  }

  // // Example: Sending binding command
  // sendATCommand("AT+ROLE=1"); // Set to master mode
  // sendATCommand("AT+BIND=09,A3,05,36,C4,72");
  // sendATCommand("AT+INIT"); // Initialize the connection
}

void loop() {
  bool currentButtonState = digitalRead(buttonPin);

  // Check for button press
  if (currentButtonState == LOW && lastButtonState == HIGH) {  // Button press detected
    ledState = !ledState;  // Toggle LED state
    delay(50);             // Debounce delay
  }

  lastButtonState = currentButtonState;

  if (ledState) {
    digitalWrite(ledPin, HIGH);
    delay(500);
    digitalWrite(ledPin, LOW);
    delay(500);
  } else {
    digitalWrite(ledPin, LOW);  // Turn off the LED if the loop is paused
  }

  // Forward data from Serial Monitor to BLE device (if any)
  if (Serial.available()) {
    String data = Serial.readStringUntil('\n');
    BLE.println(data);  // Send data to BLE device
    Serial.println("Sent to BLE: " + data);
  }

  // Forward data from BLE device to Serial Monitor (if any)
  if (BLE.available()) {
    String response = BLE.readString();
    Serial.println("Received from BLE: " + response);
  }
}

// Function to send AT commands
void sendATCommand(String command) {
  Serial.print("Sending: ");
  Serial.println(command);
  BLE.println(command);     // Send the command to HC-05
  delay(1000);              // Wait for the response
}
