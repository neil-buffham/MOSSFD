#include "SplitFlapMotor.h"
#include "flap_index.h"

SplitFlapMotor::SplitFlapMotor(int in1, int in2, int in3, int in4, int hallPin)
  : stepper(AccelStepper::HALF4WIRE, in1, in3, in2, in4),
    hallSensorPin(hallPin),
    zeroOffsetDegrees(91),
    stepOffset(0),
    magnetPassCount(0),
    motorBusy(false) {}

void SplitFlapMotor::begin() {
  pinMode(hallSensorPin, INPUT);
  stepper.setMaxSpeed(800);
  stepper.setAcceleration(400);
}

String SplitFlapMotor::getModuleID() {
  uint64_t chipid = ESP.getEfuseMac();
  char id[13];
  sprintf(id, "%02X%02X%02X%02X%02X%02X",
          (uint8_t)(chipid >> 40), (uint8_t)(chipid >> 32), (uint8_t)(chipid >> 24),
          (uint8_t)(chipid >> 16), (uint8_t)(chipid >> 8), (uint8_t)chipid);
  return String(id);
}

void SplitFlapMotor::indexToMagnet() {
  Serial.println("Searching for magnet to zero position...");

  bool magnetSeen = false;
  long startPos = stepper.currentPosition();
  long maxSteps = 4096 + (4096 / 8);
  stepper.moveTo(startPos + maxSteps);

  while (stepper.distanceToGo() > 0) {
    stepper.run();
    if (digitalRead(hallSensorPin) == LOW) {
      if (!magnetSeen) {
        magnetSeen = true;
        delay(5);
      } else {
        stepper.stop();
        break;
      }
    } else {
      magnetSeen = false;
    }
  }

  long offsetSteps = zeroOffsetDegrees * (4096.0 / 360.0);
  stepper.moveTo(stepper.currentPosition() + offsetSteps);
  while (stepper.distanceToGo() > 0) stepper.run();

  stepper.setCurrentPosition(0);
  magnetPassCount = 0;
  Serial.println("Magnet detected and offset applied. Position zeroed.");
  delay(500);
}

void SplitFlapMotor::detectStepsPerRevolution() {
  Serial.println("Measuring full revolution steps...");
  indexToMagnet();
  stepper.setMaxSpeed(200);
  stepper.setAcceleration(100);
  stepper.moveTo(stepper.currentPosition() + 100000);

  bool magnetPassedOnce = false;
  long magnetFirstStep = 0;
  long stepsBetween = 0;
  unsigned long magnetCooldown = 0;

  while (stepper.distanceToGo() > 0) {
    stepper.run();
    if (digitalRead(hallSensorPin) == LOW) {
      if (!magnetPassedOnce) {
        magnetFirstStep = stepper.currentPosition();
        magnetPassedOnce = true;
        magnetCooldown = millis();
        Serial.println("First magnet detection recorded.");
      } else if (millis() - magnetCooldown > 800) {
        stepsBetween = stepper.currentPosition() - magnetFirstStep;
        Serial.print("Steps per Revolution: ");
        Serial.println(stepsBetween);
        break;
      }
    }
  }
}

void SplitFlapMotor::setZeroOffsetDegrees(long offset) {
  Serial.println("-------------------------------------------");
  Serial.print("Previous Zero Offset (degrees): ");
  Serial.println(zeroOffsetDegrees);
  zeroOffsetDegrees = offset;
  Serial.print("New Zero Offset (degrees): ");
  Serial.println(zeroOffsetDegrees);
}

void SplitFlapMotor::setZeroOffsetDegreesInteractive() {
  if (motorBusy) {
    Serial.println("Cannot change zero offset while motor is moving.");
    return;
  }

  Serial.println("-------------------------------------------");
  Serial.print("Current Zero Offset (degrees): ");
  Serial.println(zeroOffsetDegrees);
  Serial.println("Change zero offset? (y/n):");

  while (Serial.available() == 0);
  char response = Serial.read();
  while (Serial.available()) Serial.read();

  if (response == 'y' || response == 'Y') {
    Serial.println("Enter new zero offset (in degrees, integer):");
    while (Serial.available() == 0);
    String line = Serial.readStringUntil('\n');
    line.trim();
    zeroOffsetDegrees = line.toInt();
    Serial.print("New Zero Offset (degrees) set to: ");
    Serial.println(zeroOffsetDegrees);
  } else {
    Serial.println("No changes made.");
  }
}

void SplitFlapMotor::setStepOffset(long offset) {
  Serial.println("-------------------------------------------");
  Serial.print("Previous Offset: ");
  Serial.println(stepOffset);
  stepOffset = offset;
  Serial.print("New Offset: ");
  Serial.println(stepOffset);
}

void SplitFlapMotor::setStepOffsetInteractive() {
  Serial.println("-------------------------------------------");
  Serial.print("Current Step Offset: ");
  Serial.println(stepOffset);
  Serial.println("Change offset? (y/n): ");

  while (Serial.available() == 0);
  char response = Serial.read();

  if (response == 'y' || response == 'Y') {
    Serial.println("Enter new offset (integer): ");
    while (Serial.available() == 0);
    stepOffset = Serial.readStringUntil('\n').toInt();
    Serial.print("New Step Offset set to: ");
    Serial.println(stepOffset);
  } else {
    Serial.println("No changes made.");
  }
}

void SplitFlapMotor::moveSteps(int steps) {
  Serial.print("Moving forward ");
  Serial.print(steps);
  Serial.println(" steps...");
  motorBusy = true;
  stepper.moveTo(stepper.currentPosition() + steps);
  while (stepper.distanceToGo() != 0) stepper.run();
  Serial.println("Done.");
  motorBusy = false;
}

void SplitFlapMotor::goToCharacter(char inputChar) {
  inputChar = toupper(inputChar);
  int targetIndex = getCharacterIndex(inputChar);
  if (targetIndex == -1) {
    Serial.println("Character not in index.");
    return;
  }
  goToIndex(targetIndex);
}

void SplitFlapMotor::goToIndex(int targetIndex) {
  long targetSteps = flapSteps[targetIndex] + stepOffset;
  long currentPosition = stepper.currentPosition();
  long currentStepMod = currentPosition % 4096;
  if (currentStepMod < 0) currentStepMod += 4096;

  long relativeTarget = targetSteps;
  if (targetSteps <= currentStepMod) relativeTarget += 4096;
  long absoluteTarget = currentPosition + (relativeTarget - currentStepMod);

  Serial.print("Moving to index ");
  Serial.print(targetIndex);
  Serial.print(" (Character: '");
  Serial.print(flapCharacters[targetIndex]);
  Serial.print("')... ");
  motorBusy = true;
  stepper.moveTo(absoluteTarget);

  while (stepper.distanceToGo() != 0) {
    stepper.run();
    if (digitalRead(hallSensorPin) == LOW) {
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

int SplitFlapMotor::getCharacterIndex(char inputChar) {
  for (int i = 0; i < NUM_FLAPS; i++) {
    if (flapCharacters[i] == inputChar) {
      return i;
    }
  }
  return -1;
}

void SplitFlapMotor::printPosition() {
  Serial.println("-------------------------------------------");
  Serial.print("Current Step Position: ");
  Serial.print(stepper.currentPosition());
  Serial.print(" | Degrees: ");
  Serial.println(stepper.currentPosition() / (4096.0 / 360.0));
}

void SplitFlapMotor::printIndexList() {
  Serial.println("-------------------------------------------");
  Serial.println("Character Index List:");
  for (int i = 0; i < NUM_FLAPS; i++) {
    Serial.print(i);
    Serial.print(": ");
    Serial.println(flapCharacters[i]);
  }
}

void SplitFlapMotor::printHelp() {
  Serial.println("-------------------------------------------");
  Serial.println("Available Commands:");
  Serial.println("***Info      - Print current information");
  Serial.println("***ZERO      - Re-index using magnet");
  Serial.println("***<number>  - Move to flap index by number");
  Serial.println("***POS       - Print current position in steps and degrees");
  Serial.println("***LIST      - Print character index list");
  Serial.println("***RESET     - Simulate a reset");
  Serial.println("***REV       - Measure steps in one revolution");
  Serial.println("***COUNT     - Show magnet pass count");
  Serial.println("***MOVE      - Move forward by N steps");
  Serial.println("***ZOFFSET   - Set magnet-to-flap offset in degrees");
  Serial.println("***OFFSET    - Apply global step offset");
  Serial.println("***HELP      - Show this help message");
  Serial.println("\nType any single character to move to that flap.");
}

void SplitFlapMotor::printMagnetPassCount() {
  Serial.println("-------------------------------------------");
  Serial.print("Magnet has passed ");
  Serial.print(magnetPassCount);
  Serial.println(" times since last zeroing.");
}

void SplitFlapMotor::printInfo() {
  Serial.println("-------------------------------------------");
  Serial.print("Current Step Position: ");
  Serial.print(stepper.currentPosition());
  Serial.print(" | Degrees: ");
  Serial.println(stepper.currentPosition() / (4096.0 / 360.0));
  Serial.print("Magnet has passed ");
  Serial.print(magnetPassCount);
  Serial.println(" times since Zeroing");
  Serial.print("Zero Offset (Degrees): ");
  Serial.println(zeroOffsetDegrees);
  Serial.print("Step Offset (Steps): ");
  Serial.println(stepOffset);
}

bool SplitFlapMotor::isMotorBusy() {
  return motorBusy;
}
