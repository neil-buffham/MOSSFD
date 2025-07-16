// Pin Definitions
#define PIN_DETECT_TOP     27  // D27
#define PIN_DETECT_RIGHT   26  // D26
#define PIN_DETECT_LEFT    25  // D25
#define PIN_DETECT_BOTTOM  13  // D13

// Role Definitions
bool amMainMaster = false;
bool amRowMaster = false;
bool amReg = false;     // Regular module
bool amRowEnd = false; 
bool amPleb = false;    // Bottom-right module, Plebian because its funny

void Master_detect() {
  // --- Set Detect Pins as Input (with pull-down) for neighbor detection ---
  pinMode(PIN_DETECT_TOP, INPUT_PULLDOWN);
  pinMode(PIN_DETECT_LEFT, INPUT_PULLDOWN);

  // --- Set Send Pins as Output ---
  pinMode(PIN_DETECT_BOTTOM, OUTPUT);
  pinMode(PIN_DETECT_RIGHT, OUTPUT);

  // --- Set Send Pins to High to signal presence ---
  digitalWrite(PIN_DETECT_BOTTOM, HIGH);
  digitalWrite(PIN_DETECT_RIGHT, HIGH);

  delay(5);  // Small delay to stabilize readings

  // --- Read input pins to check for neighbors ---
  bool hasTopNeighbor = digitalRead(PIN_DETECT_TOP);
  bool hasLeftNeighbor = digitalRead(PIN_DETECT_LEFT);

  // --- Role Logic ---
  if (!hasLeftNeighbor) { // No module to the left
    if (!hasTopNeighbor) {
      amMainMaster = true;
      Serial.println("This is the MAIN MASTER module.");
    } else {
      amRowMaster = true;
      Serial.println("This is a ROW MASTER module.");
    }
  } else {
    amReg = true;
    Serial.println("This is a REGULAR module.");
  }

  // --- Optional Debug Print ---
  Serial.print("Top neighbor: "); Serial.println(hasTopNeighbor ? "YES" : "NO");
  Serial.print("Left neighbor: "); Serial.println(hasLeftNeighbor ? "YES" : "NO");
}

void pleb_detect() {
  // Step 1: Set bottom/right as INPUT_PULLDOWN to sense neighbors
  pinMode(PIN_DETECT_BOTTOM, INPUT_PULLDOWN);
  pinMode(PIN_DETECT_RIGHT, INPUT_PULLDOWN);

  // Step 2: Set top/left as OUTPUT HIGH to signal presence
  pinMode(PIN_DETECT_TOP, OUTPUT);
  digitalWrite(PIN_DETECT_TOP, HIGH);
  pinMode(PIN_DETECT_LEFT, OUTPUT);
  digitalWrite(PIN_DETECT_LEFT, HIGH);

  delay(5); // Allow time for pins to stabilize

  // Step 3: Read bottom/right to see if any module is signaling back
  bool hasBottomNeighbor = digitalRead(PIN_DETECT_BOTTOM);
  bool hasRightNeighbor = digitalRead(PIN_DETECT_RIGHT);

  if (!hasBottomNeighbor && !hasRightNeighbor) {
    amPleb = true;
    Serial.println("This is the PLEB module (bottom-right corner).");
  } else if (!hasRightNeighbor && hasBottomNeighbor) {
    amRowEnd = true;
    Serial.println("This is a ROW END module (rightmost in row).");
  } else {
    Serial.println("This module is not a pleb or row end.");
  }

  // Optional debug output
  Serial.print("Bottom neighbor: "); Serial.println(hasBottomNeighbor ? "YES" : "NO");
  Serial.print("Right neighbor: ");  Serial.println(hasRightNeighbor ? "YES" : "NO");
}


void setup() {
  Serial.begin(115200);
  delay(100); // Give serial time to start
  Master_detect();
}

void loop() {
  // No loop logic for now
}
