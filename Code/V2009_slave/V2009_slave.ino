#include <WiFi.h>
#include "SplitFlapMotor.h"
#include "flap_index.h"

const char* firmwareVersion = "V2009_slave";

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
  Serial.println("Input Mode: Listening to Serial Monitor (IDE) [***LEFTMODE to change]");

  flap.indexToMagnet();
  flap.printInfo();
  Serial.println("--------------------------Setup complete-----------------------");
  ready_for_paragraph_message = true;
}


void loop() {
  // Only handle serial input if not in LEFTMODE
  if (!flap.isModuleListeningFromLeft()) {
    static String serialBuffer = "";

    while (Serial.available()) {
      char c = Serial.read();

      if (c == '\n' || c == '\r') {
        serialBuffer.trim();

        if (serialBuffer.length() > 0) {
          if (serialBuffer.equalsIgnoreCase("***LEFTMODE")) {
            flap.setModuleListening(true);
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

        serialBuffer = ""; // ready for next command
      } else {
        serialBuffer += c;
      }
    }
  }

  // LEFTMODE-only: receive from left
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

  // Log paragraph message exactly once
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

  // Forward message downstream
  if (paragraph_message_received && !paragraph_message_sent) {
    Serial2.print(paragraph_message);
    Serial.println("Forwarded paragraph message to RIGHT TX.");
    paragraph_message_sent = true;
  }

  // Process the message chunk
  if (paragraph_message != "" &&
      paragraph_message_sent &&
      !paragraph_message_processed) {

    String id = flap.getModuleID();
    int startIndex = paragraph_message.indexOf("[");
    int endIndex = paragraph_message.indexOf("]");
    if (startIndex == -1 || endIndex == -1 || endIndex <= startIndex) {
      Serial.println("Message malformed.");
      current_command_status = 7;
    } else {
      String body = paragraph_message.substring(startIndex + 1, endIndex);
      int ffffIndex = body.indexOf("FFFF:");
      if (ffffIndex != -1) {
        int colon1 = ffffIndex + 4;
        int colon2 = body.indexOf(':', colon1 + 1);
        current_command = body.substring(colon1 + 1, colon2);
        current_command_status = body.substring(colon2 + 1, body.indexOf('|', colon2 + 1)).toInt();
        Serial.println("Found FFFF broadcast command.");
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
            Serial.println("Command assigned to this module:");
          } else {
            current_command = "";
            current_command_status = 7;
            Serial.println("Chunk found but couldn't parse.");
          }
        } else {
          current_command = "";
          current_command_status = 7;
          Serial.println("Module ID not found in message.");
        }
      }
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
    flap.goToCharacter(current_command.charAt(0));
    current_command = "";
    current_command_status = 9;
    ready_for_paragraph_message = true;
    paragraph_message = "";
    paragraph_message_received = false;
    paragraph_message_sent = false;
    paragraph_message_processed = false;
    paragraph_message_logged = false;
  }

  // Local test commands (characters, ***MOVE, etc.)
  if (!flap.isMotorBusy() &&
      Serial.available() > 0 &&
      !flap.isModuleListeningFromLeft()) {

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
