const int ledPin = 13;    // LED connected to pin 13
const int buttonPin = 2;  // Button connected to pin 2

bool ledState = false;    // Tracks whether the LED is on or off
bool lastButtonState = LOW;

void setup() {
  pinMode(ledPin, OUTPUT);
  pinMode(buttonPin, INPUT_PULLUP);  // Enable internal pull-up resistor
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
}
