// --- Pin assignments ---
const int IN1 = 5;    // Stepper coil A1
const int IN2 = 4;    // Stepper coil A2
const int IN3 = 2;    // Stepper coil B1
const int IN4 = 15;   // Stepper coil B2

const int HALL_SENSOR_PIN = 34;  // A3144 OUT connected to GPIO34 (input-only)

// --- Stepper motor sequence for 28BYJ-48 (half-step mode) ---
const int stepSequence[8][4] = {
  {1, 0, 0, 0},
  {1, 1, 0, 0},
  {0, 1, 0, 0},
  {0, 1, 1, 0},
  {0, 0, 1, 0},
  {0, 0, 1, 1},
  {0, 0, 0, 1},
  {1, 0, 0, 1}
};

int currentStep = 0;

void stepMotor(int step) {
  digitalWrite(IN1, stepSequence[step][0]);
  digitalWrite(IN2, stepSequence[step][1]);
  digitalWrite(IN3, stepSequence[step][2]);
  digitalWrite(IN4, stepSequence[step][3]);
}

void setup() {
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  pinMode(HALL_SENSOR_PIN, INPUT);
  Serial.begin(115200);
  Serial.println("Stepper + Hall Sensor Sync Starting...");

  // Spin until initial magnet detection
  while (digitalRead(HALL_SENSOR_PIN) != LOW) {
    stepMotor(currentStep);
    currentStep = (currentStep + 1) % 8;
    delayMicroseconds(15000);
  }
  Serial.println("Initial magnet detected. Position set to 0 steps.");
  delay(500); // Give time to move past the detection zone
}

void loop() {
  for (int i = 1; i <= 10; i++) {
    int steps = 0;

    // Wait until magnet is no longer detected
    while (digitalRead(HALL_SENSOR_PIN) == LOW) {
      stepMotor(currentStep);
      currentStep = (currentStep + 1) % 8;
      delayMicroseconds(15000);
    }

    // Then wait until it's detected again, counting steps
    while (digitalRead(HALL_SENSOR_PIN) != LOW) {
      stepMotor(currentStep);
      currentStep = (currentStep + 1) % 8;
      steps++;
      delayMicroseconds(15000);
    }

    Serial.print("Cycle ");
    Serial.print(i);
    Serial.print(" completed. Steps since last detection: ");
    Serial.println(steps);
    delay(500);
  }

  Serial.println("All 10 cycles complete. Motor stopping.");
  while (true); // Stop the loop
}
