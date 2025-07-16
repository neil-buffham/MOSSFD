#include <AccelStepper.h>

// --- Pin Definitions ---
#define IN1 5
#define IN2 4
#define IN3 2
#define IN4 15

#define ENCODER_A 14  // Signal A (Brown)
#define ENCODER_B 12  // Signal B (Blue/White)
#define HALL_PIN 34

// --- Constants ---
#define MOTOR_SPEED 400  // steps per second
#define MAGNET_DEBOUNCE_DELAY 300  // ms

// --- Globals ---
AccelStepper stepper(AccelStepper::FULL4WIRE, IN1, IN3, IN2, IN4);
volatile int encoderCount = 0;
volatile bool directionCW = true;
bool magnetLatched = false;
unsigned long lastMagnetTime = 0;

void IRAM_ATTR handleEncoder() {
  bool A = digitalRead(ENCODER_A);
  bool B = digitalRead(ENCODER_B);

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
  delay(500);
  Serial.println("Starting stepper + encoder + hall tracking...");

  stepper.setMaxSpeed(MOTOR_SPEED);
  stepper.setSpeed(MOTOR_SPEED);  // Positive = clockwise

  pinMode(ENCODER_A, INPUT_PULLUP);
  pinMode(ENCODER_B, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(ENCODER_A), handleEncoder, CHANGE);

  pinMode(HALL_PIN, INPUT);  // Assumes external pull-up resistor
}

void loop() {
  stepper.runSpeed();

  static int lastCount = 0;
  if (encoderCount != lastCount) {
    lastCount = encoderCount;
    Serial.print("Direction: ");
    Serial.print(directionCW ? "CW  " : "CCW ");
    Serial.print(" | Count: ");
    Serial.println(encoderCount);
  }

  bool magnetDetected = digitalRead(HALL_PIN) == LOW;
  unsigned long now = millis();

  if (magnetDetected && !magnetLatched) {
    Serial.println("Magnet detected. Zeroing encoder.");
    encoderCount = 0;
    magnetLatched = true;
    lastMagnetTime = now;
  }

  // Release latch after debounce period and magnet no longer detected
  if (!magnetDetected && magnetLatched && (now - lastMagnetTime) > MAGNET_DEBOUNCE_DELAY) {
    magnetLatched = false;
  }

  delay(1);  // Print rate control and light CPU usage
}
