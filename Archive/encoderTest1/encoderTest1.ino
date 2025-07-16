const int pinA = 14;  // GPIO14 for Channel A
const int pinB = 12;  // GPIO12 for Channel B

volatile int encoderPos = 0;
int lastEncoded = 0;

void IRAM_ATTR updateEncoder() {
  int MSB = digitalRead(pinA); // Most Significant Bit
  int LSB = digitalRead(pinB); // Least Significant Bit

  int encoded = (MSB << 1) | LSB;       // Combine the two bits
  int sum = (lastEncoded << 2) | encoded;

  if (sum == 0b1101 || sum == 0b0100 || sum == 0b0010 || sum == 0b1011) encoderPos++;
  if (sum == 0b1110 || sum == 0b0111 || sum == 0b0001 || sum == 0b1000) encoderPos--;

  lastEncoded = encoded;
}

void setup() {
  Serial.begin(115200);
  pinMode(pinA, INPUT_PULLUP);
  pinMode(pinB, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(pinA), updateEncoder, CHANGE);
  attachInterrupt(digitalPinToInterrupt(pinB), updateEncoder, CHANGE);

  Serial.println("Rotary Encoder Test Starting...");
}

void loop() {
  static int lastReportedPos = 0;

  if (encoderPos != lastReportedPos) {
    Serial.print("Encoder Position: ");
    Serial.println(encoderPos);
    lastReportedPos = encoderPos;
  }

  delay(10); // Debounce
}
void setup() {
  // put your setup code here, to run once:

}

void loop() {
  // put your main code here, to run repeatedly:

}
