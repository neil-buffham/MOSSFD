// =============================================================
// Split-Flap Display Firmware - V2009-2_Master
//
// This firmware runs on an ESP32 and controls a single module
// of a modular split-flap display. It supports:
//   - Direct ***COMMAND input via the Arduino IDE Serial Monitor
//   - Multi-unit coordination via paragraph messages (****[...])
//
// The command handler supports commands like ***RESET, ***ZERO,
// ***HELP, and character inputs. All command parsing has been
// centralized in a shared `handleCommand()` function to ensure
// consistent behavior across both input modes.
// =============================================================

// example paragraph_message:
// ****[{14146C1C4278}|14146C1C4278:0:0|E0F76B1C4278:1:0|0445470B65F4:2:0|F893963B015C:3:0|848A6C1C4278:4:0|2C1A6D1C4278:5:0|9CDC550B65F4:6:0|5CB469BF1388:7:0|1CF861BF1388:8:0]



#include <WiFi.h>
#include "SplitFlapMotor.h"
#include "flap_index.h"

const char* firmwareVersion = "V2009-2_Master";

const int IN1 = 5, IN2 = 4, IN3 = 2, IN4 = 15;
const int HALL_PIN = 34;

SplitFlapMotor flap(IN1, IN2, IN3, IN4, HALL_PIN);

// Message state
String paragraph_message = "";
bool paragraph_message_received = false;
bool paragraph_message_sent = false;
bool paragraph_message_processed = false;
bool ready_for_paragraph_message = false;
bool paragraph_message_logged = false;

String current_command = "";
int current_command_status = 1;

// Shared command execution function
bool handleCommand(String input) {
  input.replace('/n', ' ');
  bool commandHandled = false;

  if (input.startsWith("***")) {
    String command = input.substring(3);

    if (command == "ZERO") {
      flap.indexToMagnet();
      commandHandled = true;
    } else if (command == "POS") {
      flap.printPosition();
      commandHandled = true;
    } else if (command == "LIST") {
      flap.printIndexList();
      commandHandled = true;
    } else if (command == "RESET") {
      ESP.restart();
      commandHandled = true;
    } else if (command == "REV") {
      flap.detectStepsPerRevolution();
      commandHandled = true;
    } else if (command == "COUNT") {
      flap.printMagnetPassCount();
      commandHandled = true;
    } else if (command == "INFO") {
      flap.printInfo();
      commandHandled = true;
    } else if (command == "RESETCONFIG") {
      flap.resetConfig();
      commandHandled = true;
    } else if (command.startsWith("MOVE")) {
      int moveStepsVal = command.substring(4).toInt();
      if (moveStepsVal > 0) {
        flap.moveSteps(moveStepsVal);
        commandHandled = true;
      } else {
        Serial.println("Invalid step count for MOVE command.");
      }
    } else if (command.startsWith("OFFSET")) {
      String arg = command.substring(6);
      if (arg.length() == 0) flap.setStepOffsetInteractive();
      else flap.setStepOffset(arg.toInt());
      commandHandled = true;
    } else if (command.startsWith("ZOFFSET")) {
      String arg = command.substring(7);
      if (arg.length() == 0) flap.setZeroOffsetDegreesInteractive();
      else flap.setZeroOffsetDegrees(arg.toInt());
      commandHandled = true;
    } else if (command == "HELP") {
      flap.printHelp();
      commandHandled = true;
    } else {
      Serial.print("ERROR: Unknown command keyword: ");
      Serial.println(command);
    }
  } else if (input.length() == 1) {
    flap.goToCharacter(input.charAt(0));
    commandHandled = true;
  } else {
    Serial.print("ERROR: Unknown command format: ");
    Serial.println(input);
  }

  return commandHandled;
}

void setup() {
  Serial.begin(115200);  // USB serial (IDE)
  // Serial1.begin removed to disable listening to LEFT
  Serial2.begin(9600, SERIAL_8N1, -1, 19);  // Right Comm TX on GPIO19
  flap.begin();

  String id = flap.getModuleID();
  Serial.println("***************************************************************");
  Serial.println("Split Flap Powered on - module: " + id);
  Serial.print("Firmware Version: ");
  Serial.println(firmwareVersion);
  Serial.println("Mode: V2009_slave (paragraph message aware)");
  Serial.println("Input Mode: Serial Monitor (IDE) ONLY — LEFTMODE disabled");

  flap.setModuleListening(false); // Disable LEFTMODE entirely

  flap.indexToMagnet();
  flap.printInfo();
  Serial.println("--------------------------Setup complete-----------------------");
  ready_for_paragraph_message = true;
}

void loop() {
  // Only handle serial input (IDE)
  static String serialBuffer = "";

  while (Serial.available()) {
    char c = Serial.read();

    if (c == '\n' || c == '\r') {
      if (serialBuffer.length() > 0) {
        if (serialBuffer.equalsIgnoreCase("***LEFTMODE")) {
          Serial.println("LEFTMODE is disabled in this firmware.");
          serialBuffer = "";
          break;
        }

        if (serialBuffer.equalsIgnoreCase("***LEFTMODE")) {
  Serial.println("LEFTMODE is disabled in this firmware.");
} else if (serialBuffer.startsWith("****[")) {
  paragraph_message = serialBuffer.substring(4);
  paragraph_message_received = true;
  paragraph_message_sent = false;
  paragraph_message_processed = false;
  ready_for_paragraph_message = false;
  paragraph_message_logged = false;
  Serial.println("Paragraph message injected via Serial Monitor.");
} else {
  bool handled = handleCommand(serialBuffer);
  if (handled) {
    Serial.println("Command executed.");
  }
}

  // REMOVE LEFTMODE block entirely
  // Log paragraph message exactly once
  if (paragraph_message_received && !paragraph_message_logged) {
    Serial.println("-------------------------------------------");
    Serial.println("Full Paragraph Message Received:");
    Serial.println(paragraph_message);
    Serial.println("Source: Serial (Arduino IDE)");
    paragraph_message_logged = true;
    Serial.println("-------------------------------------------");
  }

  // Forward message downstream
  if (paragraph_message_received && !paragraph_message_sent) {
    Serial2.print(paragraph_message);
    Serial.println("Forwarded paragraph message to RIGHT TX.");
    paragraph_message_sent = true;
  }

  // Process paragraph message using scoped error-handling block
  if (paragraph_message != "" &&
      paragraph_message_sent &&
      !paragraph_message_processed) {

    bool parseSuccess = true;
    do {
      String id = flap.getModuleID();
      Serial.println("---------- PARSE DEBUG START ----------");
      Serial.print("Raw paragraph_message: [[");
      Serial.print(paragraph_message);
      Serial.println("]]");
      Serial.print("paragraph_message.length(): ");
      Serial.println(paragraph_message.length());
      Serial.println("---------- PARSE DEBUG END ------------");

      Serial.print("Parsing for Module ID: ");
      Serial.println(id);

      int startIndex = paragraph_message.indexOf("[");
      int endIndex = paragraph_message.indexOf("]");
      if (startIndex == -1 || endIndex == -1 || endIndex <= startIndex) {
        Serial.println("PARSE ERROR: Message format malformed. Could not locate [ and ].");
        parseSuccess = false;
        break;
      }

      String body = paragraph_message.substring(startIndex + 1, endIndex);
      Serial.print("Extracted Body: "); Serial.println(body);

      int ffffIndex = body.indexOf("FFFF:");
      if (ffffIndex != -1) {
        int colon1 = ffffIndex + 4;
        int colon2 = body.indexOf(':', colon1 + 1);
        if (colon2 == -1) {
          Serial.println("PARSE ERROR: Malformed FFFF command block.");
          parseSuccess = false;
          break;
        }
        current_command = body.substring(colon1 + 1, colon2);
        current_command_status = body.substring(colon2 + 1, body.indexOf('|', colon2 + 1)).toInt();
        Serial.println("Found FFFF broadcast command.");
      } else {
        int matchIndex = body.indexOf(id);
        Serial.print("ID match index: "); Serial.println(matchIndex);

        if (matchIndex != -1) {
          int colon1 = body.indexOf(':', matchIndex);
          int colon2 = body.indexOf(':', colon1 + 1);
          if (colon1 != -1 && colon2 != -1) {
            current_command = body.substring(colon1 + 1, colon2);
            int end = body.indexOf('|', colon2 + 1);
            if (end == -1) end = body.length();
            current_command_status = body.substring(colon2 + 1, end).toInt();
            Serial.println("Command assigned to this module:");
          } else {
            Serial.println("PARSE ERROR: Chunk found but colons could not be parsed.");
            parseSuccess = false;
            break;
          }
        } else {
          Serial.println("PARSE ERROR: Module ID not found in message.");
          parseSuccess = false;
          break;
        }
      }
    } while (false);

    if (!parseSuccess) {
      paragraph_message = "";
      paragraph_message_received = false;
      paragraph_message_sent = false;
      paragraph_message_processed = false;
      paragraph_message_logged = false;
      return;
    }

    Serial.print("Current Command: ");
    Serial.println(current_command);
    Serial.print("Command Status: ");
    Serial.println(current_command_status);
    paragraph_message_processed = true;
  }

  // Execute command
  if (paragraph_message_processed &&
      current_command != "" &&
      current_command_status == 0 &&
      !flap.isMotorBusy()) {

    Serial.print("Executing command: ");
    Serial.println(current_command);

    bool commandHandled = handleCommand(current_command);

    if (commandHandled) {
      current_command = "";
      current_command_status = 9;
      ready_for_paragraph_message = true;
      paragraph_message = "";
      paragraph_message_received = false;
      paragraph_message_sent = false;
      paragraph_message_processed = false;
      paragraph_message_logged = false;
    } else {
      Serial.println("Skipping execution due to invalid command.");
    }
  } else {
      Serial.println("Skipping execution due to invalid command.");
    }
  }
}
