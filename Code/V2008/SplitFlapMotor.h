#ifndef SPLIT_FLAP_MOTOR_H
#define SPLIT_FLAP_MOTOR_H

#include <Arduino.h>
#include <AccelStepper.h>

class SplitFlapMotor {
  public:
    SplitFlapMotor(int in1, int in2, int in3, int in4, int hallPin);

    void begin();
    String getModuleID();

    void indexToMagnet();
    void detectStepsPerRevolution();
    void goToCharacter(char inputChar);
    void goToIndex(int targetIndex);
    void moveSteps(int steps);

    void setZeroOffsetDegrees(long offset);
    void setZeroOffsetDegreesInteractive();
    void setStepOffset(long offset);
    void setStepOffsetInteractive();
    void resetConfig();

    void printInfo();
    void printHelp();
    void printMagnetPassCount();
    void printPosition();
    void printIndexList();

    bool isMotorBusy();

  private:
    AccelStepper stepper;
    int hallSensorPin;
    long zeroOffsetDegrees;
    long stepOffset;
    int magnetPassCount;
    bool motorBusy;

    long getStepsPerDegree();
    int getCharacterIndex(char inputChar);
};

#endif
