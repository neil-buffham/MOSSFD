/*
===============================================================================
Split Flap Display Firmware - Version V2008
===============================================================================

Changelog from V2007:

Functionality:
- added non-volatile storage of the two offset values
  - added SFConfig.h
  - added SFConfig.cpp

File Layout:
- V2007.ino - Main sketch file (handles setup, loop, and command parsing)
- SplitFlapMotor.h / .cpp - Library encapsulating all motor and index logic
- flap_index.h - Unchanged: defines character/index mapping
- SFConfig.h
- SFConfig.cpp 
===============================================================================
*/




#include <WiFi.h>
#include "SplitFlapMotor.h"
#include "flap_index.h"

const char* firmwareVersion = "V2008"; //only used in initial print message

const int IN1 = 5, IN2 = 4, IN3 = 2, IN4 = 15;
const int HALL_PIN = 34;

SplitFlapMotor flap(IN1, IN2, IN3, IN4, HALL_PIN);

void setup() {
  Serial.begin(115200);
  flap.begin();
  String id = flap.getModuleID();
  Serial.println("***************************************************************");
  Serial.println("Split Flap Powered on - module: " + id);
  Serial.print("Firmware Version: ");
  Serial.println(firmwareVersion);
  flap.indexToMagnet();
  flap.printInfo();
  Serial.println("--------------------------Setup complete-----------------------");
}

void loop() {
  if (!flap.isMotorBusy() && Serial.available() > 0) {
    String input = Serial.readStringUntil('\n');
    input.replace('\r', ' ');

    if (input.startsWith("***")) {
      String command = input.substring(3);
      command.trim();
      command.toUpperCase();

      if (command == "ZERO") flap.indexToMagnet();
      else if (command == "POS") flap.printPosition();
      else if (command == "LIST") flap.printIndexList();
      else if (command == "RESET") ESP.restart();
      else if (command == "REV") flap.detectStepsPerRevolution();
      else if (command == "COUNT") flap.printMagnetPassCount();
      else if (command == "INFO") flap.printInfo();
      else if (command == "RESETCONFIG") flap.resetConfig();
      else if (command.startsWith("MOVE")) {
        int moveStepsVal = command.substring(4).toInt();
        if (moveStepsVal > 0) flap.moveSteps(moveStepsVal);
        else Serial.println("Invalid step count for MOVE command.");
      } else if (command.startsWith("OFFSET")) {
        String arg = command.substring(6);
        arg.trim();
        if (arg.length() == 0) flap.setStepOffsetInteractive();
        else flap.setStepOffset(arg.toInt());
      } else if (command.startsWith("ZOFFSET")) {
        String arg = command.substring(7);
        arg.trim();
        if (arg.length() == 0) flap.setZeroOffsetDegreesInteractive();
        else flap.setZeroOffsetDegrees(arg.toInt());
      } else if (command == "HELP") flap.printHelp();
      else {
        int index = command.toInt();
        if (index >= 0 && index < NUM_FLAPS) flap.goToIndex(index);
        else Serial.println("Invalid command or index.");
      }
    } else if (input.length() > 0) {
      flap.goToCharacter(input.charAt(0));
    }
  }
}
