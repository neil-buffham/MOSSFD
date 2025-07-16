#include <AccelStepper.h>
#include "flap_index.h"  // Include the flap character index

// --- Pin assignments ---
const int IN1 = 5;    // Stepper coil A1
const int IN2 = 4;    // Stepper coil A2
const int IN3 = 2;    // Stepper coil B1
const int IN4 = 15;   // Stepper coil B2
const int HALL_SENSOR_PIN = 34;  // Hall Effect sensor OUT

const int STEPS_PER_REV = 4096;  // 28BYJ-48 in half-step mode
const float STEPS_PER_DEGREE = STEPS_PER_REV / 360.0;
const int OFFSET_DEGREES = 88;  // Offset from magnet detection position

AccelStepper stepper(AccelStepper::HALF4WIRE, IN1, IN3, IN2, IN4);
bool motorBusy = false;
int magnetPassCount = 0;

void indexToMagnet() {
  Serial.println("Searching for magnet to zero position...");
  stepper.setMaxSpeed(1000);  // Fast speed for zeroing
  stepper.setAcceleration(500);

  // Advance by 1/8 of a rotation before checking for the magnet
  long preOffsetSteps = STEPS_PER_REV / 8;
  stepper.moveTo(preOffsetSteps);
  while (stepper.distanceToGo() > 0) {
    stepper.run();
  }

  stepper.moveTo(stepper.currentPosition() + 100000); // Large move target after offset
  while (digitalRead(HALL_SENSOR_PIN) != LOW && stepper.currentPosition() < 100000 + preOffsetSteps) {
    stepper.run();
  }

  // Move forward by the offset degrees to avoid backward motion
  long offsetSteps = OFFSET_DEGREES * STEPS_PER_DEGREE;
  stepper.moveTo(stepper.currentPosition() + offsetSteps);
  while (stepper.distanceToGo() > 0) {
    stepper.run();
  }

  // Reset position to 0 after offset
  stepper.setCurrentPosition(0);
  magnetPassCount = 0;  // Reset magnet pass counter
  Serial.println("Magnet detected and offset applied. Position zeroed.");
  delay(500);
}

void detectStepsPerRevolution() {
  Serial.println("Measuring full revolution steps...");
  indexToMagnet();

  stepper.setMaxSpeed(200);  // Controlled slow speed
  stepper.setAcceleration(100);

  long startPos = stepper.currentPosition();
  stepper.moveTo(startPos + 100000);  // Arbitrary large move

  bool magnetPassedOnce = false;
  bool waitingForSecond = false;
  long magnetFirstStep = 0;
  long stepsBetween = 0;
  unsigned long magnetCooldown = 0;

  while (stepper.distanceToGo() > 0) {
    stepper.run();

    if (digitalRead(HALL_SENSOR_PIN) == LOW) {
      if (!magnetPassedOnce) {
        magnetFirstStep = stepper.currentPosition();
        magnetPassedOnce = true;
        waitingForSecond = true;
        magnetCooldown = millis();
        Serial.println("First magnet detection recorded.");
      } else if (waitingForSecond && millis() - magnetCooldown > 800) {
        stepsBetween = stepper.currentPosition() - magnetFirstStep;
        Serial.print("Steps between magnet detection / Steps per Revolution: ");
        Serial.println(stepsBetween);
        break;
      }
    }
  }
}

void moveSteps(int steps) {
  Serial.print("Moving forward ");
  Serial.print(steps);
  Serial.println(" steps...");
  motorBusy = true;
  stepper.moveTo(stepper.currentPosition() + steps);
  while (stepper.distanceToGo() != 0) {
    stepper.run();
  }
  Serial.println("Done.");
  motorBusy = false;
}

void goToCharacter(char inputChar) {
  inputChar = toupper(inputChar);
  int targetIndex = -1;
  for (int i = 0; i < NUM_FLAPS; i++) {
    if (flapCharacters[i] == inputChar) {
      targetIndex = i;
      break;
    }
  }

  if (targetIndex == -1) {
    Serial.println("Character not in index.");
    return;
  }
  goToIndex(targetIndex);
}

void goToIndex(int targetIndex) {
  float targetDegrees = (360.0 / NUM_FLAPS) * targetIndex;
  long targetSteps = targetDegrees * STEPS_PER_DEGREE;

  long currentPosition = stepper.currentPosition();
  long currentStepMod = currentPosition % STEPS_PER_REV;
  if (currentStepMod < 0) currentStepMod += STEPS_PER_REV;  // Normalize to 0–4095

  long relativeTarget = targetSteps;
  if (targetSteps <= currentStepMod) {
    relativeTarget += STEPS_PER_REV;  // Always move forward
  }

  long absoluteTarget = currentPosition + (relativeTarget - currentStepMod);

  Serial.println("Moving...");
  motorBusy = true;
  stepper.moveTo(absoluteTarget);
  while (stepper.distanceToGo() != 0) {
    stepper.run();
    if (digitalRead(HALL_SENSOR_PIN) == LOW) {
      static unsigned long lastMagnetTime = 0;
      if (millis() - lastMagnetTime > 500) {
        magnetPassCount++;
        lastMagnetTime = millis();
      }
    }
  }
  Serial.println("Done.");
  motorBusy = false;
}

void printPosition() {
  Serial.print("Current Step Position: ");
  Serial.print(stepper.currentPosition());
  Serial.print(" | Degrees: ");
  Serial.println(stepper.currentPosition() / STEPS_PER_DEGREE);
}

void printIndexList() {
  Serial.println("Character Index List:");
  for (int i = 0; i < NUM_FLAPS; i++) {
    Serial.print(i);
    Serial.print(": ");
    Serial.println(flapCharacters[i]);
  }
}

void printHelp() {
  Serial.println("Available Commands:");
  Serial.println("***ZERO - Re-index using magnet");
  Serial.println("***<number> - Move to flap index by number");
  Serial.println("***POS - Print current position in steps and degrees");
  Serial.println("***LIST - Print character index list");
  Serial.println("***RESET - Set current step to 0");
  Serial.println("***REV - Measure steps between magnet detections (1 revolution)");
  Serial.println("***COUNT - Show number of magnet passes since last zero");
  Serial.println("***MOVE <steps> - Move forward by a number of steps");
  Serial.println("***HELP - Show this help message");
  Serial.println("Type any character to move to that flap.");
}

void printMagnetPassCount() {
  Serial.print("Magnet has passed ");
  Serial.print(magnetPassCount);
  Serial.println(" times since last zeroing.");
}

void setup() {
  pinMode(HALL_SENSOR_PIN, INPUT);
  Serial.begin(115200);
  Serial.println("Starting Stepper Degree Test...");

  indexToMagnet();
}

void loop() {
  if (!motorBusy && Serial.available() > 0) {
    String input = Serial.readStringUntil('\n');
    input.replace('\r', ' ');  // Remove carriage return, preserve spaces

    input.trim();
    if (input.startsWith("***")) {
      String command = input.substring(3);
      command.toUpperCase();

      if (command == "ZERO") {
        indexToMagnet();
      } else if (command == "POS") {
        printPosition();
      } else if (command == "LIST") {
        printIndexList();
      } else if (command == "RESET") {
        stepper.setCurrentPosition(0);
        Serial.println("Stepper position reset to 0.");
      } else if (command == "REV") {
        detectStepsPerRevolution();
      } else if (command == "COUNT") {
        printMagnetPassCount();
      } else if (command.startsWith("MOVE")) {
        int moveStepsVal = command.substring(5).toInt();
        if (moveStepsVal > 0) {
          moveSteps(moveStepsVal);
        } else {
          Serial.println("Invalid step count for MOVE command.");
        }
      } else if (command == "HELP") {
        printHelp();
      } else {
        int index = command.toInt();
        if (index >= 0 && index < NUM_FLAPS) {
          goToIndex(index);
        } else {
          Serial.println("Invalid command or index.");
        }
      }
    } else if (input.length() > 0) {
      char selectedChar = input.charAt(0);
      Serial.print("Character received: [");
      Serial.print(selectedChar);
      Serial.println("]");
      goToCharacter(selectedChar);
    }
  }
}
