#include <WiFi.h>
#include "SplitFlapMotor.h"
#include "flap_index.h"

const char* firmwareVersion = "V2009-2_slave";

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
      serialBuffer.trim();

      if (serialBuffer.length() > 0) {
        if (serialBuffer.equalsIgnoreCase("***LEFTMODE")) {
          Serial.println("LEFTMODE is disabled in this firmware.");
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

  // (Remaining code unchanged)
  // Execute command and handle local test commands, as previously written...
}
