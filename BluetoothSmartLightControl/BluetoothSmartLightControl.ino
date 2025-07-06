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

const int LIGHT_BRIGHTNESS_STEP = 15;

const int MAIN_LIGHT = 0;
const int RGB_RING = 1;

// --- State Variables ---
int currentBrightness = 0; // 0-255 (for actual PWM light control)

bool fanSpeedUpPressed = false;
bool fanSpeedDownPressed = false;

int mode = MAIN_LIGHT;
int currentFanSpeed = 0; // 0=Off, 1=Low, 2=Medium, 3=High
int currentLightWarmness = 0; // 0x00 - 0xFA
int lightWarmnessStep = 10;

int hue = 0, ringR = 255, ringG = 0, ringB = 0;

// --- main light toggle button variables ---
bool lightOn = false;      // True if light is active
unsigned long lastLightPressTime = 0; 
unsigned long lastDebounceTime = 0;
unsigned long lastLongPressActionTime = 0;
int lightToggleButtonState;
int lastLightToggleButtonState = HIGH;
bool isLongPress = false;
int clickCount = 0;

// --- Bluetooth Classic Setup ---
BluetoothSerial SerialBT;

// --- Debounce Variables for Buttons ---
const long debounceDelay = 50; // ms
const long longPressDelay = 750; // ms
const long doubleClickDelay = 500; // ms
const unsigned long longPressActionInterval = 50; // Call action every 50ms

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
    if (mode == MAIN_LIGHT){
        Serial.print("Light Brightness: ");
        Serial.println(currentBrightness);
    } else {
        recomputeRGB();
    }
    // In a real scenario, you'd send this PWM value to a MOSFET/LED driver.
    // For this prototype, the brightness change is internal logic and reported via serial/BT.
}

void toggleLight() {
    lightOn = !lightOn;
    if (lightOn) {
        Serial.println("Light ON");
    } else {
        Serial.println("Light OFF");
    }
}

void rotateHue() {
    hue = (++hue) % 100;
    recomputeRGB();
}

void recomputeRGB() {
    hslToRgb((float)(hue / 100.0), 1.0, (float)(currentBrightness/255.0), &ringR, &ringG, &ringB);
    Serial.printf("Ring RGB: %d, %d, %d (hue: %d, brightness: %d)\n", ringR, ringG, ringB, hue, currentBrightness);
}

void hslToRgb(float h, float s, float l, int* r, int* g, int* b) {
    Serial.printf("hsltorgb: h: %f, s: %f, l: %f \n", h, s, l);
    if (s == 0.0) {
        *r = *g = *b = (int)round(l * 255.0);
    } else {
        float q = l < 0.5 ? l * (1.0 + s) : l + s - l * s;
        float p = 2.0 * l - q;
        *r = (int)round(255 * hueToRgb(p, q, h + 1.0/3.0));
        *g = (int)round(255 * hueToRgb(p, q, h));
        *b = (int)round(255 * hueToRgb(p, q, h - 1.0/3.0));
    }
}

float hueToRgb(float p, float q, float t) {
  if (t < 0.0) t += 1;
  if (t > 1.0) t -= 1;
  if (t < 1.0/6.0) return p + (q - p) * 6.0 * t;
  if (t < 1.0/2.0) return q;
  if (t < 2.0/3.0) return p + (q - p) * (2.0/3.0 - t) * 6.0;
  return p;
}

void changeWarmness() {
    if (!lightOn) return;
    currentLightWarmness += lightWarmnessStep;
    if (currentLightWarmness >= 0xFA) {
        currentLightWarmness = 0xFA;
        lightWarmnessStep = -lightWarmnessStep;
    } else if (currentLightWarmness <= 0) {
        currentLightWarmness = 0x00;
        lightWarmnessStep = -lightWarmnessStep;
    }

    Serial.print("warmness: ");
    Serial.println(currentLightWarmness);
}

void switchMode() {
    if (mode == MAIN_LIGHT) {
        mode = RGB_RING;
        Serial.println("switched to RGB Ring");
    } else {
        mode = MAIN_LIGHT;
        Serial.println("switched to Main Light");
    }
}

void increaseBrightness() {
    if (!lightOn) return; // if light is off, ignore
    setLightBrightness(currentBrightness + LIGHT_BRIGHTNESS_STEP); // Increase by a step
}

void decreaseBrightness() {
    if (!lightOn) return; // if light is off, ignore
    setLightBrightness(currentBrightness - LIGHT_BRIGHTNESS_STEP); // Decrease by a step
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

    int reading = digitalRead(LIGHT_TOGGLE_BTN_PIN);

    // If the switch changed, due to noise or pressing, reset the debounce timer
    if (reading != lastLightToggleButtonState) {
        lastDebounceTime = millis();
    }

    // After the debounce delay, if the state is stable, process it
    if ((millis() - lastDebounceTime) > debounceDelay) {
        // If the button state has changed
        if (reading != lightToggleButtonState) {
            lightToggleButtonState = reading;

            // --- Step 2: Handle Press and Release Events ---
            if (lightToggleButtonState == LOW) { // Button was just PRESSED
                isLongPress = false;
                lastLightPressTime = millis();
                clickCount++;
                Serial.println("Button Pressed");

            } else { // Button was just RELEASED
                Serial.println("Button Released");
                // If it was a long press, we don't do anything on release
                // The action was handled in Step 3. We just reset the flag.
                if (isLongPress) {
                    isLongPress = false; 
                }
            }
        }
    }
      
    lastLightToggleButtonState = reading; // Save the current reading for next time

    // --- Step 3: Handle Continuous Long Press ---
    // Part A: DETECT the start of a long press
    // This block runs only ONCE when the long press threshold is crossed.
    if (lightToggleButtonState == LOW && !isLongPress && (millis() - lastLightPressTime > longPressDelay)) {
        Serial.println("Long Press Started!");
        isLongPress = true;
        clickCount = 0; // Cancel any pending single/double clicks
    }
   
    // Part B: PERFORM the continuous action while in a long press state
    // This block runs REPEATEDLY as long as the button is held.
    if (isLongPress) {
        // Rate-limit the action to avoid changing too fast
        if (millis() - lastLongPressActionTime > longPressActionInterval) {
            lastLongPressActionTime = millis(); // Update the time of the last action

            Serial.println("...Executing continuous action");
            if (mode == MAIN_LIGHT) {
                changeWarmness();
            } else {
                rotateHue();
            }
        }
    }


    // --- Step 4: Detect Single and Double Clicks ---
    // This check happens after the button has been released and the double-click window has passed
    if (clickCount > 0 && lightToggleButtonState == HIGH && (millis() - lastLightPressTime) > doubleClickDelay) {
        if (clickCount == 1) {
            // SINGLE CLICK detected
            Serial.println("Single Click Detected!");
            toggleLight();
        } else if (clickCount == 2) {
            // DOUBLE CLICK detected
            Serial.println("Double Click Detected!");
            switchMode();
        }
        // Reset click counter after action
        clickCount = 0;
    }

    // if (digitalRead(LIGHT_TOGGLE_BTN_PIN) == LOW) {
    //     if (!isLightTogglePressed) {
    //         lastLightPressTime = millis();
    //         isLightTogglePressed = true;
    //         Serial.println("Light toggle pressed");
    //     } else if (millis() - longPressDelay > lastLightPressTime) {
    //         if (mode == MAIN_LIGHT){
    //             changeWarmness();
    //         } else {
    //             rotateHue();
    //         }
    //     }
    // }
    // if (digitalRead(LIGHT_TOGGLE_BTN_PIN) == HIGH && isLightTogglePressed) {
    //     Serial.println("Light toggle released");
    //     if (millis() - doubleClickDelay < lastLightPressTime) {
    //         isLightTogglePressed = false;
    //         if (++clickCount == 2) {
    //             clickCount = 0;
    //             switchMode();
    //         }
    //     } else if (millis() - longPressDelay < lastLightPressTime) {
    //         clickCount = 0;
    //         toggleLight();
    //         isLightTogglePressed = false;
    //         lastLightPressTime = 0;
    //     }
    // }

    if (digitalRead(FAN_SPEED_UP_BTN_PIN) == LOW && !fanSpeedUpPressed) {
        fanSpeedUpPressed = true;
        increaseFanSpeed();
    }
    if (digitalRead(FAN_SPEED_UP_BTN_PIN) == HIGH && fanSpeedUpPressed) {
        fanSpeedUpPressed = false;
    }
    
    if (digitalRead(FAN_SPEED_DOWN_BTN_PIN) == LOW && !fanSpeedDownPressed) {
        fanSpeedDownPressed = true;
        decreaseFanSpeed();
    }
    if (digitalRead(FAN_SPEED_DOWN_BTN_PIN) == HIGH && fanSpeedDownPressed) {
        fanSpeedDownPressed = false;
    }

    // // --- Bluetooth Classic Communication ---
    // if (SerialBT.available()) {
    //     String command = SerialBT.readStringUntil('\n'); // Read incoming command
    //     command.trim(); // Remove any whitespace

    //     Serial.print("Received BT Command: ");
    //     Serial.println(command);

    //     if (command == "L+") {
    //         increaseBrightness();
    //     } else if (command == "L-") {
    //         decreaseBrightness();
    //     } else if (command == "LO") {
    //         toggleLight();
    //     } else if (command == "F+") {
    //         increaseFanSpeed();
    //     } else if (command == "F-") {
    //         decreaseFanSpeed();
    //     }
    //     // Example for future: Light Temperature control via BT
    //     else if (command.startsWith("LT")) {
    //          int tempVal = command.substring(2).toInt(); // Extract number after "LT"
    //          setLightTemperature(tempVal);
    //     }

    //     // Send status back to connected Bluetooth device
    //     SerialBT.print("STATUS: Light_"); SerialBT.print(lightOn ? "ON" : "OFF");
    //     SerialBT.print(" Brightness_"); SerialBT.print(currentBrightness);
    //     SerialBT.print(" Fan_"); SerialBT.println(currentFanSpeed);
    // }

    delay(10); // Small delay to prevent continuous re-reading in loop
}