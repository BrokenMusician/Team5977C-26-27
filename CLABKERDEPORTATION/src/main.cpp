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

    Brain.Screen.clearScreen();
    Brain.Screen.setCursor(1, 1);
    Brain.Screen.print("[SYS] DRIVE HUD");
    Brain.Screen.setCursor(2, 1);
    Brain.Screen.print("----------------");
    Brain.Screen.setCursor(3, 1);
    Brain.Screen.print("FWD:%4d TURN:%4d", forward, turn);
    Brain.Screen.setCursor(4, 1);
    Brain.Screen.print("LEFT:%4d RIGHT:%4d", leftSpeed, rightSpeed);
    Brain.Screen.setCursor(5, 1);
    Brain.Screen.print("MODE:%s", Competition.isDriverControl() ? "DRIVER" : "AUTO");
}

void initializeDrive() {
    LeftDrive.setStopping(brake);
    RightDrive.setStopping(brake);
    Brain.Screen.clearScreen();
    Brain.Screen.setCursor(1, 1);
    Brain.Screen.print("[BOOT] GABRIEL DNA SIGNATURE DETECTED ENABLING GABRIEL ATTACK MODE");
    Brain.Screen.setCursor(2, 1);
    Brain.Screen.print("------------------------");
    Brain.Screen.setCursor(3, 1);
    Brain.Screen.print("SYSTEMS ONLINE");
    Brain.Screen.setCursor(4, 1);
    Brain.Screen.print("STATUS: READY");
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
