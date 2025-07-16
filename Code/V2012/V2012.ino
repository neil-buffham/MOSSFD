/* This code iterates on V2011, which just ended up giving up and going back to 
step-driven zeroing, but adds the rotary encoder back in just as a spectator. */

#include <AccelStepper.h>

// --- Pin Definitions ---
#define IN1 5         // Stepper motor coil 1
#define IN2 4         // Stepper motor coil 2
#define IN3 2         // Stepper motor coil 3
#define IN4 15        // Stepper motor coil 4
#define HALL_PIN 34   // Hall effect sensor signal pin
#define ENCODER_A 14  // Rotary encoder signal A (Brown wire)
#define ENCODER_B 12  // Rotary encoder signal B (Blue/White wire)

// --- Constants ---
#define MOTOR_SPEED 200            // Motor speed in steps per second
#define OFFSET_STEPS 540           // Stepper steps to move past the magnet before zeroing

// --- Globals ---
AccelStepper stepper(AccelStepper::FULL4WIRE, IN1, IN3, IN2, IN4);
volatile int encoderCount = 0;
volatile bool directionCW = true;

// Interrupt handler for rotary encoder
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

void zeroStepperWithHall() {
  Serial.println("Searching for magnet using stepper...");

  // If magnet is already detected at startup, move forward to clear it
  if (digitalRead(HALL_PIN) == LOW) {
    Serial.println("Magnet detected at startup — moving past it to begin search...");
    stepper.moveTo(512);  // Move forward ~1/8 turn
    while (stepper.distanceToGo() > 0) stepper.run();
  }

  stepper.setSpeed(MOTOR_SPEED);
  while (digitalRead(HALL_PIN) != LOW) {
    stepper.runSpeed();
    Serial.print("Encoder: ");
    Serial.println(encoderCount);
  }

  Serial.print("Magnet detected at encoder count: ");
  Serial.println(encoderCount);
  encoderCount = 0;  // Zero the encoder count as well

  Serial.println("Magnet detected. Moving offset...");

  stepper.moveTo(stepper.currentPosition() + OFFSET_STEPS);
  while (stepper.distanceToGo() > 0) {
    stepper.run();
    Serial.print("Encoder: ");
    Serial.println(encoderCount);
  }

  stepper.setCurrentPosition(0);
  Serial.println("Offset applied and position zeroed.");
  delay(500);
}

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\n--------------------------------------------");
  Serial.println("Starting stepper + hall zeroing...");

  pinMode(HALL_PIN, INPUT);
  pinMode(ENCODER_A, INPUT_PULLUP);
  pinMode(ENCODER_B, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(ENCODER_A), handleEncoder, CHANGE);

  stepper.setMaxSpeed(MOTOR_SPEED);
  stepper.setSpeed(MOTOR_SPEED);
  stepper.setAcceleration(800);

  zeroStepperWithHall();
}

void loop() {
  stepper.runSpeed();
}
