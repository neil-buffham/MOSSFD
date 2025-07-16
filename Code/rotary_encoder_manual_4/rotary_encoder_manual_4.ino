// Simple Rotary Encoder Test using Single-Channel Debounced Edge Detection
// ------------------------------------------------------------
// This version tracks only rising edges on ENCODER_A with debounce.
// Direction is not tracked; only incremental count is used.

#define ENCODER_A 14  // Brown wire

volatile long encoderCount = 0;           // Rotary encoder count
volatile unsigned long lastInterruptTime = 0;
const unsigned long debounceMicros = 1000;  // 1 ms debounce

void IRAM_ATTR handleEncoder() {
  unsigned long now = micros();
  if (now - lastInterruptTime < debounceMicros) return;
  lastInterruptTime = now;

  if (digitalRead(ENCODER_A) == HIGH) {
    encoderCount++;
  }
}

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("\n--- Simple Rotary Encoder Test ---");

  pinMode(ENCODER_A, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(ENCODER_A), handleEncoder, RISING);
}

void loop() {
  static long lastReported = 0;
  if (encoderCount != lastReported) {
    Serial.print("Encoder Count: ");
    Serial.println(encoderCount);
    lastReported = encoderCount;
  }
  delay(10);
}
