/* This code works to index the drum based on the rotary encoder, and it is somewhat precise
but not very accurate. It keeps returning to the same flaps, but the flaps are not by each other
and it doesn't make much sense. Apparently we are only using half of the rotary encoder resolution,
so v2011 will move to full resolution instead of half.*/



#include <AccelStepper.h>

// --- Pin Definitions ---
#define IN1 5         // Stepper motor coil 1
#define IN2 4         // Stepper motor coil 2
#define IN3 2         // Stepper motor coil 3
#define IN4 15        // Stepper motor coil 4
#define ENCODER_A 14  // Rotary encoder signal A (Brown wire)
#define ENCODER_B 12  // Rotary encoder signal B (Blue/White wire)
#define HALL_PIN 34   // Hall effect sensor signal pin

// --- Constants ---
#define MOTOR_SPEED 200            // Motor speed in steps per second
#define OFFSET_COUNT 215           // Encoder count to move past the magnet before setting zero

// --- Globals ---
AccelStepper stepper(AccelStepper::FULL4WIRE, IN1, IN3, IN2, IN4);
volatile int encoderCount = 0;     // Tracks rotary encoder position
volatile bool directionCW = true;  // Tracks encoder rotation direction

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

// Zero the encoder when the magnet is detected, then move to a defined offset
void zeroEncoderAndOffset() {
  Serial.println("Searching for magnet to zero position using encoder...");

  // If magnet is already detected at startup, move forward to clear it
  if (digitalRead(HALL_PIN) == LOW) {
    Serial.println("Magnet detected at startup — moving past it to begin search...");
    stepper.moveTo(512);  // Move forward ~1/8 turn
    while (stepper.distanceToGo() > 0) {
      stepper.run();
    }
  }

  // Set motor to run at a steady speed
  stepper.setSpeed(MOTOR_SPEED);
  bool magnetDetected = false;

  // Continue rotating until hall sensor detects magnet
  while (!magnetDetected) {
    stepper.runSpeed();
    if (digitalRead(HALL_PIN) == LOW) {
      magnetDetected = true;
      Serial.print("Magnet detected at encoder count: ");
      Serial.println(encoderCount);
      break;
    }
  }

  // Move further by offset value using encoder readings
  long startEncoder = encoderCount;
  long targetEncoder = startEncoder + OFFSET_COUNT;
  Serial.print("Moving to offset target count: ");
  Serial.println(targetEncoder);

  while (encoderCount < targetEncoder) {
    stepper.runSpeed();
  }

  // Zero encoder and stepper
  encoderCount = 0;
  stepper.setCurrentPosition(0);
  Serial.println("Offset applied and encoder zeroed.");
  delay(500);
}

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\n--------------------------------------------");
  Serial.println("Starting stepper + encoder + hall tracking...");

  // Set up encoder pins with pull-ups and attach interrupt
  pinMode(ENCODER_A, INPUT_PULLUP);
  pinMode(ENCODER_B, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(ENCODER_A), handleEncoder, CHANGE);

  // Set up Hall effect sensor pin
  pinMode(HALL_PIN, INPUT);

  // Configure stepper motor
  stepper.setMaxSpeed(MOTOR_SPEED);
  stepper.setSpeed(MOTOR_SPEED);
  stepper.setAcceleration(800);

  // Perform initial zeroing
  zeroEncoderAndOffset();
}

void loop() {
  // Loop left empty for now to isolate and troubleshoot zeroing
}
