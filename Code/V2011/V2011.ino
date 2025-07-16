#include <AccelStepper.h>

// --- Pin Definitions ---
#define IN1 5         // Stepper motor coil 1
#define IN2 4         // Stepper motor coil 2
#define IN3 2         // Stepper motor coil 3
#define IN4 15        // Stepper motor coil 4
#define HALL_PIN 34   // Hall effect sensor signal pin

// --- Constants ---
#define MOTOR_SPEED 200            // Motor speed in steps per second
#define OFFSET_STEPS 540           // Stepper steps to move past the magnet before zeroing

// --- Globals ---
AccelStepper stepper(AccelStepper::FULL4WIRE, IN1, IN3, IN2, IN4);

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
  }

  Serial.println("Magnet detected. Moving offset...");

  stepper.moveTo(stepper.currentPosition() + OFFSET_STEPS);
  while (stepper.distanceToGo() > 0) {
    stepper.run();
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

  stepper.setMaxSpeed(MOTOR_SPEED);
  stepper.setSpeed(MOTOR_SPEED);
  stepper.setAcceleration(800);

  zeroStepperWithHall();
}

void loop() {
  stepper.runSpeed();
} 
