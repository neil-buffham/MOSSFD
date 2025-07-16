// Simple Rotary Encoder Test with Full Quadrature Decoding
// --------------------------------------------------
// Reads a quadrature rotary encoder using full 2-bit state tracking (Gray code)

// --- Pin Definitions ---
#define ENCODER_A 14  // Brown wire
#define ENCODER_B 12  // Blue/White wire

// --- Globals ---
volatile int encoderCount = 0;        // Current position
volatile uint8_t lastEncoded = 0;     // Last 2-bit encoded state (AB)

// Lookup table to determine direction from previous and current encoder state
const int8_t encoderTable[16] = {
   0, -1,  1,  0,
   1,  0,  0, -1,
  -1,  0,  0,  1,
   0,  1, -1,  0
};

void IRAM_ATTR handleEncoder() {
  bool A = digitalRead(ENCODER_A);
  bool B = digitalRead(ENCODER_B);

  uint8_t encoded = (A << 1) | B;                // Combine A and B into a 2-bit value
  uint8_t combined = (lastEncoded << 2) | encoded; // 4-bit value representing state change

  encoderCount += encoderTable[combined & 0x0F];   // Apply movement from lookup table
  lastEncoded = encoded;                          // Save state for next change
}

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("\n--- Full Quadrature Rotary Encoder Test ---");

  pinMode(ENCODER_A, INPUT_PULLUP);
  pinMode(ENCODER_B, INPUT_PULLUP);

  lastEncoded = (digitalRead(ENCODER_A) << 1) | digitalRead(ENCODER_B); // Initial state
  attachInterrupt(digitalPinToInterrupt(ENCODER_A), handleEncoder, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENCODER_B), handleEncoder, CHANGE);
}

void loop() {
  static int lastReported = 0;
  if (encoderCount != lastReported) {
    Serial.print("Encoder Count: ");
    Serial.println(encoderCount);
    lastReported = encoderCount;
  }

  delay(10);  // Smoothing update interval
}
