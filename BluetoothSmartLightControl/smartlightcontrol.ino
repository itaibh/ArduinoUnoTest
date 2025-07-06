#include <BluetoothSerial.h> // For ESP32 Bluetooth Classic

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error "Bluetooth is not enabled! Please run `make menuconfig` to enable it"
#endif

// --- Pin Definitions (Updated based on your setup) ---
const int LIGHT_UP_BTN_PIN = 16;   // Light Brightness Up
const int LIGHT_DOWN_BTN_PIN = 17; // Light Brightness Down
const int LIGHT_TOGGLE_BTN_PIN = 23; // Light ON/OFF (simulates knob push)

const int FAN_SPEED_UP_BTN_PIN = 18;   // Fan Speed Up
const int FAN_SPEED_DOWN_BTN_PIN = 19; // Fan Speed Down

// --- State Variables ---
int currentBrightness = 0; // 0-255 (for actual PWM light control)
bool lightOn = false;      // True if light is active

int currentFanSpeed = 0; // 0=Off, 1=Low, 2=Medium, 3=High

// --- Bluetooth Classic Setup ---
BluetoothSerial SerialBT;

// --- Debounce Variables for Buttons ---
const long debounceDelay = 50; // ms
unsigned long lastButtonPressTime[5]; // Array to store last press time for each button

// Function to handle button presses more generically
bool isButtonPressed(int pin, int buttonIndex) {
    bool buttonState = digitalRead(pin) == LOW; // Button is active LOW (pressed)
    if (buttonState && (millis() - lastButtonPressTime[buttonIndex] > debounceDelay)) {
        lastButtonPressTime[buttonIndex] = millis();
        return true;
    }
    return false;
}

// --- Light Control Functions ---
void setLightBrightness(int brightness) {
    currentBrightness = constrain(brightness, 0, 255); // Ensure within range
    Serial.print("Light Brightness: ");
    Serial.println(currentBrightness);
    // In a real scenario, you'd send this PWM value to a MOSFET/LED driver.
    // For this prototype, the brightness change is internal logic and reported via serial/BT.
}

void toggleLight() {
    lightOn = !lightOn;
    if (lightOn) {
        Serial.println("Light ON");
        if (currentBrightness == 0) { // If turning on from 0 brightness, set a default
            currentBrightness = 100;
        }
        setLightBrightness(currentBrightness);
    } else {
        Serial.println("Light OFF");
        setLightBrightness(0); // Effectively turn off light logic
    }
}

void increaseBrightness() {
    if (!lightOn) toggleLight(); // Turn on light if it's off
    setLightBrightness(currentBrightness + 25); // Increase by a step
}

void decreaseBrightness() {
    setLightBrightness(currentBrightness - 25); // Decrease by a step
    if (currentBrightness == 0) toggleLight(); // If brightness hits 0, turn off
}

// --- Fan Control Functions ---
void setFanSpeed(int speed) {
    currentFanSpeed = constrain(speed, 0, 3); // 0=Off, 1=Low, 2=Medium, 3=High
    String speedText;
    switch (currentFanSpeed) {
        case 0: speedText = "Off"; break;
        case 1: speedText = "Low"; break;
        case 2: speedText = "Medium"; break;
        case 3: speedText = "High"; break;
    }
    Serial.print("Fan Speed: ");
    Serial.println(speedText);
    // In a real device, you'd send signals to a relay or motor driver.
}

void increaseFanSpeed() {
    setFanSpeed(currentFanSpeed + 1);
}

void decreaseFanSpeed() {
    setFanSpeed(currentFanSpeed - 1);
}

// --- Light Temperature Simulation (Future Expansion - no visual feedback on prototype) ---
void setLightTemperature(int tempValue) { // 0 for warm, 255 for cool
    Serial.print("Light Temperature (0=Warm, 255=Cool): ");
    Serial.println(tempValue);
    // In a real device, this would adjust the CCT of a tunable white light source.
    // Feedback for this would primarily be seen in the actual light fixture's output.
}

// --- Setup Function ---
void setup() {
    Serial.begin(115200); // Initialize serial communication for debugging
    Serial.println("ESP32 Smart Dimmer Prototype Starting...");

    // Initialize button pins with internal pull-up resistors
    pinMode(LIGHT_UP_BTN_PIN, INPUT_PULLUP);
    pinMode(LIGHT_DOWN_BTN_PIN, INPUT_PULLUP);
    pinMode(LIGHT_TOGGLE_BTN_PIN, INPUT_PULLUP);
    pinMode(FAN_SPEED_UP_BTN_PIN, INPUT_PULLUP);
    pinMode(FAN_SPEED_DOWN_BTN_PIN, INPUT_PULLUP);

    // Initialize last button press times
    for (int i = 0; i < 5; i++) {
        lastButtonPressTime[i] = 0;
    }

    // Initialize Bluetooth Classic
    SerialBT.begin("SmartDimmer_ESP32"); // Bluetooth device name
    Serial.println("Bluetooth device name: SmartDimmer_ESP32");
    Serial.println("The device started, now you can pair it to your phone!");

    // Set initial state
    setLightBrightness(0); // Light off initially
    lightOn = false; // Ensure light is off
    setFanSpeed(0);        // Fan off initially
}

// --- Loop Function ---
void loop() {
    // --- Button Polling ---
    if (isButtonPressed(LIGHT_UP_BTN_PIN, 0)) {
        increaseBrightness();
    }
    if (isButtonPressed(LIGHT_DOWN_BTN_PIN, 1)) {
        decreaseBrightness();
    }
    if (isButtonPressed(LIGHT_TOGGLE_BTN_PIN, 2)) {
        toggleLight();
    }
    if (isButtonPressed(FAN_SPEED_UP_BTN_PIN, 3)) {
        increaseFanSpeed();
    }
    if (isButtonPressed(FAN_SPEED_DOWN_BTN_PIN, 4)) {
        decreaseFanSpeed();
    }

    // --- Bluetooth Classic Communication ---
    if (SerialBT.available()) {
        String command = SerialBT.readStringUntil('\n'); // Read incoming command
        command.trim(); // Remove any whitespace

        Serial.print("Received BT Command: ");
        Serial.println(command);

        if (command == "L+") {
            increaseBrightness();
        } else if (command == "L-") {
            decreaseBrightness();
        } else if (command == "LO") {
            toggleLight();
        } else if (command == "F+") {
            increaseFanSpeed();
        } else if (command == "F-") {
            decreaseFanSpeed();
        }
        // Example for future: Light Temperature control via BT
        else if (command.startsWith("LT")) {
             int tempVal = command.substring(2).toInt(); // Extract number after "LT"
             setLightTemperature(tempVal);
        }

        // Send status back to connected Bluetooth device
        SerialBT.print("STATUS: Light_"); SerialBT.print(lightOn ? "ON" : "OFF");
        SerialBT.print(" Brightness_"); SerialBT.print(currentBrightness);
        SerialBT.print(" Fan_"); SerialBT.println(currentFanSpeed);
    }

    delay(10); // Small delay to prevent continuous re-reading in loop
}