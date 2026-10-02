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

void printDriveDebug() {
    int forward = Controller1.Axis3.position(pct);
    int turn = Controller1.Axis1.position(pct);
    int leftSpeed = forward + turn;
    int rightSpeed = forward - turn;
    int throttle = (abs(leftSpeed) + abs(rightSpeed)) / 2;
    const char* modeText = Competition.isDriverControl() ? "DRIVER" : "AUTO";

    Brain.Screen.clearScreen();
    Brain.Screen.setPenColor(white);
    Brain.Screen.printAt(10, 12, "[SYS] DRILL-BIT LINK ACTIVE");
    Brain.Screen.printAt(10, 24, "================================");
    Brain.Screen.printAt(10, 36, "FWD:%4d | TURN:%4d | THR:%3d", forward, turn, throttle);
    Brain.Screen.printAt(10, 48, "LEFT:%4d | RIGHT:%4d", leftSpeed, rightSpeed);
    Brain.Screen.printAt(10, 60, "MODE: ");
    Brain.Screen.printAt(58, 60, modeText);
    Brain.Screen.printAt(120, 60, " | STATUS: LIVE");
    Brain.Screen.printAt(10, 72, "PORTS: 1-4 | DRIVE: SPLIT ARCADE");
    Brain.Screen.printAt(10, 84, "STATUS: READY");
    Brain.Screen.printAt(10, 96, "--------------------------------");
    Brain.Screen.printAt(10, 108, "AI LINK: STABLE");
}

void initializeDrive() {
    LeftDrive.setStopping(brake);
    RightDrive.setStopping(brake);
    Brain.Screen.clearScreen();
    Brain.Screen.setPenColor(white);

    Brain.Screen.printAt(10, 12, "DRILL-BIT DNA SIGNATURE DETECTED");
    Brain.Screen.printAt(10, 24, "ENABLING DRILL-BIT ATTACK MODE");
    Brain.Screen.printAt(10, 36, "================================");
    Brain.Screen.printAt(10, 52, "SYSTEMS: ONLINE");
    Brain.Screen.printAt(10, 64, "TARGET: DUAL-MOTOR DRIVE");
    Brain.Screen.printAt(10, 76, "STATUS: READY");
    Brain.Screen.printAt(10, 88, "AI LINK: STABLE");
    Brain.Screen.printAt(10, 100, "--------------------------------");
    Brain.Screen.printAt(10, 112, "BOOT SEQUENCE COMPLETE");

    this_thread::sleep_for(1000);
    Brain.Screen.clearScreen();
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

        printDriveDebug();
        this_thread::sleep_for(5);
    }
}
