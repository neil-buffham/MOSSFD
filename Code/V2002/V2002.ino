#include <AccelStepper.h>

// --- Pin definitions for 28BYJ-48 with ULN2003 ---
#define IN1 5
#define IN2 4
#define IN3 2
#define IN4 15
#define HALL_SENSOR_PIN 34  // A3144 sensor OUT -> GPIO34 (input only on ESP32)

// Create AccelStepper in HALF_STEP mode with 4 control pins
AccelStepper stepper(AccelStepper::HALF4WIRE, IN1, IN3, IN2, IN4);

// Detection configuration
const int requiredConsecutiveLowReads = 8;    // Filter strength: number of LOWs in a row to count as magnet
const int minStepsBetweenMagnets = 1000;       // Minimum steps between magnet detections

// Test speeds (in steps per second)
const int testSpeeds[] = {50, 100, 150, 200, 250};
const int numTestSpeeds = sizeof(testSpeeds) / sizeof(testSpeeds[0]);
const int cyclesPerSpeed = 3;

void setup() {
  Serial.begin(115200);
  pinMode(HALL_SENSOR_PIN, INPUT);

  // Stepper setup
  stepper.setMaxSpeed(500); // Max allowed (used by runSpeed)
  stepper.setAcceleration(100); // Not used by runSpeed
  stepper.setSpeed(100); // Default to safe speed initially

  Serial.println("Stepper + Hall Sensor Filtered Test (V2003) Starting...");
  delay(1000); // Give serial time to initialize
}

void loop() {
  for (int test = 0; test < numTestSpeeds; test++) {
    float speed = testSpeeds[test];
    stepper.setSpeed(speed);

    Serial.println();
    Serial.print("=== Test Set ");
    Serial.print(test + 1);
    Serial.print(" | Speed: ");
    Serial.print(speed, 2);
    Serial.println(" steps/sec ===");

    for (int cycle = 1; cycle <= cyclesPerSpeed; cycle++) {
      int stepsTaken = 0;
      int consecutiveLowReads = 0;
      bool magnetDetected = false;

      while (!magnetDetected) {
        stepper.runSpeed();
        stepsTaken++;

        int hallValue = digitalRead(HALL_SENSOR_PIN);
        if (hallValue == LOW) {
          consecutiveLowReads++;
        } else {
          consecutiveLowReads = 0;
        }

        if (consecutiveLowReads >= requiredConsecutiveLowReads && stepsTaken >= minStepsBetweenMagnets) {
          magnetDetected = true;
        }

        delayMicroseconds(500);  // Small delay for stability
      }

      Serial.print("Cycle ");
      Serial.print(cycle);
      Serial.print(": Magnet found after ");
      Serial.print(stepsTaken);
      Serial.println(" steps.");
      delay(500); // Pause before next cycle
    }
  }

  Serial.println("\nAll tests complete. Motor stopping.");
  while (true); // Halt
}
