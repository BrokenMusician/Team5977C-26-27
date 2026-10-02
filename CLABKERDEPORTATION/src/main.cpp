/*----------------------------------------------------------------------------*/
/*    Module:       main.cpp                                                  */
/*    Author:       kodie                                                     */
/*    Description:  V5 project - VEXTOP v3 (HUD edition)                      */
/*----------------------------------------------------------------------------*/

#include "vex.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>

using namespace vex;

vex::brain Brain;

motor LeftMotor1 = motor(PORT1, ratio18_1, false);
motor LeftMotor2 = motor(PORT2, ratio18_1, false);
motor RightMotor1 = motor(PORT3, ratio18_1, false);
motor RightMotor2 = motor(PORT4, ratio18_1, false);
motor_group LeftDrive(LeftMotor1, LeftMotor2);
motor_group RightDrive(RightMotor1, RightMotor2);
motor* const motorDevices[4] = {&LeftMotor1, &LeftMotor2, &RightMotor1, &RightMotor2};
controller Controller1;
competition Competition;

/* ------------------------------- settings -------------------------------- */

int driveDeadbandPct = 5;
constexpr int kMaxMotorPct = 100;
int selectedTab = 0;
int uiFrame = 0;

const int motorPorts[4] = {1, 2, 3, 4};
const char* const motorNames[4] = {"LEFT A", "LEFT B", "RIGHT A", "RIGHT B"};
const char* const tabNames[5] = {"DASH", "MOTORS", "GRAPH", "INPUT", "SYSTEM"};
const char* const buttonNames[12] = {"L1", "L2", "R1", "R2", "A", "B", "X", "Y",
                                     "UP", "DN", "LT", "RT"};

constexpr int kSideW = 66;
constexpr int kTabH = 42;
constexpr int kTabTop = 28;

struct Rgb { int r, g, b; };
#define PAL(name, R, G, B) const Rgb name##Rgb = {R, G, B}; const color name(R, G, B);
PAL(cBg, 8, 11, 18)
PAL(cPanel, 17, 23, 34)
PAL(cLine, 40, 50, 68)
PAL(cText, 236, 242, 252)
PAL(cMuted, 116, 128, 150)
PAL(cAccent, 0, 200, 255)
PAL(cAccent2, 255, 64, 150)
PAL(cGood, 70, 240, 150)
PAL(cWarn, 255, 184, 48)
PAL(cDanger, 255, 64, 76)
PAL(cRain, 0, 52, 68)
PAL(cWarnDim, 70, 50, 12)

/* ------------------------------ small helpers ------------------------------ */

int clampInt(int v, int lo, int hi) { return v < lo ? lo : v > hi ? hi : v; }
int rnd(float v) { return static_cast<int>(v >= 0 ? v + 0.5f : v - 0.5f); }

color mix(const Rgb& a, const Rgb& b, int t) {
    t = clampInt(t, 0, 256);
    return color(a.r + (b.r - a.r) * t / 256,
                 a.g + (b.g - a.g) * t / 256,
                 a.b + (b.b - a.b) * t / 256);
}

const Rgb& heatRgb(int temp) {
    return temp >= 60 ? cDangerRgb : temp >= 45 ? cWarnRgb : cGoodRgb;
}

/* ------------------------------ drive logic ------------------------------- */

int applyDeadband(int value) {
    if (value >= -driveDeadbandPct && value <= driveDeadbandPct) return 0;
    return value;
}

int clampPercent(int value) { return clampInt(value, -kMaxMotorPct, kMaxMotorPct); }

struct DriveOutput { int left; int right; };

DriveOutput mixDrive(int forward, int turn) {
    DriveOutput output = {forward + turn, forward - turn};
    int maxSpeed = abs(output.left) > abs(output.right) ? abs(output.left) : abs(output.right);
    if (maxSpeed > kMaxMotorPct) {
        output.left = output.left * kMaxMotorPct / maxSpeed;
        output.right = output.right * kMaxMotorPct / maxSpeed;
    }
    output.left = clampPercent(output.left);
    output.right = clampPercent(output.right);
    return output;
}

void driveArcadeSplit() {
    DriveOutput output = mixDrive(
        applyDeadband(Controller1.Axis3.position(pct)),
        applyDeadband(Controller1.Axis1.position(pct)));
    LeftDrive.spin(fwd, output.left, pct);
    RightDrive.spin(fwd, output.right, pct);
}

/* ----------------------------- draw primitives ---------------------------- */
// (prefixed with g so nothing collides with vex:: names like vex::line)

void gText(int x, int y, color c, const char* fmt, ...) {
    char buf[48];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    Brain.Screen.setPenColor(c);
    Brain.Screen.printAt(x, y, true, "%s", buf);
}

void gTextC(int cx, int y, int charW, color c, const char* fmt, ...) {
    char buf[48];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    Brain.Screen.setPenColor(c);
    Brain.Screen.printAt(cx - static_cast<int>(strlen(buf)) * charW / 2, y, true, "%s", buf);
}

void gLine(int x1, int y1, int x2, int y2, color c) {
    Brain.Screen.setPenColor(c);
    Brain.Screen.drawLine(x1, y1, x2, y2);
}

void gRect(int x, int y, int w, int h, color c) {
    if (w <= 0 || h <= 0) return;
    Brain.Screen.setPenColor(c);
    Brain.Screen.drawRectangle(x, y, w, h, c);
}

void gBox(int x, int y, int w, int h, color c) {
    Brain.Screen.setPenColor(c);
    Brain.Screen.drawLine(x, y, x + w, y);
    Brain.Screen.drawLine(x, y + h, x + w, y + h);
    Brain.Screen.drawLine(x, y, x, y + h);
    Brain.Screen.drawLine(x + w, y, x + w, y + h);
}

void gDot(int x, int y, int r, color c) {
    Brain.Screen.setPenColor(c);
    Brain.Screen.drawCircle(x, y, r, c);
}

void gRing(int x, int y, int r, color c) {
    Brain.Screen.setFillColor(color::transparent);
    Brain.Screen.setPenColor(c);
    Brain.Screen.drawCircle(x, y, r);
}

// HUD card: dark panel, thin border, bright corner brackets, title with trailing rule
void gCard(int x, int y, int w, int h, const char* title) {
    gRect(x, y, w, h, cPanel);
    gBox(x, y, w, h, cLine);
    const int L = 9;
    gLine(x, y, x + L, y, cAccent);
    gLine(x, y + 1, x + L, y + 1, cAccent);
    gLine(x, y, x, y + L, cAccent);
    gLine(x + 1, y, x + 1, y + L, cAccent);
    gLine(x + w - L, y + h, x + w, y + h, cAccent);
    gLine(x + w - L, y + h - 1, x + w, y + h - 1, cAccent);
    gLine(x + w, y + h - L, x + w, y + h, cAccent);
    gLine(x + w - 1, y + h - L, x + w - 1, y + h, cAccent);
    if (title) {
        gText(x + 12, y + 16, cMuted, "%s", title);
        int rx = x + 12 + static_cast<int>(strlen(title)) * 8 + 6;
        gLine(rx, y + 12, x + w - 26, y + 12, cLine);
    }
}

// gradient segmented bar, value 0..100
void barG(int x, int y, int w, int h, int v, const Rgb& lo, const Rgb& hi) {
    v = clampInt(v, 0, 100);
    gRect(x, y, w, h, cBg);
    int segs = (w - 2) / 6;
    if (segs < 1) segs = 1;
    int lit = v * segs / 100;
    for (int i = 0; i < lit; ++i) {
        gRect(x + 1 + i * 6, y + 1, 5, h - 2, mix(lo, hi, segs > 1 ? i * 256 / (segs - 1) : 0));
    }
    gBox(x, y, w, h, cLine);
}

// signed gradient bar, value -100..100
void sbarG(int x, int y, int w, int h, int v, const Rgb& loP, const Rgb& hiP,
           const Rgb& loN, const Rgb& hiN) {
    v = clampInt(v, -100, 100);
    gRect(x, y, w, h, cBg);
    int center = x + w / 2;
    int segs = (w / 2 - 2) / 6;
    if (segs < 1) segs = 1;
    int lit = abs(v) * segs / 100;
    for (int i = 0; i < lit; ++i) {
        int t = segs > 1 ? i * 256 / (segs - 1) : 0;
        color c = v < 0 ? mix(loN, hiN, t) : mix(loP, hiP, t);
        int px = v < 0 ? center - 1 - (i + 1) * 6 + 1 : center + 2 + i * 6;
        gRect(px, y + 1, 5, h - 2, c);
    }
    gLine(center, y, center, y + h, cMuted);
    gBox(x, y, w, h, cLine);
}

// 270-degree tick gauge, gap at the bottom. peak < 0 hides the peak marker.
void arcGauge(int cx, int cy, int r, int thick, int value, const Rgb& lo, const Rgb& hi, int peak) {
    const int N = 32;
    value = clampInt(value, 0, 100);
    int lit = value * N / 100;
    for (int i = 0; i < N; ++i) {
        double a = (135.0 + 270.0 * i / (N - 1)) * 0.0174533;
        color c = i < lit ? mix(lo, hi, i * 256 / (N - 1)) : cLine;
        Brain.Screen.setPenColor(c);
        for (int k = 0; k < 2; ++k) {
            double b = a + k * 0.03;
            Brain.Screen.drawLine(cx + static_cast<int>((r - thick) * cos(b)),
                                  cy + static_cast<int>((r - thick) * sin(b)),
                                  cx + static_cast<int>(r * cos(b)),
                                  cy + static_cast<int>(r * sin(b)));
        }
    }
    if (peak >= 0) {
        double a = (135.0 + 270.0 * clampInt(peak, 0, 100) / 100.0) * 0.0174533;
        Brain.Screen.setPenColor(cText);
        Brain.Screen.drawLine(cx + static_cast<int>((r + 1) * cos(a)), cy + static_cast<int>((r + 1) * sin(a)),
                              cx + static_cast<int>((r + 6) * cos(a)), cy + static_cast<int>((r + 6) * sin(a)));
    }
}

int stickPx(int c, int s, int v) { return c + clampInt(v, -100, 100) * (s / 2 - 6) / 100; }

// stick box with fading trail. trail arrays are oldest -> newest.
void stickBox(int x, int y, int s, int vx, int vy, bool showDz, const int* tx, const int* ty, int tn) {
    int cx = x + s / 2, cy = y + s / 2;
    gRect(x, y, s, s, cBg);
    gBox(x, y, s, s, cLine);
    gLine(x, cy, x + s, cy, cLine);
    gLine(cx, y, cx, y + s, cLine);
    gRing(cx, cy, s / 2 - 2, cLine);
    gRing(cx, cy, s / 4, cLine);
    int dz = driveDeadbandPct * (s / 2) / 100;
    if (showDz && dz > 0) gBox(cx - dz, cy - dz, dz * 2, dz * 2, cMuted);
    for (int i = 0; i < tn; ++i) {
        gDot(stickPx(cx, s, tx[i]), stickPx(cy, s, -ty[i]), 1 + i * 3 / tn,
             mix(cLineRgb, cAccentRgb, (i + 1) * 256 / tn));
    }
    int px = stickPx(cx, s, vx), py = stickPx(cy, s, -vy);
    bool idle = abs(vx) <= driveDeadbandPct && abs(vy) <= driveDeadbandPct;
    gLine(cx, cy, px, py, idle ? cLine : cAccent);
    gDot(px, py, 5, idle ? cMuted : cAccent);
}

/* -------------------------------- telemetry ------------------------------- */

struct Tel {
    int rawFwd, rawTurn, rawLx, rawRy, fw, tr, left, right, throttle;
    int bat, mv, bda, btemp, ctrl, sd, mode, enabled, field, sw, runtime;
    int mTemp[4], mRpm[4], mDa[4], mInst[4], mOut[4];
    int online, hot, warn, totalDa;
    int btn[12];
    // smoothed for display
    int sLeft, sRight, sThr, sRpm[4], peak;
};

void readTel(Tel& t) {
    t.rawFwd = Controller1.Axis3.position(pct);
    t.rawTurn = Controller1.Axis1.position(pct);
    t.rawLx = Controller1.Axis4.position(pct);
    t.rawRy = Controller1.Axis2.position(pct);
    t.fw = applyDeadband(t.rawFwd);
    t.tr = applyDeadband(t.rawTurn);
    bool driver = Competition.isDriverControl();
    DriveOutput o = mixDrive(t.fw, t.tr);
    t.left = driver ? o.left : 0;
    t.right = driver ? o.right : 0;
    t.throttle = (abs(t.left) + abs(t.right)) / 2;
    t.bat = clampInt(Brain.Battery.capacity(pct), 0, 100);
    t.mv = static_cast<int>(Brain.Battery.voltage() * 1000.0 + 0.5);
    t.bda = static_cast<int>(Brain.Battery.current(amp) * 10.0);
    t.btemp = static_cast<int>(Brain.Battery.temperature(celsius));
    t.ctrl = Controller1.installed();
    t.sd = Brain.SDcard.isInserted();
    t.mode = Competition.isAutonomous() ? 1 : driver ? 2 : 0;
    t.enabled = Competition.isEnabled() ? 1 : 0;
    t.field = Competition.isFieldControl() ? 1 : 0;
    t.sw = Competition.isCompetitionSwitch() ? 1 : 0;
    t.runtime = static_cast<int>(Brain.Timer.value());
    t.online = 0;
    t.hot = 0;
    t.totalDa = 0;
    for (int i = 0; i < 4; ++i) {
        t.mTemp[i] = static_cast<int>(motorDevices[i]->temperature(celsius));
        t.mRpm[i] = static_cast<int>(motorDevices[i]->velocity(rpm));
        t.mDa[i] = static_cast<int>(motorDevices[i]->current(amp) * 10.0);
        t.mInst[i] = motorDevices[i]->installed();
        t.mOut[i] = i < 2 ? t.left : t.right;
        if (t.mInst[i]) ++t.online;
        t.totalDa += abs(t.mDa[i]);
        if (t.mTemp[i] > t.mTemp[t.hot]) t.hot = i;
    }
    t.warn = (t.online < 4) + (t.mTemp[t.hot] >= 60) + (t.bat <= 20) + (t.btemp >= 50);
    int b[12] = {
        Controller1.ButtonL1.pressing(), Controller1.ButtonL2.pressing(),
        Controller1.ButtonR1.pressing(), Controller1.ButtonR2.pressing(),
        Controller1.ButtonA.pressing(), Controller1.ButtonB.pressing(),
        Controller1.ButtonX.pressing(), Controller1.ButtonY.pressing(),
        Controller1.ButtonUp.pressing(), Controller1.ButtonDown.pressing(),
        Controller1.ButtonLeft.pressing(), Controller1.ButtonRight.pressing()};
    for (int i = 0; i < 12; ++i) t.btn[i] = b[i];
}

/* animation / history state */
constexpr int kHist = 95;
int histL[kHist], histR[kHist], histA[kHist];
int histHead = 0;
constexpr int kTrail = 8;
int trailX[2][kTrail], trailY[2][kTrail];
float wheelPhase[4] = {0, 0, 0, 0};
int transFrames = 0;

void pushHistory(const Tel& t) {
    histL[histHead] = (t.mRpm[0] + t.mRpm[1]) / 2;
    histR[histHead] = (t.mRpm[2] + t.mRpm[3]) / 2;
    histA[histHead] = t.totalDa;
    histHead = (histHead + 1) % kHist;
}

void pushTrail(int s, int x, int y) {
    for (int i = 0; i < kTrail - 1; ++i) {
        trailX[s][i] = trailX[s][i + 1];
        trailY[s][i] = trailY[s][i + 1];
    }
    trailX[s][kTrail - 1] = x;
    trailY[s][kTrail - 1] = y;
}

/* ---------------------------------- chrome -------------------------------- */

const char* modeName(int mode) { return mode == 2 ? "DRIVER" : mode == 1 ? "AUTO" : "DISABLED"; }

void drawChrome(const Tel& t) {
    // title with offset shadow
    Brain.Screen.setFont(prop20);
    gText(9, 21, cAccent2, "VEXTOP");
    gText(8, 20, cText, "VEXTOP");
    Brain.Screen.setFont(mono12);
    gDot(88, 13, 2 + ((uiFrame / 8) % 2), t.warn > 0 ? cDanger : cGood);
    gText(100, 19, cMuted, "// %s", tabNames[selectedTab]);

    // mode chip
    color mc = t.mode == 2 ? cAccent : t.mode == 1 ? cWarn : cMuted;
    if (t.mode != 0) gRect(268, 4, 96, 18, mc);
    else gBox(268, 4, 96, 18, mc);
    gTextC(316, 18, 8, t.mode != 0 ? cBg : mc, "%s", modeName(t.mode));

    // battery
    color bc = t.bat <= 20 ? cDanger : t.bat <= 50 ? cWarn : cGood;
    gText(374, 19, cText, "%3d%%", t.bat);
    gBox(414, 7, 30, 12, cMuted);
    gRect(444, 10, 3, 6, cMuted);
    gRect(416, 9, t.bat * 26 / 100, 8, bc);
    if (t.warn > 0) gText(458, 19, cDanger, "!");

    // header rule with sweeping highlight
    gLine(0, 26, 480, 26, cLine);
    int sx = (uiFrame * 7) % 560 - 40;
    for (int k = 0; k < 40; ++k) {
        int x = sx + k;
        if (x < 0 || x >= 480) continue;
        gRect(x, 25, 1, 3, mix(cLineRgb, cAccentRgb, k * 256 / 40));
    }

    // sidebar
    gRect(0, 28, kSideW, 212, cPanel);
    gLine(kSideW, 27, kSideW, 240, cLine);
    for (int i = 0; i < 5; ++i) {
        int y = kTabTop + i * kTabH;
        bool sel = i == selectedTab;
        if (sel) {
            gRect(0, y, kSideW, kTabH - 2, cBg);
            gRect(0, y, 3, kTabH - 2, cAccent);
            gLine(kSideW - 1, y, kSideW - 1, y + kTabH - 3, cAccent);
        }
        Brain.Screen.setFont(mono12);
        gText(10, y + 14, sel ? cAccent : cLine, "0%d", i + 1);
        Brain.Screen.setFont(mono15);
        gTextC(kSideW / 2 + 2, y + 33, 9, sel ? cAccent : cMuted, "%s", tabNames[i]);
    }
    Brain.Screen.setFont(mono12);
}

/* ---------------------------------- pages --------------------------------- */

void drawWheel(int x, int y, int out, float phase) {
    const int w = 12, h = 36;
    int mag = clampInt(abs(out), 0, 100);
    color c = out < 0 ? mix(cLineRgb, cAccent2Rgb, mag * 256 / 100) : mix(cLineRgb, cAccentRgb, mag * 256 / 100);
    gRect(x, y, w, h, c);
    int off = static_cast<int>(phase) % 6;
    for (int k = 0; k < 6; ++k) {
        int yy = y + ((k * 6 - off + 36) % 36);
        gLine(x + 1, yy, x + w - 1, yy, cBg);
    }
    gBox(x, y, w, h, cMuted);
}

void pageDash(const Tel& t) {
    // drivetrain card with live robot
    gCard(76, 32, 190, 200, "DRIVETRAIN");
    const int cx = 171, cy = 106;
    drawWheel(129, 66, t.sLeft, wheelPhase[0]);
    drawWheel(129, 110, t.sLeft, wheelPhase[1]);
    drawWheel(201, 66, t.sRight, wheelPhase[2]);
    drawWheel(201, 110, t.sRight, wheelPhase[3]);
    gRect(143, 64, 56, 84, cBg);
    gBox(143, 64, 56, 84, cAccent);
    gRect(161, 67, 20, 3, cAccent);
    gLine(cx - 6, cy, cx + 6, cy, cLine);
    gLine(cx, cy - 6, cx, cy + 6, cLine);
    for (int i = 0; i < 4; ++i) {
        int ty = i % 2 == 0 ? 88 : 132;
        int tx = i < 2 ? 88 : 222;
        gText(tx, ty, mix(cMutedRgb, heatRgb(t.mTemp[i]), 256), "%2dC", t.mTemp[i]);
    }
    float fv = (t.sLeft + t.sRight) / 2.0f;
    float tv = (t.sLeft - t.sRight) / 2.0f;
    float mag = sqrtf(fv * fv + tv * tv);
    if (mag > 6.0f) {
        float dx = tv * 0.44f, dy = -fv * 0.44f;
        int ex = cx + rnd(dx), ey = cy + rnd(dy);
        color ac = fv < 0 ? cAccent2 : cText;
        gLine(cx, cy, ex, ey, ac);
        gLine(cx + 1, cy, ex + 1, ey, ac);
        double ang = atan2(static_cast<double>(dy), static_cast<double>(dx));
        gLine(ex, ey, ex + static_cast<int>(9 * cos(ang + 2.6)), ey + static_cast<int>(9 * sin(ang + 2.6)), ac);
        gLine(ex, ey, ex + static_cast<int>(9 * cos(ang - 2.6)), ey + static_cast<int>(9 * sin(ang - 2.6)), ac);
    }
    gText(86, 188, cMuted, "L");
    gText(100, 188, cText, "%4d%%", t.sLeft);
    gText(176, 188, cMuted, "R");
    gText(190, 188, cText, "%4d%%", t.sRight);
    gText(86, 208, cMuted, "FWD");
    gText(118, 208, cText, "%4d", t.fw);
    gText(176, 208, cMuted, "TRN");
    gText(208, 208, cText, "%4d", t.tr);

    // throttle gauge
    gCard(272, 32, 202, 130, "THROTTLE");
    arcGauge(373, 104, 50, 10, t.sThr, cAccentRgb, t.sThr > 80 ? cDangerRgb : cAccent2Rgb, t.peak);
    gRing(373, 104, 34, cLine);
    Brain.Screen.setFont(prop30);
    gTextC(373, 114, 17, cText, "%d", t.sThr);
    Brain.Screen.setFont(mono12);
    gTextC(373, 150, 8, cMuted, "PEAK %d%%", t.peak);

    // power
    gCard(272, 168, 202, 64, "POWER");
    gText(282, 202, cMuted, "BAT");
    barG(318, 192, 118, 10, t.bat, t.bat <= 20 ? cDangerRgb : cAccentRgb, t.bat <= 20 ? cDangerRgb : cGoodRgb);
    gText(442, 202, cText, "%3d%%", t.bat);
    gText(282, 222, cMuted, "AMP");
    barG(318, 212, 118, 10, t.totalDa * 100 / 80, cAccentRgb, cWarnRgb);
    gText(442, 222, cText, "%d.%dA", t.totalDa / 10, t.totalDa % 10);
}

void pageMotors(const Tel& t) {
    const int cxs[4] = {76, 278, 76, 278};
    const int cys[4] = {32, 134, 32, 134};
    const int w = 196, h = 98;
    // order on screen: LEFT A, RIGHT A top row; LEFT B, RIGHT B bottom row
    const int order[4] = {0, 2, 1, 3};
    for (int slot = 0; slot < 4; ++slot) {
        int i = order[slot];
        int x = slot % 2 == 0 ? 76 : 278;
        int y = slot < 2 ? 32 : 134;
        (void)cxs;
        (void)cys;
        char title[24];
        snprintf(title, sizeof(title), "P%d %s", motorPorts[i], motorNames[i]);
        gCard(x, y, w, h, title);
        gDot(x + w - 12, y + 12, 4, t.mInst[i] ? cGood : cDanger);

        int rv = t.sRpm[i];
        bool rev = rv < 0;
        arcGauge(x + 38, y + 58, 28, 7, abs(rv) * 100 / 200,
                 rev ? cAccent2Rgb : cAccentRgb, rev ? cWarnRgb : cGoodRgb, -1);
        gTextC(x + 38, y + 62, 8, cText, "%d", rv);
        gTextC(x + 38, y + 86, 8, cMuted, "RPM");

        int x0 = x + 78, bw = w - 86, rx = x + w - 8;
        gText(x0, y + 34, cMuted, "TEMP");
        char tb[12];
        snprintf(tb, sizeof(tb), "%dC", t.mTemp[i]);
        gText(rx - static_cast<int>(strlen(tb)) * 8, y + 34, mix(cMutedRgb, heatRgb(t.mTemp[i]), 256), "%s", tb);
        barG(x0, y + 38, bw, 6, t.mTemp[i] * 100 / 70, cGoodRgb, heatRgb(t.mTemp[i]));

        int ad = abs(t.mDa[i]);
        gText(x0, y + 56, cMuted, "AMPS");
        char ab[12];
        snprintf(ab, sizeof(ab), "%d.%dA", ad / 10, ad % 10);
        gText(rx - static_cast<int>(strlen(ab)) * 8, y + 56, cText, "%s", ab);
        barG(x0, y + 60, bw, 6, ad * 100 / 25, cAccentRgb, ad > 20 ? cDangerRgb : cWarnRgb);

        gText(x0, y + 78, cMuted, "OUT");
        char ob[12];
        snprintf(ob, sizeof(ob), "%d%%", t.mOut[i]);
        gText(rx - static_cast<int>(strlen(ob)) * 8, y + 78, cText, "%s", ob);
        sbarG(x0, y + 82, bw, 6, t.mOut[i], cAccentRgb, cGoodRgb, cAccent2Rgb, cWarnRgb);
    }
}

void pageGraph(const Tel& t) {
    gCard(76, 32, 398, 202, "TELEMETRY");
    gText(168, 48, cAccent, "L %4d", (t.mRpm[0] + t.mRpm[1]) / 2);
    gText(250, 48, cAccent2, "R %4d", (t.mRpm[2] + t.mRpm[3]) / 2);
    gText(332, 48, cWarn, "A %d.%dA", t.totalDa / 10, t.totalDa % 10);

    const int gx = 84, gy = 58, gw = 382, gh = 164;
    gRect(gx, gy, gw, gh, cBg);
    gBox(gx, gy, gw, gh, cLine);
    for (int i = 1; i < 4; ++i) {
        for (int x = gx + 2; x < gx + gw; x += 8) {
            gRect(x, gy + i * gh / 4, 3, 1, i == 2 ? cMuted : cLine);
        }
    }
    gText(gx + 4, gy + 12, cMuted, "+200");
    gText(gx + 4, gy + gh / 2 - 3, cMuted, "0");
    gText(gx + 4, gy + gh - 4, cMuted, "-200");

    int mid = gy + gh / 2;
    int prevL = mid, prevR = mid, prevA = gy + gh;
    int lastL = mid, lastR = mid;
    for (int i = 0; i < kHist; ++i) {
        int idx = (histHead + i) % kHist;
        int l = clampInt(histL[idx], -200, 200);
        int r = clampInt(histR[idx], -200, 200);
        int a = clampInt(histA[idx], 0, 80);
        int yl = mid - l * (gh / 2 - 4) / 200;
        int yr = mid - r * (gh / 2 - 4) / 200;
        int ya = gy + gh - 2 - a * (gh - 6) / 80;
        int x = gx + 2 + i * 4;
        gLine(x, ya, x, gy + gh - 2, cWarnDim);
        gLine(x + 1, ya, x + 1, gy + gh - 2, cWarnDim);
        if (i > 0) {
            gLine(x - 4, prevA, x, ya, cWarn);
            gLine(x - 4, prevL, x, yl, cAccent);
            gLine(x - 4, prevL + 1, x, yl + 1, cAccent);
            gLine(x - 4, prevR, x, yr, cAccent2);
            gLine(x - 4, prevR + 1, x, yr + 1, cAccent2);
        }
        prevL = yl;
        prevR = yr;
        prevA = ya;
        lastL = yl;
        lastR = yr;
    }
    int ex = gx + 2 + (kHist - 1) * 4;
    int pr = 3 + ((uiFrame / 4) % 2);
    gDot(ex, lastL, pr, cAccent);
    gDot(ex, lastR, pr, cAccent2);
}

void pageInput(const Tel& t) {
    gCard(76, 32, 112, 200, "L STICK");
    stickBox(82, 56, 100, t.rawLx, t.rawFwd, false, trailX[0], trailY[0], kTrail);
    gText(84, 178, cMuted, "X");
    gText(110, 178, cText, "%4d", t.rawLx);
    gText(84, 196, cMuted, "Y");
    gText(110, 196, cText, "%4d", t.rawFwd);

    gCard(194, 32, 112, 200, "R STICK");
    stickBox(200, 56, 100, t.rawTurn, t.rawRy, false, trailX[1], trailY[1], kTrail);
    gText(202, 178, cMuted, "X");
    gText(228, 178, cText, "%4d", t.rawTurn);
    gText(202, 196, cMuted, "Y");
    gText(228, 196, cText, "%4d", t.rawRy);

    gCard(312, 32, 162, 122, "BUTTONS");
    for (int i = 0; i < 12; ++i) {
        int bx = 318 + (i % 4) * 38;
        int by = 46 + (i / 4) * 34;
        bool on = t.btn[i] != 0;
        gRect(bx, by, 34, 28, on ? cAccent : cBg);
        gBox(bx, by, 34, 28, on ? cText : cLine);
        if (on) gBox(bx - 1, by - 1, 36, 30, cAccent);
        gTextC(bx + 17, by + 18, 8, on ? cBg : cText, "%s", buttonNames[i]);
    }

    gCard(312, 162, 162, 70, "DEADZONE");
    gBox(320, 188, 36, 38, cLine);
    gBox(430, 188, 36, 38, cLine);
    gText(334, 212, cText, "-");
    gText(444, 212, cText, "+");
    Brain.Screen.setFont(prop20);
    gTextC(393, 214, 11, cAccent, "%d%%", driveDeadbandPct);
    Brain.Screen.setFont(mono12);
}

void statusRow(int x, int y, const char* label, int state, const char* value) {
    color c = state == 1 ? cGood : state == 0 ? cDanger : cMuted;
    gDot(x + 4, y - 4, 4, c);
    gText(x + 16, y, cText, "%s", label);
    gText(x + 160 - static_cast<int>(strlen(value)) * 8, y, c, "%s", value);
}

void pageSystem(const Tel& t) {
    gCard(76, 32, 190, 200, "POWER");
    bool low = t.bat <= 20;
    arcGauge(171, 104, 46, 9, t.bat, low ? cDangerRgb : cAccentRgb, low ? cWarnRgb : cGoodRgb, -1);
    gRing(171, 104, 31, cLine);
    Brain.Screen.setFont(prop30);
    gTextC(171, 114, 17, cText, "%d", t.bat);
    Brain.Screen.setFont(mono12);
    gTextC(171, 146, 8, cMuted, "BATTERY");
    gText(84, 172, cMuted, "VOLTAGE");
    gText(176, 172, cText, "%d mV", t.mv);
    gText(84, 190, cMuted, "BAT AMPS");
    gText(176, 190, cText, "%d.%d A", t.bda / 10, abs(t.bda % 10));
    gText(84, 208, cMuted, "BAT TEMP");
    gText(176, 208, t.btemp >= 50 ? cDanger : t.btemp >= 40 ? cWarn : cText, "%d C", t.btemp);
    gText(84, 226, cMuted, "UPTIME");
    gText(176, 226, cText, "%02d:%02d", t.runtime / 60, t.runtime % 60);

    gCard(272, 32, 202, 200, "STATUS");
    char buf[16];
    statusRow(284, 66, "CONTROLLER", t.ctrl ? 1 : 0, t.ctrl ? "OK" : "LOST");
    snprintf(buf, sizeof(buf), "%d/4", t.online);
    statusRow(284, 88, "MOTORS", t.online == 4 ? 1 : 0, buf);
    statusRow(284, 110, "FIELD", t.field ? 1 : 2, t.field ? "FIELD" : "LOCAL");
    statusRow(284, 132, "COMP SW", t.sw ? 1 : 2, t.sw ? "YES" : "NO");
    statusRow(284, 154, "SD CARD", t.sd ? 1 : 2, t.sd ? "READY" : "NONE");
    snprintf(buf, sizeof(buf), "P%d %dC", motorPorts[t.hot], t.mTemp[t.hot]);
    statusRow(284, 176, "HOT MOTOR", t.mTemp[t.hot] >= 60 ? 0 : t.mTemp[t.hot] >= 45 ? 2 : 1, buf);
    bool blink = (uiFrame / 6) % 2 == 0;
    color wc = t.warn > 0 ? (blink ? cDanger : cWarn) : cGood;
    gBox(284, 192, 178, 30, wc);
    if (t.warn > 0) gText(296, 212, wc, "%d WARNING%s ACTIVE", t.warn, t.warn > 1 ? "S" : "");
    else gText(296, 212, wc, "ALL SYSTEMS NOMINAL");
}

void drawUI() {
    static int lastTab = -1;
    static float sL = 0, sR = 0, sT = 0, sRpm[4] = {0, 0, 0, 0}, peak = 0;
    Tel t;
    readTel(t);
    ++uiFrame;
    if (uiFrame % 3 == 0) pushHistory(t);
    if (uiFrame % 2 == 0) {
        pushTrail(0, t.rawLx, t.rawFwd);
        pushTrail(1, t.rawTurn, t.rawRy);
    }

    // easing
    sL += (t.left - sL) * 0.35f;
    sR += (t.right - sR) * 0.35f;
    sT += (t.throttle - sT) * 0.30f;
    for (int i = 0; i < 4; ++i) sRpm[i] += (t.mRpm[i] - sRpm[i]) * 0.30f;
    t.sLeft = rnd(sL);
    t.sRight = rnd(sR);
    t.sThr = rnd(sT);
    for (int i = 0; i < 4; ++i) t.sRpm[i] = rnd(sRpm[i]);
    if (t.sThr > peak) peak = static_cast<float>(t.sThr);
    else peak -= 0.4f;
    if (peak < 0) peak = 0;
    t.peak = rnd(peak);
    for (int i = 0; i < 4; ++i) {
        float out = i < 2 ? sL : sR;
        wheelPhase[i] = fmodf(wheelPhase[i] + out / 25.0f + 600.0f, 6.0f);
    }

    if (selectedTab != lastTab) {
        transFrames = 8;
        lastTab = selectedTab;
    }

    Brain.Screen.setFillColor(cBg);
    Brain.Screen.clearScreen(cBg);
    Brain.Screen.setFont(mono12);
    drawChrome(t);
    switch (selectedTab) {
        case 0: pageDash(t); break;
        case 1: pageMotors(t); break;
        case 2: pageGraph(t); break;
        case 3: pageInput(t); break;
        default: pageSystem(t); break;
    }

    // page transition: wipe reveal left -> right
    if (transFrames > 0) {
        int rx = kSideW + 1 + (8 - transFrames) * 52;
        gRect(rx, 27, 480 - rx, 213, cBg);
        gRect(rx, 27, 2, 213, cAccent);
        --transFrames;
    }
    Brain.Screen.render();
}

/* ---------------------------------- touch --------------------------------- */

void handleScreenTouch() {
    static bool wasTouching = false;
    bool touching = Brain.Screen.pressing();
    if (touching && !wasTouching) {
        int tx = Brain.Screen.xPosition();
        int ty = Brain.Screen.yPosition();
        if (tx < kSideW && ty >= kTabTop) {
            int idx = (ty - kTabTop) / kTabH;
            if (idx >= 0 && idx < 5) selectedTab = idx;
        } else if (selectedTab == 3 && ty >= 188 && ty <= 226) {
            if (tx >= 320 && tx <= 356 && driveDeadbandPct > 0) --driveDeadbandPct;
            else if (tx >= 430 && tx <= 466 && driveDeadbandPct < 25) ++driveDeadbandPct;
        }
    }
    wasTouching = touching;
}

/* ------------------------------- boot sequence ----------------------------- */

void bootAnimation() {
    const char* labels[8] = {"MOTOR P1", "MOTOR P2", "MOTOR P3", "MOTOR P4",
                             "CONTROLLER", "BATTERY", "SD CARD", "FIELD LINK"};
    int state[8];
    char detail[8][14];
    for (int i = 0; i < 4; ++i) {
        bool ok = motorDevices[i]->installed();
        state[i] = ok ? 1 : 0;
        snprintf(detail[i], sizeof(detail[i]), "%s", ok ? "ONLINE" : "MISSING");
    }
    bool ctrlOk = Controller1.installed();
    state[4] = ctrlOk ? 1 : 0;
    snprintf(detail[4], sizeof(detail[4]), "%s", ctrlOk ? "LINKED" : "NOT FOUND");
    int bat = clampInt(Brain.Battery.capacity(pct), 0, 100);
    state[5] = bat > 20 ? 1 : 0;
    snprintf(detail[5], sizeof(detail[5]), "%d%%", bat);
    bool sdOk = Brain.SDcard.isInserted();
    state[6] = sdOk ? 1 : 2;
    snprintf(detail[6], sizeof(detail[6]), "%s", sdOk ? "READY" : "NONE");
    bool fieldOk = Competition.isFieldControl();
    state[7] = 2;
    snprintf(detail[7], sizeof(detail[7]), "%s", fieldOk ? "FIELD" : "LOCAL");

    const int frames = 84;
    for (int f = 0; f <= frames; ++f) {
        Brain.Screen.setFillColor(cBg);
        Brain.Screen.clearScreen(cBg);

        // falling hex rain
        Brain.Screen.setFont(mono12);
        for (int col = 0; col < 16; ++col) {
            int speed = 3 + (col * 5) % 5;
            int head = (f * speed * 2 + col * 53) % 300 - 20;
            for (int k = 0; k < 6; ++k) {
                int y = head - k * 12;
                if (y < 8 || y > 236) continue;
                color c = k == 0 ? mix(cRainRgb, cAccentRgb, 140) : mix(cBgRgb, cRainRgb, (6 - k) * 256 / 6);
                gText(6 + col * 30, y, c, "%X", (f * 3 + col * 7 + k * 5) & 15);
            }
        }

        // logo band
        gRect(0, 40, 480, 80, cBg);
        int ul = f * 14;
        if (ul > 480) ul = 480;
        gLine(240 - ul / 2, 40, 240 + ul / 2, 40, cAccent);
        gLine(240 - ul / 2, 120, 240 + ul / 2, 120, cAccent);
        int letters = f / 3;
        if (letters > 6) letters = 6;
        char logo[8];
        snprintf(logo, sizeof(logo), "%.*s", letters, "VEXTOP");
        Brain.Screen.setFont(prop60);
        bool glitch = f > 22 && (f % 9) < 2;
        if (glitch) {
            gText(136, 106, cAccent2, "%s", logo);
            gText(146, 102, cAccent, "%s", logo);
        }
        gText(141, 104, cText, "%s", logo);
        Brain.Screen.setFont(mono12);

        // checks
        if (f > 20) gText(28, 138, cMuted, "V5 ROBOT MONITOR  //  SYSTEM CHECK");
        gText(400, 138, cAccent, "%3d%%", f * 100 / frames);
        for (int i = 0; i < 8; ++i) {
            int start = 26 + i * 6;
            if (f < start) continue;
            int x = 40 + (i / 4) * 220;
            int y = 160 + (i % 4) * 16;
            bool pending = f < start + 3;
            color c = pending ? cMuted : state[i] == 1 ? cGood : state[i] == 0 ? cDanger : cMuted;
            gRect(x - 12, y - 8, 6, 6, c);
            gText(x, y, cText, "%s", labels[i]);
            gText(x + 100, y, c, "%s", pending ? "....." : detail[i]);
        }

        barG(28, 224, 424, 10, f * 100 / frames, cAccentRgb, cAccent2Rgb);
        Brain.Screen.render();
        this_thread::sleep_for(28);
    }

    // flash + ring burst
    Brain.Screen.setFillColor(cAccent);
    Brain.Screen.clearScreen(cAccent);
    Brain.Screen.render();
    this_thread::sleep_for(40);
    for (int i = 0; i < 10; ++i) {
        Brain.Screen.setFillColor(cBg);
        Brain.Screen.clearScreen(cBg);
        gRing(240, 120, 16 + i * 16, i % 2 ? cAccent : cAccent2);
        gRing(240, 120, 8 + i * 9, cLine);
        gRect(0, 96, 480, 48, cBg);
        Brain.Screen.setFont(prop30);
        gTextC(240, 128, 17, cAccent, "SYSTEM ONLINE");
        Brain.Screen.setFont(mono12);
        gLine(240 - i * 16, 136, 240 + i * 16, 136, cAccent2);
        Brain.Screen.render();
        this_thread::sleep_for(36);
    }

    // diagonal wipe out
    for (int s = 0; s <= 12; ++s) {
        for (int i = 0; i < 6; ++i) gRect(0, i * 40, clampInt(s * 80 - i * 60, 0, 480), 40, cAccent);
        Brain.Screen.render();
        this_thread::sleep_for(16);
    }
    for (int s = 0; s <= 12; ++s) {
        gRect(0, 0, 480, 240, cAccent);
        for (int i = 0; i < 6; ++i) gRect(0, i * 40, clampInt(s * 80 - i * 60, 0, 480), 40, cBg);
        Brain.Screen.render();
        this_thread::sleep_for(16);
    }
}

void matchOutro() {
    Tel t;
    readTel(t);
    const int frames = 28;
    for (int frame = 0; frame <= frames; ++frame) {
        Brain.Screen.setFillColor(cBg);
        Brain.Screen.clearScreen(cBg);

        gRect(0, 0, 4, 240, cAccent2);
        gLine(12, 26, 468, 26, cLine);
        gText(16, 20, cMuted, "FIELD SESSION  //  FINAL REPORT");

        int reveal = frame * 100 / frames;
        arcGauge(105, 119, 65, 8, t.bat * reveal / 100,
                 cAccentRgb, t.bat <= 20 ? cDangerRgb : cGoodRgb, -1);
        gRing(105, 119, 48, cLine);
        Brain.Screen.setFont(prop30);
        gTextC(105, 116, 17, cText, "%d%%", t.bat);
        Brain.Screen.setFont(mono12);
        gTextC(105, 143, 8, cMuted, "BATTERY");

        gCard(190, 38, 278, 174, "MATCH COMPLETE");
        gText(204, 76, cMuted, "MOTORS ONLINE");
        gText(366, 76, t.online == 4 ? cGood : cDanger, "%d / 4", t.online);
        gText(204, 101, cMuted, "HOT MOTOR");
        gText(366, 101, t.mTemp[t.hot] >= 60 ? cDanger :
              t.mTemp[t.hot] >= 45 ? cWarn : cText, "P%d  %dC", motorPorts[t.hot], t.mTemp[t.hot]);
        gText(204, 126, cMuted, "BATTERY VOLTAGE");
        gText(366, 126, cText, "%d mV", t.mv);
        gText(204, 151, cMuted, "MOTOR CURRENT");
        gText(366, 151, cText, "%d.%dA", t.totalDa / 10, t.totalDa % 10);
        gText(204, 176, cMuted, "SYSTEM STATUS");
        gText(366, 176, t.warn ? cWarn : cGood, t.warn ? "%d WARNINGS" : "NOMINAL", t.warn);

        gLine(16, 221, 464, 221, cLine);
        gText(16, 237, cAccent2, "VEXTOP  //  V5 ROBOTICS");
        gText(366, 237, cMuted, "RUN %02d:%02d", t.runtime / 60, t.runtime % 60);
        Brain.Screen.render();
        this_thread::sleep_for(35);
    }
    this_thread::sleep_for(1200);
}

void initializeDrive() {
    LeftDrive.setStopping(brake);
    RightDrive.setStopping(brake);
    bootAnimation();
}

int main() {
    competition::bStopTasksBetweenModes = true;
    initializeDrive();

    bool wasEnabled = Competition.isEnabled();
    bool outroVisible = false;
    int lastDraw = -100;
    while (1) {
        bool enabled = Competition.isEnabled();
        if (Competition.isDriverControl()) {
            driveArcadeSplit();
        } else {
            LeftDrive.stop();
            RightDrive.stop();
        }

        if (wasEnabled && !enabled) {
            matchOutro();
            outroVisible = true;
        }
        wasEnabled = enabled;

        handleScreenTouch();
        int now = static_cast<int>(Brain.Timer.time(msec));
        if (enabled) outroVisible = false;
        if (!outroVisible && now - lastDraw >= 40) {
            drawUI();
            lastDraw = now;
        }
        this_thread::sleep_for(5);
    }
}
