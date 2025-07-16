/*This worked well to just read the encoder and nothing else.*/


// Simple Rotary Encoder Test Without External Library
// --------------------------------------------------
// Reads a quadrature rotary encoder using direct digital reads and interrupt-based counting

// --- Pin Definitions ---
#define ENCODER_A 14  // Brown wire
#define ENCODER_B 12  // Blue/White wire

// --- Globals ---
volatile int encoderCount = 0;        // Current position
volatile bool lastEncodedState = 0;   // Last known A/B encoded state

void IRAM_ATTR handleEncoder() {
  bool A = digitalRead(ENCODER_A);
  bool B = digitalRead(ENCODER_B);

  // Determine direction and increment/decrement count
  if (A == B) {
    encoderCount++;
  } else {
    encoderCount--;
  }
}

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("\n--- Minimal Rotary Encoder Test (No Library) ---");

  pinMode(ENCODER_A, INPUT_PULLUP);
  pinMode(ENCODER_B, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(ENCODER_A), handleEncoder, CHANGE);
}

void loop() {
  static int lastReported = 0;
  if (encoderCount != lastReported) {
    Serial.print("Encoder Count: ");
    Serial.println(encoderCount);
    lastReported = encoderCount;
  }

  delay(10);  // Debounce smoothing
}
