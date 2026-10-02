/*----------------------------------------------------------------------------*/
/*                                                                            */
/*    Module:       main.cpp                                                  */
/*    Author:       kodie                                                     */
/*    Created:      10/1/2026, 9:56:52 AM                                     */
/*    Description:  V5 project                                                */
/*                                                                            */
/*----------------------------------------------------------------------------*/

#include "vex.h"

using namespace vex;

vex::brain Brain;

motor LeftMotor1 = motor(PORT1, ratio18_1, false);
motor LeftMotor2 = motor(PORT2, ratio18_1, false);
motor RightMotor1 = motor(PORT3, ratio18_1, false);
motor RightMotor2 = motor(PORT4, ratio18_1, false);
motor_group LeftDrive(LeftMotor1, LeftMotor2);
motor_group RightDrive(RightMotor1, RightMotor2);
controller Controller1;
competition Competition;

constexpr int kDriveDeadbandPct = 5;
constexpr int kMaxMotorPct = 100;

int applyDeadband(int value) {
    if (value >= -kDriveDeadbandPct && value <= kDriveDeadbandPct) {
        return 0;
    }
    return value;
}

int clampPercent(int value) {
    if (value > kMaxMotorPct) {
        return kMaxMotorPct;
    }
    if (value < -kMaxMotorPct) {
        return -kMaxMotorPct;
    }
    return value;
}

void driveArcadeSplit() {
    int forward = applyDeadband(Controller1.Axis3.position(pct));
    int turn = applyDeadband(Controller1.Axis1.position(pct));

    int leftSpeed = forward + turn;
    int rightSpeed = forward - turn;

    int maxSpeed = abs(leftSpeed);
    if (abs(rightSpeed) > maxSpeed) {
        maxSpeed = abs(rightSpeed);
    }

    if (maxSpeed > kMaxMotorPct) {
        leftSpeed = (leftSpeed * kMaxMotorPct) / maxSpeed;
        rightSpeed = (rightSpeed * kMaxMotorPct) / maxSpeed;
    }

    LeftDrive.spin(fwd, clampPercent(leftSpeed), pct);
    RightDrive.spin(fwd, clampPercent(rightSpeed), pct);
}

void initializeDrive() {
    LeftDrive.setStopping(brake);
    RightDrive.setStopping(brake);
    Brain.Screen.clearScreen();
    Brain.Screen.printAt(10, 50, "Gabriel detected activating GABRIEL ATTACK MODE");
}

int main() {
    competition::bStopTasksBetweenModes = true;
    initializeDrive();

    while (1) {
        if (Competition.isDriverControl()) {
            driveArcadeSplit();
        } else {
            LeftDrive.stop();
            RightDrive.stop();
        }

        this_thread::sleep_for(10);
    }
}
