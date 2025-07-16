// Rotary Encoder Pin Definitions
const int pinA = 14;  // Signal A (Brown)
const int pinB = 12;  // Signal B (Blue/White)

volatile int encoderCount = 0;
volatile bool directionCW = true;

void IRAM_ATTR handleEncoder() {
  // Read both encoder pins
  bool A = digitalRead(pinA);
  bool B = digitalRead(pinB);

  // Determine direction
  if (A == B) {
    encoderCount++;
    directionCW = true;
  } else {
    encoderCount--;
    directionCW = false;
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("Rotary Encoder Test: Direction + Count");

  pinMode(pinA, INPUT_PULLUP);
  pinMode(pinB, INPUT_PULLUP);

  // Attach interrupt to Signal A only
  attachInterrupt(digitalPinToInterrupt(pinA), handleEncoder, CHANGE);
}

void loop() {
  static int lastCount = 0;

  // Only print when the count changes
  if (encoderCount != lastCount) {
    lastCount = encoderCount;

    Serial.print("Direction: ");
    Serial.print(directionCW ? "CW  " : "CCW ");
    Serial.print(" | Count: ");
    Serial.println(encoderCount);
  }

  delay(1);  // Light CPU load
}
