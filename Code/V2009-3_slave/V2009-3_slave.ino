// Split Flap Display - V2009-3_slave
// --------------------------------------------------
// Changelog:
// [2025-07-01] Updated ZOFFSET command handling to use:
//              - setZeroOffsetStepsInteractive()
//              - setZeroOffsetSteps(int)
//              (Replaced previous degree-based functions)
// --------------------------------------------------
// Save attempt - messed up with google drive syncing

//V2009-3_slave
#include <WiFi.h>
#include "SplitFlapMotor.h"
#include "flap_index.h"

const char* firmwareVersion = "V2009-3_slave";

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

void setup() {
  Serial.begin(115200);  // USB serial (IDE)
  Serial1.begin(9600, SERIAL_8N1, 17, -1);  // Left Comm RX on GPIO17
  Serial2.begin(9600, SERIAL_8N1, -1, 19);  // Right Comm TX on GPIO19
  flap.begin();

  String id = flap.getModuleID();
  Serial.println("***************************************************************");
  Serial.println("Split Flap Powered on - module: " + id);
  Serial.print("Firmware Version: ");
  Serial.println(firmwareVersion);
  Serial.println("Mode: V2009_slave (paragraph message aware)");
  flap.setModuleListening(true);
  Serial.println("Input Mode: Listening from the left only and you can't change it, hhahahahahahahaa");

  flap.indexToMagnet();
  flap.printInfo();
  Serial.println("--------------------------Setup complete-----------------------");
  ready_for_paragraph_message = true;
}

void loop() {
  if (!flap.isModuleListeningFromLeft()) {
    static String serialBuffer = "";

    while (Serial.available()) {
      char c = Serial.read();

      if (c == '\n' || c == '\r') {
        if (serialBuffer.length() > 0) {
          if (serialBuffer.equalsIgnoreCase("***LEFTMODE")) {
            flap.setModuleListening(true);
            Serial.println("LEFTMODE enabled — Serial1 listening active.");
            serialBuffer = "";
            break;
          }

          if (serialBuffer.equalsIgnoreCase("***HELP")) {
            flap.printHelp();
          }

          else if (serialBuffer.startsWith("****[")) {
            paragraph_message = serialBuffer.substring(4);
            paragraph_message_received = true;
            paragraph_message_sent = false;
            paragraph_message_processed = false;
            ready_for_paragraph_message = false;
            paragraph_message_logged = false;
            Serial.println("Paragraph message injected via Serial Monitor.");
          }
        }

        serialBuffer = "";
      } else {
        serialBuffer += c;
      }
    }
  }

  if (flap.isModuleListeningFromLeft()) {
    static String fromLeft = "";
    while (Serial1.available()) {
      char c = Serial1.read();
      fromLeft += c;
      if (c == ']') {
        paragraph_message = fromLeft;
        paragraph_message_received = true;
        paragraph_message_sent = false;
        paragraph_message_processed = false;
        ready_for_paragraph_message = false;
        paragraph_message_logged = false;
        fromLeft = "";
        Serial.println("Paragraph message received from LEFT RX.");
        break;
      }
    }
  }

  if (paragraph_message_received && !paragraph_message_logged) {
    Serial.println("-------------------------------------------");
    Serial.println("Full Paragraph Message Received:");
    Serial.println(paragraph_message);

    if (flap.isModuleListeningFromLeft()) {
      Serial.println("Source: Serial1 (LEFT MODULE)");
    } else {
      Serial.println("Source: Serial (Arduino IDE)");
    }

    paragraph_message_logged = true;
    Serial.println("-------------------------------------------");
  }

  if (paragraph_message_received && !paragraph_message_sent) {
    Serial2.print(paragraph_message);
    Serial.println("Forwarded paragraph message to RIGHT TX.");
    paragraph_message_sent = true;
  }

  if (paragraph_message != "" &&
      paragraph_message_sent &&
      !paragraph_message_processed) {

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
      paragraph_message = "";
      paragraph_message_received = false;
      paragraph_message_sent = false;
      paragraph_message_processed = false;
      paragraph_message_logged = false;
      return;
    }

    String body = paragraph_message.substring(startIndex + 1, endIndex);
    Serial.print("Extracted Body: "); Serial.println(body);

    int ffffIndex = body.indexOf("FFFF:");
    if (ffffIndex != -1) {
      int colon1 = ffffIndex + 4;
      int colon2 = body.indexOf(':', colon1 + 1);
      if (colon2 == -1) {
        Serial.println("PARSE ERROR: Malformed FFFF command block.");
        paragraph_message = "";
        paragraph_message_received = false;
        paragraph_message_sent = false;
        paragraph_message_processed = false;
        paragraph_message_logged = false;
        return;
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
          paragraph_message = "";
          paragraph_message_received = false;
          paragraph_message_sent = false;
          paragraph_message_processed = false;
          paragraph_message_logged = false;
          return;
        }
      } else {
        Serial.println("PARSE ERROR: Module ID not found in message.");
        paragraph_message = "";
        paragraph_message_received = false;
        paragraph_message_sent = false;
        paragraph_message_processed = false;
        paragraph_message_logged = false;
        return;
      }
    }

    Serial.print("Current Command: ");
    Serial.println(current_command);
    Serial.print("Command Status: ");
    Serial.println(current_command_status);
    paragraph_message_processed = true;
  }

  if (paragraph_message_processed &&
      current_command != "" &&
      current_command_status == 0 &&
      !flap.isMotorBusy()) {

    Serial.print("Executing command: ");
    Serial.println(current_command);

    String input = current_command;
    input.replace('\r', ' ');

    bool commandHandled = false;

    if (input.startsWith("***")) {
      String command = input.substring(3);
      command.toUpperCase();

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
        if (arg.length() == 0) flap.setZeroOffsetStepsInteractive(); // UPDATED
        else flap.setZeroOffsetSteps(arg.toInt()); // UPDATED
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
  }

  if (!flap.isMotorBusy() &&
      Serial.available() > 0 &&
      !flap.isModuleListeningFromLeft()) {

    String input = Serial.readStringUntil('\n');
    input.replace('\r', ' ');

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
        int index = command.toInt();
        if (index >= 0 && index < NUM_FLAPS) flap.goToIndex(index);
        else Serial.println("Invalid command or index.");
      }
    } else if (input.length() > 0) {
      char ch = input.charAt(0);
      int idx = flap.getCharacterIndex(ch);
      if (idx == -1) {
        Serial.print("Invalid character: '");
        Serial.print(ch);
        Serial.println("' — not found in flap index.");
      } else {
        flap.goToCharacter(ch);
      }
    }
  }
}
