/*
Version V2006.3 - Iterated from V2005 after resolving key motor logic issues.

Changes in V2006:
- Added MAC address detection to uniquely identify modules
- Moved space character to front of flap index
- Fixed input trimming issue that rejected spaces
- Improved zeroing motion for smoothness and magnet re-detection protection
*/

#include <WiFi.h>
#include <AccelStepper.h>
#include "flap_index.h"  // Character-to-index lookup table

// ------------------------ Pin Assignments ------------------------
const int IN1 = 5;     // Stepper motor coil A1
const int IN2 = 4;     // Stepper motor coil A2
const int IN3 = 2;     // Stepper motor coil B1
const int IN4 = 15;    // Stepper motor coil B2
const int HALL_SENSOR_PIN = 34;  // Hall effect sensor for magnet detection

// ------------------------ Motor Configuration ------------------------
const int STEPS_PER_REV = 4096;                     // Half-step mode for 28BYJ-48
const float STEPS_PER_DEGREE = STEPS_PER_REV / 360.0;
long zeroOffsetDegrees = 91;                        // Distance from magnet to first flap
long stepOffset = 0;                                // Offset applied to all flap positions

// ------------------------ State Tracking ------------------------
String fw_Version = "V2006.3";                      // Firmware version
String module_ID;                                   // ESP32 unique identifier
AccelStepper stepper(AccelStepper::HALF4WIRE, IN1, IN3, IN2, IN4);
bool motorBusy = false;                             // Flag for motor activity
int magnetPassCount = 0;                            // Count since last zero

// ------------------------ Core Functions ------------------------

// Generate unique module ID from MAC address
String getModuleID() {
  uint64_t chipid = ESP.getEfuseMac();
  char id[13];
  sprintf(id, "%02X%02X%02X%02X%02X%02X",
          (uint8_t)(chipid >> 40), (uint8_t)(chipid >> 32), (uint8_t)(chipid >> 24),
          (uint8_t)(chipid >> 16), (uint8_t)(chipid >> 8), (uint8_t)chipid);
  return String(id);
}

// Search for magnet and set position 0 with an offset to first flap
void indexToMagnet() {
  Serial.println("Searching for magnet to zero position...");
  stepper.setMaxSpeed(800);
  stepper.setAcceleration(400);

  bool magnetSeen = false;
  long startPos = stepper.currentPosition();
  long maxSteps = STEPS_PER_REV + (STEPS_PER_REV / 8);  // 1.125 rotations

  stepper.moveTo(startPos + maxSteps);
  while (stepper.distanceToGo() > 0) {
    stepper.run();

    if (digitalRead(HALL_SENSOR_PIN) == LOW) {
      if (!magnetSeen) {
        magnetSeen = true;
        delay(5);  // Debounce
      } else {
        stepper.stop();  // Second consecutive LOW confirms detection
        break;
      }
    } else {
      magnetSeen = false;
    }
  }

  // Move forward to flap 0 position
  long offsetSteps = zeroOffsetDegrees * STEPS_PER_DEGREE;
  stepper.moveTo(stepper.currentPosition() + offsetSteps);
  while (stepper.distanceToGo() > 0) stepper.run();

  stepper.setCurrentPosition(0);
  magnetPassCount = 0;
  Serial.println("Magnet detected and offset applied. Position zeroed.");
  delay(500);
}

// Measure full rotation by detecting two magnet passes
void detectStepsPerRevolution() {
  Serial.println("Measuring full revolution steps...");
  indexToMagnet();

  stepper.setMaxSpeed(200);
  stepper.setAcceleration(100);

  stepper.moveTo(stepper.currentPosition() + 100000);  // Long forward spin

  bool magnetPassedOnce = false;
  long magnetFirstStep = 0;
  long stepsBetween = 0;
  unsigned long magnetCooldown = 0;

  while (stepper.distanceToGo() > 0) {
    stepper.run();
    if (digitalRead(HALL_SENSOR_PIN) == LOW) {
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

// Set zeroing offset (degrees) from code
void setZeroOffsetDegrees(long offset) {
  Serial.println("-------------------------------------------");
  Serial.print("Previous Zero Offset (degrees): ");
  Serial.println(zeroOffsetDegrees);
  zeroOffsetDegrees = offset;
  Serial.print("New Zero Offset (degrees): ");
  Serial.println(zeroOffsetDegrees);
}

// Interactive prompt for zeroing offset
void setZeroOffsetDegreesInteractive() {
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
  while (Serial.available()) Serial.read();  // Clear buffer

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

// Set global step offset directly
void setStepOffset(long offset) {
  Serial.println("-------------------------------------------");
  Serial.print("Previous Offset: ");
  Serial.println(stepOffset);
  stepOffset = offset;
  Serial.print("New Offset: ");
  Serial.println(stepOffset);
}

// Interactive prompt for setting step offset
void setStepOffsetInteractive() {
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

// Move motor by exact number of steps
void moveSteps(int steps) {
  Serial.print("Moving forward ");
  Serial.print(steps);
  Serial.println(" steps...");
  motorBusy = true;
  stepper.moveTo(stepper.currentPosition() + steps);
  while (stepper.distanceToGo() != 0) stepper.run();
  Serial.println("Done.");
  motorBusy = false;
}

// Move to flap that matches input character
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

// Move to specific index, accounting for wraparound and offset
void goToIndex(int targetIndex) {
  long targetSteps = flapSteps[targetIndex] + stepOffset;
  long currentPosition = stepper.currentPosition();
  long currentStepMod = currentPosition % STEPS_PER_REV;
  if (currentStepMod < 0) currentStepMod += STEPS_PER_REV;

  long relativeTarget = targetSteps;
  if (targetSteps <= currentStepMod) relativeTarget += STEPS_PER_REV;
  long absoluteTarget = currentPosition + (relativeTarget - currentStepMod);

  Serial.print("Moving to index ");
  Serial.print(targetIndex);
  Serial.print(" (Character: '");
  Serial.print(flapCharacters[targetIndex]);
  Serial.println("')...");
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

// ------------------------ Printing Utilities ------------------------

void printPosition() {
  Serial.println("-------------------------------------------");
  Serial.print("Current Step Position: ");
  Serial.print(stepper.currentPosition());
  Serial.print(" | Degrees: ");
  Serial.println(stepper.currentPosition() / STEPS_PER_DEGREE);
}

void printIndexList() {
  Serial.println("-------------------------------------------");
  Serial.println("Character Index List:");
  for (int i = 0; i < NUM_FLAPS; i++) {
    Serial.print(i);
    Serial.print(": ");
    Serial.println(flapCharacters[i]);
  }
}

void printHelp() {
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

void printMagnetPassCount() {
  Serial.println("-------------------------------------------");
  Serial.print("Magnet has passed ");
  Serial.print(magnetPassCount);
  Serial.println(" times since last zeroing.");
}

void printInfo() {
  Serial.println("-------------------------------------------");
  Serial.print("Current Step Position: ");
  Serial.print(stepper.currentPosition());
  Serial.print(" | Degrees: ");
  Serial.println(stepper.currentPosition() / STEPS_PER_DEGREE);
  Serial.print("Magnet has passed ");
  Serial.print(magnetPassCount);
  Serial.println(" times since Zeroing");
  Serial.print("Zero Offset (Degrees): ");
  Serial.println(zeroOffsetDegrees);
  Serial.print("Step Offset (Steps): ");
  Serial.println(stepOffset);
}

// ------------------------ Setup & Loop ------------------------

void setup() {
  pinMode(HALL_SENSOR_PIN, INPUT);
  Serial.begin(115200);
  module_ID = getModuleID();
  Serial.println("-------------------------------------------------------");
  Serial.println("Split Flap Powered on - module: " + module_ID);
  Serial.println("Firmware version: " + fw_Version);
  Serial.println("Beginning initial zero-ing...");
  indexToMagnet();
  printInfo();
  Serial.println("----------Initialization complete----------------------");
}

void loop() {
  if (!motorBusy && Serial.available() > 0) {
    String input = Serial.readStringUntil('\n');
    input.replace('\r', ' ');  // Preserve spacing

    if (input.startsWith("***")) {
      String command = input.substring(3);
      command.toUpperCase();

      if (command == "ZERO") indexToMagnet();
      else if (command == "POS") printPosition();
      else if (command == "LIST") printIndexList();
      else if (command == "RESET") ESP.restart();
      else if (command == "REV") detectStepsPerRevolution();
      else if (command == "COUNT") printMagnetPassCount();
      else if (command == "INFO") printInfo();
      else if (command.startsWith("MOVE")) {
        int moveStepsVal = command.substring(5).toInt();
        if (moveStepsVal > 0) moveSteps(moveStepsVal);
        else Serial.println("Invalid step count for MOVE command.");
      } else if (command.startsWith("OFFSET")) {
        String arg = command.substring(6);
        arg.trim();
        if (arg.length() == 0) setStepOffsetInteractive();
        else setStepOffset(arg.toInt());
      } else if (command.startsWith("ZOFFSET")) {
        String arg = command.substring(7);
        arg.trim();
        if (arg.length() == 0) setZeroOffsetDegreesInteractive();
        else setZeroOffsetDegrees(arg.toInt());
      } else {
        int index = command.toInt();
        if (index >= 0 && index < NUM_FLAPS) goToIndex(index);
        else Serial.println("Invalid command or index.");
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
