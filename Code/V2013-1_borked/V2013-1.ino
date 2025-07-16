// Unified Split Flap Firmware (Merged Master/Slave)
// Version: V2013-1
// -----------------------------------------------
// This firmware handles both Master and Slave roles using a single .ino
// Role is determined by the variable: ActiveListeningSerial
// Master listens to Serial Monitor; Slaves listen to Serial1 (LEFT)
// Paragraph messages are forwarded *before* being executed
// All commands can be executed either from paragraph messages or directly
// Uses setZeroOffsetSteps() and .Interactive() (step-based)
// -----------------------------------------------

#include <WiFi.h>
#include "SplitFlapMotor.h"
#include "flap_index.h"

const char* firmwareVersion = "V2013-1";

const int IN1 = 5, IN2 = 4, IN3 = 2, IN4 = 15;
const int HALL_PIN = 34;
SplitFlapMotor flap(IN1, IN2, IN3, IN4, HALL_PIN);

// Input source selection
enum SerialSource { SERIAL_MONITOR, LEFT_MODULE };
// SerialSource ActiveListeningSerial = SERIAL_MONITOR; // Change manually per role

SerialSource ActiveListeningSerial = LEFT_MODULE; // Change manually per role


// Message state
String paragraph_message = "";
String original_message = ""; // Preserves canonical full message
bool paragraph_message_received = false;
bool paragraph_message_sent = false;
bool paragraph_message_processed = false;
bool ready_for_paragraph_message = false;
bool paragraph_message_logged = false;

String current_command = "";
int current_command_status = 1;

void setup() {
  Serial.begin(115200);
  Serial1.begin(9600, SERIAL_8N1, 17, -1);  // LEFT RX
  Serial2.begin(9600, SERIAL_8N1, -1, 19);  // RIGHT TX
  flap.begin();

  String id = flap.getModuleID();
  Serial.println("***************************************************************");
  Serial.println("Split Flap Powered on - module: " + id);
  Serial.print("Firmware Version: "); Serial.println(firmwareVersion);
  Serial.print("Mode: ");
  Serial.println(ActiveListeningSerial == SERIAL_MONITOR ? "Master (Serial Monitor)" : "Slave (Left RX)");

  flap.setModuleListening(ActiveListeningSerial == LEFT_MODULE);
  flap.indexToMagnet();
  flap.printInfo();
  Serial.println("--------------------------Setup complete-----------------------");
  ready_for_paragraph_message = true;
}

// Unified serial input buffer reader based on role
String getInputFromActiveSerial() {
  static String buffer = "";
  HardwareSerial* activeSerial = (ActiveListeningSerial == LEFT_MODULE) ? &Serial1 : &Serial;

  while (activeSerial->available()) {
    char c = activeSerial->read();
    buffer += c;

    if ((c == '\n' || c == '\r') || (ActiveListeningSerial == LEFT_MODULE && c == ']')) {
      String result = buffer;
      buffer = "";
      return result;
    }
  }
  return "";
}

// Core command handler for both paragraph and direct serial input
bool executeCommand(String input) {
  input.replace('\r', ' ');
  bool commandHandled = false;

  if (input.startsWith("***")) {
    String command = input.substring(3);
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
      if (arg.length() == 0) flap.setStepOffsetInteractive();
      else flap.setStepOffset(arg.toInt());
    } else if (command.startsWith("ZOFFSET")) {
      String arg = command.substring(7);
      if (arg.length() == 0) flap.setZeroOffsetStepsInteractive();
      else flap.setZeroOffsetSteps(arg.toInt());
    } else if (command == "HELP") flap.printHelp();
    else {
      Serial.print("ERROR: Unknown command keyword: ");
      Serial.println(command);
    }
    commandHandled = true;
  } else if (input.length() == 1) {
    flap.goToCharacter(input.charAt(0));
    commandHandled = true;
  } else {
    Serial.print("ERROR: Unknown command format: ");
    Serial.println(input);
  }

  return commandHandled;
}

void loop() {
  // Unified input handler
  String input = getInputFromActiveSerial();
  if (input.length() > 0) {

    if (input.startsWith("***")) {
      if (input.startsWith("****[")) {
        // Detected a paragraph message - pass to forward/parse logic
        paragraph_message = input.substring(4); // [ ... ]
        original_message = input;              // ****[ ... ]
        paragraph_message_received = true;
        paragraph_message_sent = false;
        paragraph_message_processed = false;
        paragraph_message_logged = false;
        ready_for_paragraph_message = false;
        Serial.println("Paragraph message received.");
      } else {
        // Execute direct *** command immediately (not from paragraph)
        executeCommand(input);
      }
    } else if (!paragraph_message_received && input.length() == 1) {
      flap.goToCharacter(input.charAt(0));
    }
  }

  if (paragraph_message_received && !paragraph_message_logged) {
    Serial.println("-------------------------------------------");
    Serial.println("Full Paragraph Message Received:");
    Serial.println(paragraph_message);
    Serial.println("Source: " + String(ActiveListeningSerial == LEFT_MODULE ? "LEFT RX" : "Serial Monitor"));
    Serial.println("-------------------------------------------");
    paragraph_message_logged = true;
  }

  if (paragraph_message_received && !paragraph_message_sent) {
    Serial2.print(original_message); // Forward the canonical message
    Serial.println("Forwarded paragraph message to RIGHT TX. See Below:");
    Serial.println(original_message);
    paragraph_message_sent = true;
  }

  if (paragraph_message != "" && paragraph_message_sent && !paragraph_message_processed) {
    bool parseSuccess = true;
    do {
      String id = flap.getModuleID();
      int startIndex = paragraph_message.indexOf("[");
      int endIndex = paragraph_message.indexOf("]");
      if (startIndex == -1 || endIndex == -1 || endIndex <= startIndex) {
        Serial.println("PARSE ERROR: Malformed message.");
        parseSuccess = false;
        break;
      }

      String body = paragraph_message.substring(startIndex + 1, endIndex);
      int ffffIndex = body.indexOf("FFFF:");

      if (ffffIndex != -1) {
        int colon1 = body.indexOf(':', ffffIndex);
        int colon2 = body.indexOf(':', colon1 + 1);
        if (colon1 == -1 || colon2 == -1) {
          Serial.println("PARSE ERROR: Malformed FFFF block.");
          parseSuccess = false;
          break;
        }
        current_command = body.substring(colon1 + 1, colon2);
        int end = body.indexOf('|', colon2 + 1);
        if (end == -1) end = body.length();
        current_command_status = body.substring(colon2 + 1, end).toInt();
      } else {
        int matchIndex = body.indexOf(id);
        if (matchIndex != -1) {
          int colon1 = body.indexOf(':', matchIndex);
          int colon2 = body.indexOf(':', colon1 + 1);
          if (colon1 != -1 && colon2 != -1) {
            current_command = body.substring(colon1 + 1, colon2);
            int end = body.indexOf('|', colon2 + 1);
            if (end == -1) end = body.length();
            current_command_status = body.substring(colon2 + 1, end).toInt();
          } else {
            Serial.println("PARSE ERROR: Could not find colons.");
            parseSuccess = false;
            break;
          }
        } else {
          Serial.println("PARSE ERROR: ID not found in message.");
          parseSuccess = false;
          break;
        }
      }
    } while (false);

    if (!parseSuccess) {
      paragraph_message = "";
      original_message = "";
      paragraph_message_received = false;
      paragraph_message_sent = false;
      paragraph_message_processed = false;
      paragraph_message_logged = false;
      return;
    }

    paragraph_message_processed = true;
  }

  if (paragraph_message_processed && current_command != "" && current_command_status == 0 && !flap.isMotorBusy()) {
    Serial.print("Executing command: ");
    Serial.println(current_command);
    bool handled = executeCommand(current_command);
    if (handled) {
      current_command = "";
      current_command_status = 9;
      ready_for_paragraph_message = true;
      paragraph_message = "";
      original_message = "";
      paragraph_message_received = false;
      paragraph_message_sent = false;
      paragraph_message_processed = false;
      paragraph_message_logged = false;
    }
  }
}
