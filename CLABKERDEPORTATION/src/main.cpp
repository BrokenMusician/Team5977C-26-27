/*----------------------------------------------------------------------------*/
/*    Module:       main.cpp                                                  */
/*    Author:       kodie                                                     */
/*    Description:  V5 project - VEXTOP v3 (HUD edition) - Ultra Optimized    */
/*----------------------------------------------------------------------------*/

#include "vex.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>

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

// Ultra-smooth adaptive render: 33ms (30Hz) driving, 66ms (15Hz) idle, 16ms (60Hz) transitions
constexpr int kRenderIntervalMs = 33;
constexpr int kIdleRenderIntervalMs = 66;
constexpr int kTransitionRenderIntervalMs = 16;
int lastRenderTime = 0;
bool wasDriving = false;
int transitionFramesLeft = 0;

const int motorPorts[4] = {1, 2, 3, 4};
const char* const motorNames[4] = {"LEFT A", "LEFT B", "RIGHT A", "RIGHT B"};
const char* const tabNames[7] = {"DASH", "MOTORS", "GRAPH", "INPUT", "SYSTEM", "DEBUG", "DOOM"};
const char* const buttonNames[12] = {"L1", "L2", "R1", "R2", "A", "B", "X", "Y",
                                     "UP", "DN", "LT", "RT"};

constexpr int kSideW = 66;

/* -------------------------------- telemetry ------------------------------- */

struct Tel {
    int rawFwd, rawTurn, rawLx, rawRy, fw, tr, left, right, throttle;
    int bat, mv, bda, btemp, ctrl, sd, mode, enabled, field, sw, runtime;
    int mTemp[4], mRpm[4], mDa[4], mInst[4], mOut[4];
    int online, hot, warn, totalDa;
    int btn[12];
    // smoothed for display - using float for ultra-smooth interpolation
    float sLeft, sRight, sThr, sRpm[4];
    int peak;
};
constexpr int kTabH = 42;
constexpr int kTabTop = 28;

// Reduced history for memory/CPU savings (60 samples @ 30Hz = 2 seconds)
constexpr int kHist = 60;
constexpr int kTrail = 8;  // Increased for smoother trails

struct Rgb { int r, g, b; };
#define PAL(name, R, G, B) const Rgb name##Rgb = {R, G, B}; const color name(R, G, B);
// Premium color palette with deeper contrast
PAL(cBg, 6, 8, 14)
PAL(cPanel, 12, 16, 26)
PAL(cLine, 32, 42, 60)
PAL(cText, 240, 248, 255)
PAL(cMuted, 96, 112, 136)
PAL(cAccent, 0, 210, 255)
PAL(cAccent2, 255, 50, 140)
PAL(cGood, 50, 255, 140)
PAL(cWarn, 255, 196, 32)
PAL(cDanger, 255, 50, 60)
PAL(cRain, 0, 40, 56)
// Premium accent colors
PAL(cGold, 255, 215, 0)
PAL(cCyan, 0, 255, 255)

// Dynamic theme system
enum ThemeMode { THEME_CYBER, THEME_NEON, THEME_FIRE, THEME_ICE, THEME_GOLD, THEME_COUNT };
ThemeMode currentTheme = THEME_CYBER;
int themeTransitionFrame = 0;
bool themeTransitioning = false;

// Chromatic aberration offset
int chromaticOffset = 0;
bool chromaticEnabled = true;

// Vignette strength
float vignetteStrength = 0.3f;

// Scanline intensity
float scanlineIntensity = 0.15f;

// Bloom threshold
int bloomThreshold = 200;

// CRT curvature
float crtCurvature = 0.02f;

// Glitch effect
int glitchTimer = 0;
int glitchIntensity = 0;

// Post-processing pipeline
bool postProcessEnabled = true;
bool bloomEnabled = true;
int bloomIterations = 3;
float bloomIntensity = 0.5f;
bool lensFlareEnabled = true;
bool filmGrainEnabled = true;
float filmGrainIntensity = 0.05f;
bool chromaticAberrationEnabled = true;
float chromaticAberrationStrength = 1.0f;
bool vignetteEnabled = true;
bool scanlinesEnabled = true;
bool ditheringEnabled = true;

// Advanced render targets (simulated with screen buffers)
uint16_t bloomBuffer[480 * 240];
uint16_t sceneBuffer[480 * 240];

// Particle trail system
struct ParticleTrail {
    float x[16], y[16];
    float vx[16], vy[16];
    float life[16];
    color colors[16];
    int count;
    bool active;
};
ParticleTrail particleTrails[8];
int trailIndex = 0;

// Ribbon particles for velocity trails
struct RibbonPoint {
    float x, y, width;
    color c;
    float life;
};
RibbonPoint ribbonPoints[64];
int ribbonCount = 0;

// Shockwave rings
struct Shockwave {
    float x, y, radius, maxRadius;
    color c;
    float life, maxLife;
    int thickness;
    bool active;
};
Shockwave shockwaves[16];
int shockwaveCount = 0;

// Energy orbs
struct EnergyOrb {
    float x, y, vx, vy;
    float radius, targetRadius;
    color c;
    float pulsePhase;
    bool active;
};
EnergyOrb energyOrbs[8];
int orbCount = 0;

// Hologram scanlines
bool hologramMode = false;
int hologramPhase = 0;

// Advanced holographic rendering
struct HologramRenderer {
    bool enabled;
    float scanlineIntensity;
    float flickerIntensity;
    float chromaticAberration;
    float glowIntensity;
    float transparency;
    color tintColor;
    int renderMode; // 0=normal, 1=wireframe, 2=points, 3=scanlines
    float noiseScale;
    float noiseSpeed;
};
HologramRenderer hologramRenderer = {true, 0.3f, 0.1f, 0.5f, 0.8f, 0.9f, {0, 200, 255}, 3, 0.01f, 0.5f};

// Vec3 for 3D math
struct Vec3 {
    float x, y, z;
};

// Volumetric holograms
struct VolumetricHologram {
    Vec3 position;
    Vec3 size;
    float density;
    color color;
    float animationPhase;
    bool active;
    int pattern; // 0=cube, 1=sphere, 2=pyramid, 3=torus, 4=custom
};
VolumetricHologram volumetricHolograms[16];
int volumetricHologramCount = 0;

// Color grading LUT
uint32_t colorLUT[256];
bool colorGradingEnabled = true;

// Advanced color grading
struct ColorGradingSettings {
    bool enabled;
    float contrast;
    float saturation;
    float brightness;
    float gamma;
    float temperature;
    float tint;
    float lift[3];
    float gammaRGB[3];
    float gain[3];
    float shadows[3];
    float midtones[3];
    float highlights[3];
    float vignetteAmount;
    float vignetteMidpoint;
    float vignetteRoundness;
    float vignetteFeather;
    bool filmGrainEnabled;
    float filmGrainIntensity;
    float filmGrainSize;
    bool ditheringEnabled;
    float ditheringStrength;
};
ColorGradingSettings colorGradingSettings = {true, 1.1f, 1.05f, 0.0f, 2.2f, 0.0f, 0.0f, {0,0,0}, {1,1,1}, {1,1,1}, {0,0,0}, {0,0,0}, {0,0,0}, 0.3f, 0.5f, 1.0f, 0.5f, true, 0.05f, 1.0f, true, 0.5f};

// LUT textures
uint32_t lut3D[16][16][16];
bool lut3DEnabled = false;

// Performance metrics
int maxFrameTime = 0;
int avgFrameTime = 0;
int frameTimeSamples = 0;

// Advanced performance monitoring
struct PerformanceMonitor {
    uint32_t frameTimes[120];
    int frameTimeIndex;
    uint32_t gpuTime;
    uint32_t cpuTime;
    uint32_t drawCalls;
    uint32_t triangles;
    uint32_t vertices;
    uint32_t textureMemory;
    uint32_t vertexMemory;
    float fps;
    float frameTimeMs;
    float minFrameTime;
    float maxFrameTime;
    int frameCount;
};
PerformanceMonitor perfMonitor;

struct MemoryTracker {
    size_t allocated;
    size_t peakAllocated;
    size_t allocations;
    size_t deallocations;
    struct Allocation {
        void* ptr;
        size_t size;
        const char* file;
        int line;
    } allocations_list[256];
    int allocationCount;
};
MemoryTracker memoryTracker;

// Frame graph for profiling
struct FrameGraph {
    struct Node {
        char name[32];
        uint32_t startTime;
        uint32_t endTime;
        int parent;
        int children[8];
        int childCount;
        color graphColor;
    } nodes[64];
    int nodeCount;
    int currentNode;
    uint32_t frameStart;
};
FrameGraph frameGraph;

// Audio visualization (simulated)
struct AudioBar {
    float height, targetHeight, velocity;
    color c;
    float pulsePhase;
};
AudioBar audioBars[32];
int audioBarCount = 32;
bool audioVizEnabled = true;

// Advanced audio system
struct AudioSource {
    Vec3 position;
    float volume;
    float pitch;
    float radius;
    bool looping;
    bool playing;
    int soundId;
    float fadeIn;
    float fadeOut;
};
AudioSource audioSources[16];
int audioSourceCount = 0;

struct AudioListener {
    Vec3 position;
    Vec3 forward;
    Vec3 up;
    float volume;
};
AudioListener audioListener;

struct AudioReverbZone {
    Vec3 position;
    float radius;
    float density;
    float diffusion;
    float gain;
    float gainHF;
    float decayTime;
    float decayHFRatio;
    float reflectionsGain;
    float reflectionsDelay;
    float lateReverbGain;
    float lateReverbDelay;
    bool active;
};
AudioReverbZone audioReverbZones[8];
int audioReverbZoneCount = 0;

// 3D perspective system
struct Mat4 { float m[16]; };
Mat4 viewMatrix, projMatrix;
float cameraYaw = 0, cameraPitch = 0, cameraDist = 300;
bool perspective3DEnabled = true;

// Advanced 3D rendering system
struct Mesh3D {
    Vec3* vertices;
    int* indices;
    int vertexCount;
    int indexCount;
    Vec3 position;
    Vec3 rotation;
    Vec3 scale;
    color wireColor;
    color fillColor;
    bool wireframe;
    bool visible;
};
Mesh3D meshes3D[16];
int mesh3DCount = 0;

struct Camera3D {
    Vec3 position;
    Vec3 target;
    Vec3 up;
    float fov;
    float nearPlane;
    float farPlane;
    float aspect;
    Mat4 viewMatrix;
    Mat4 projMatrix;
    bool orthographic;
    float orthoSize;
};
Camera3D camera3D;

struct Light3D {
    Vec3 position;
    Vec3 direction;
    color color;
    float intensity;
    float range;
    float innerAngle;
    float outerAngle;
    int type; // 0=point, 1=directional, 2=spot
    bool enabled;
    bool castShadows;
};
Light3D lights3D[8];
int light3DCount = 0;

// Volumetric lighting
struct LightShaft {
    Vec3 start;
    Vec3 end;
    color color;
    float intensity;
    int samples;
    bool active;
};
LightShaft lightShafts[16];
int lightShaftCount = 0;

// Fog system
struct FogSettings {
    bool enabled;
    color color;
    float density;
    float start;
    float end;
    int mode; // 0=linear, 1=exp, 2=exp2
};
FogSettings fogSettings = {true, {10, 12, 18}, 0.001f, 50.0f, 500.0f, 1};

// Weather/atmosphere system
enum WeatherType { WEATHER_NONE, WEATHER_RAIN, WEATHER_SNOW, WEATHER_STORM, WEATHER_AURORA };
WeatherType currentWeather = WEATHER_NONE;
struct WeatherParticle {
    float x, y, vx, vy, life;
    color c;
    float size;
    bool active;
};
WeatherParticle weatherParticles[200];
int weatherParticleCount = 0;

// Advanced weather effects
struct CloudLayer {
    float x, y, speedX, speedY;
    float scale;
    color tint;
    float opacity;
    int textureType;
    bool active;
};
CloudLayer cloudLayers[8];
int cloudLayerCount = 0;

struct LightningBolt {
    Vec3 start;
    Vec3 end;
    Vec3* segments;
    int segmentCount;
    float life;
    float maxLife;
    color color;
    float intensity;
    bool active;
};
LightningBolt lightningBolts[16];
int lightningBoltCount = 0;

// Aurora borealis
struct AuroraBand {
    float y;
    float height;
    float speed;
    color color1;
    color color2;
    float phase;
    float waveSpeed;
    bool active;
};
AuroraBand auroraBands[8];
int auroraBandCount = 0;

// Particle field system (for snow, rain, ash, etc.)
struct ParticleField {
    Vec3 boundsMin;
    Vec3 boundsMax;
    Vec3 gravity;
    Vec3 wind;
    float spawnRate;
    float particleLife;
    float particleSize;
    color particleColor;
    int maxParticles;
    bool active;
    int type; // 0=rain, 1=snow, 2=ash, 3=leaves, 4=sparks
};
ParticleField particleFields[4];
int particleFieldCount = 0;

// Advanced UI transitions
enum TransitionType { TRANS_NONE, TRANS_WIPE, TRANS_FADE, TRANS_SLIDE, TRANS_ZOOM, TRANS_ROTATE, TRANS_GLITCH };
TransitionType currentTransition = TRANS_WIPE;
float transitionProgress = 0;
int transitionDuration = 300;

// Shader-like post-processing effects
struct PostProcessEffect {
    bool enabled;
    float intensity;
    float time;
    int type; // 0=bloom, 1=chromatic, 2=vignette, 3=filmGrain, 4=colorGrade, 5=scanlines, 6=CRT, 7=lensFlare, 8=depthOfField, 9=motionBlur
    float params[8];
};
PostProcessEffect postProcessEffects[16];
int postProcessEffectCount = 0;

// Screen-space reflections
struct SSRSettings {
    bool enabled;
    float maxDistance;
    float thickness;
    float fadeStart;
    float fadeEnd;
    int maxSteps;
    float roughness;
};
SSRSettings ssrSettings = {true, 200.0f, 0.1f, 0.5f, 1.0f, 32, 0.5f};

// Ambient occlusion
struct SSAOSettings {
    bool enabled;
    float radius;
    float intensity;
    int sampleCount;
    float bias;
    float power;
};
SSAOSettings ssaoSettings = {true, 50.0f, 1.0f, 16, 0.01f, 2.0f};

// Tone mapping
struct ToneMappingSettings {
    bool enabled;
    int mode; // 0=Reinhard, 1=ACES, 2=Uncharted2, 3=Custom
    float exposure;
    float gamma;
    float whitePoint;
};
ToneMappingSettings toneMappingSettings = {true, 1, 1.0f, 2.2f, 1.0f};

// FXAA anti-aliasing
struct FXAASettings {
    bool enabled;
    float spanMax;
    float reduceMin;
    float reduceMul;
};
FXAASettings fxaaSettings = {true, 8.0f, 1.0f/128.0f, 1.0f/8.0f};

// UI Animation system
struct UIAnimation {
    float* target;
    float startValue;
    float endValue;
    float duration;
    float elapsed;
    int easingType;
    bool active;
    bool loop;
    bool pingPong;
    void (*onComplete)();
};
UIAnimation uiAnimations[64];
int uiAnimationCount = 0;

// Advanced UI components
struct UITooltip {
    char text[128];
    float x, y;
    float targetX, targetY;
    float alpha;
    float fadeSpeed;
    bool visible;
    int anchor; // 0=top, 1=bottom, 2=left, 3=right
};
UITooltip uiTooltip;

struct UIContextMenu {
    char items[16][32];
    int itemCount;
    float x, y;
    float width;
    float alpha;
    bool visible;
    int selectedItem;
    void (*callbacks[16])();
};
UIContextMenu uiContextMenu;

struct UISlider {
    float* value;
    float min, max;
    float x, y, width;
    char label[32];
    color trackColor;
    color fillColor;
    color handleColor;
    bool dragging;
    bool hover;
};
UISlider uiSliders[16];
int uiSliderCount = 0;

struct UIToggle {
    bool* value;
    float x, y, size;
    char label[32];
    color onColor;
    color offColor;
    float animation;
    bool hover;
};
UIToggle uiToggles[16];
int uiToggleCount = 0;

struct UIDropdown {
    char items[16][32];
    int itemCount;
    int* selectedIndex;
    float x, y, width;
    float animation;
    bool open;
    bool hover;
    int hoverIndex;
};
UIDropdown uiDropdowns[8];
int uiDropdownCount = 0;

// Advanced telemetry visualizations
struct TelemetryGraph {
    float values[120];
    int head;
    color c;
    float minVal, maxVal;
    bool autoScale;
};
TelemetryGraph telemetryGraphs[8];

// Advanced telemetry - 3D graphs
struct TelemetryGraph3D {
    Vec3 points[200];
    int pointCount;
    color color;
    float scale;
    Vec3 rotation;
    bool wireframe;
    bool showGrid;
    bool showAxes;
};
TelemetryGraph3D telemetryGraphs3D[4];
int telemetryGraph3DCount = 0;

// Particle text system
struct TextParticle {
    float x, y, vx, vy;
    char text[16];
    color c;
    float life, maxLife;
    float scale;
    bool active;
};
TextParticle textParticles[32];
int textParticleCount = 0;

// HUD element animations
struct HUDAnim {
    float progress, target;
    float velocity;
    float delay;
    bool active;
    int easingType;
};
HUDAnim hudAnims[32];

// Floating damage/stat numbers
struct FloatingNumber {
    float x, y, vy;
    char text[16];
    color c;
    float life, maxLife;
    float scale, scaleVel;
    bool active;
};
FloatingNumber floatingNumbers[16];
int floatingNumberCount = 0;

// Radial menu system
struct RadialMenuItem {
    char label[16];
    color c;
    float angle;
    bool enabled;
    void (*callback)();
};
RadialMenuItem radialMenuItems[8];
int radialMenuCount = 0;
bool radialMenuOpen = false;
float radialMenuProgress = 0;

// Notification system
struct Notification {
    char text[64];
    color c;
    float life, maxLife;
    float y, targetY;
    bool active;
};
Notification notifications[8];
int notificationCount = 0;

// Achievement system
struct Achievement {
    char name[32];
    char desc[64];
    color c;
    bool unlocked;
    float unlockTime;
    float displayProgress;
};
Achievement achievements[16];
int achievementCount = 0;

// Combo system
int comboCount = 0;
float comboTimer = 0;
int maxCombo = 0;

// Screen space effects
bool motionBlurEnabled = false;
float motionBlurStrength = 0.5f;
bool depthOfFieldEnabled = false;
float dofFocus = 120.0f;
float dofRange = 50.0f;

// Procedural background
struct BGElement {
    float x, y, vx, vy;
    float size, rotation, rotSpeed;
    color c;
    int type; // 0=circle, 1=square, 2=triangle, 3=star
    bool active;
};
BGElement bgElements[64];
int bgElementCount = 0;

// Performance profiler
struct ProfilerSection {
    const char* name;
    uint32_t startTime;
    uint32_t totalTime;
    uint32_t maxTime;
    uint32_t callCount;
    color graphColor;
};
ProfilerSection profilerSections[16];
int profilerCount = 0;
bool profilerEnabled = true;
int profilerFrame = 0;

// Easter eggs
int konamiCode[10] = {0, 0, 1, 1, 2, 2, 3, 3, 4, 5}; // Up Up Down Down Left Left Right Right A B
int konamiIndex = 0;
bool konamiActivated = false;
int easterEggTimer = 0;
enum EasterEggType { EGG_NONE, EGG_KONAMI, EGG_RAINBOW, EGG_MATRIX, EGG_RETRO, EGG_HYPER, EGG_CYBERPUNK, EGG_GLITCH_ART, EGG_NEON_DREAMS, EGG_VOID, EGG_HOLOGRAM, EGG_SYNTHWAVE, EGG_DIGITAL_RAIN, EGG_PLASMA };
EasterEggType activeEasterEgg = EGG_NONE;

// Holographic UI elements
struct HologramElement {
    float x, y, z;
    float scale, rotation;
    color c;
    int type; // 0=text, 1=icon, 2=graph, 3=model
            float pulsePhase;
            bool active;
        };
        HologramElement hologramElements[32];
        int hologramCount = 0;

        // Minimap system
    bool minimapEnabled = true;
    int minimapX = 400, minimapY = 20, minimapSize = 60;

    // Spectator mode
    bool spectatorMode = false;
    int spectatorTarget = -1;

    // Replay system
    struct ReplayFrame {
        Tel tel;
        int frame;
    };
    ReplayFrame replayBuffer[1800]; // 60 seconds at 30fps
    int replayHead = 0;
    int replayCount = 0;
    bool replayRecording = true;
    bool replayPlaying = false;
    int replayPlaybackIndex = 0;

    // Color palette generator
    uint32_t paletteCache[16][16]; // 16 palettes, 16 colors each

    // Time dilation
    float timeScale = 1.0f;
    float timeScaleTarget = 1.0f;
    float timeScaleVel = 0.0f;

    // Parallax background layers
struct ParallaxLayer {
    float x, y, speedX, speedY;
    int textureType; // 0=stars, 1=grid, 2=particles, 3=nebula
    float scale;
    color tint;
    float opacity;
};
ParallaxLayer parallaxLayers[5];

// Input visualization
struct InputViz {
    float lx, ly, rx, ry;
    bool buttons[12];
    float pressAnim[12];
};
InputViz inputViz;

// Combo/chain system for driving
int driftCombo = 0;
float driftAngle = 0;
bool isDrifting = false;

// Precomputed arc gauge geometry (48 segments for ultra-smooth arcs)
constexpr int kArcSegments = 48;
struct ArcPt { int x1, y1, x2, y2; };
ArcPt arcLUT[kArcSegments][2];
int arcCenterX = 0, arcCenterY = 0, arcRadius = 0, arcThick = 0;
bool arcLUTValid = false;

struct ArcPtSmall { int x, y; };
ArcPtSmall arcRingLUT[kArcSegments];

// Frame timing for buttery smooth animation
uint32_t frameStartTime = 0;
float deltaTime = 16.67f; // Target 60fps = 16.67ms

// Particle system (defined early for forward references)
struct Particle {
    float x, y, vx, vy, life, maxLife;
    color c;
    int size;
    bool active;
};

constexpr int kMaxParticles = 120;
Particle particles[kMaxParticles];
int particleCount = 0;
int shakeX = 0, shakeY = 0, shakeTime = 0;

void spawnParticles(int cx, int cy, int count, color c, float speedMin, float speedMax, int sizeMin, int sizeMax, int lifeMs);
void updateParticles(int dt);
void drawParticles();
void triggerShake(int intensity, int durationMs);
void applyShake();

// Advanced effects forward declarations
void initColorLUT();
void updateAdvancedEffects(int dt);
void drawAdvancedEffects();
void triggerVelocityEffects(const Tel& t);
void triggerThemeByPerformance(const Tel& t);
void spawnShockwave(float x, float y, float maxRadius, color c, int thickness, int durationMs);
void triggerThemeChange(ThemeMode newTheme);
void triggerGlitch(int intensity, int duration);
void drawVignette();
void drawScanlines(float intensity);
void drawCRTCurvature();
void drawHologramScanlines();
void drawParticleTrails();
void drawRibbons();
void drawShockwaves();
void drawEnergyOrbs();
void updateAdvancedEffects(int dt);
void drawAdvancedEffects();
void triggerVelocityEffects(const Tel& t);
void triggerThemeByPerformance(const Tel& t);
void triggerThemeChange(ThemeMode newTheme);
void triggerGlitch(int intensity, int duration);
void updateGlitch();
void drawVignette();
void drawScanlines(float intensity);
void drawCRTCurvature();
void drawHologramScanlines();
void drawParticleTrails();
void drawRibbons();
void drawShockwaves();
void drawEnergyOrbs();
void updateParticleTrails(int dt);
void updateRibbons(int dt);
void updateShockwaves(int dt);
void updateEnergyOrbs(int dt);
void spawnParticleTrail(float x, float y, float vx, float vy, color c, int count);
void spawnRibbon(float x, float y, float width, color c);
void spawnShockwave(float x, float y, float maxRadius, color c, int thickness, int durationMs);
void spawnEnergyOrb(float x, float y, float vx, float vy, float radius, color c);
void updateParticleTrails(int dt);
void updateRibbons(int dt);
void updateShockwaves(int dt);
void updateEnergyOrbs(int dt);
void drawParticleTrails();
void drawRibbons();
void drawShockwaves();
void drawEnergyOrbs();
void drawVignette();
void drawScanlines(float intensity);
void drawCRTCurvature();
void drawHologramScanlines();
void initColorLUT();
color applyColorGrade(color c);
void getThemeColors(ThemeMode theme, color& primary, color& secondary, color& accent, color& bg);
void updateThemeTransition();
void triggerThemeChange(ThemeMode newTheme);
void triggerGlitch(int intensity, int duration);
void updateGlitch();
void drawShockwaves();
void drawEnergyOrbs();
void updateParticleTrails(int dt);
void updateRibbons(int dt);
void updateShockwaves(int dt);
void updateEnergyOrbs(int dt);
void spawnParticleTrail(float x, float y, float vx, float vy, color c, int count);
void spawnRibbon(float x, float y, float width, color c);
void spawnShockwave(float x, float y, float maxRadius, color c, int thickness, int durationMs);
void spawnEnergyOrb(float x, float y, float vx, float vy, float radius, color c);
void updateParticleTrails(int dt);
void updateRibbons(int dt);
void updateShockwaves(int dt);
void updateEnergyOrbs(int dt);
void drawParticleTrails();
void drawRibbons();
void drawShockwaves();
void drawEnergyOrbs();
void drawVignette();
void drawScanlines(float intensity);
void drawCRTCurvature();
void drawHologramScanlines();
void initColorLUT();
color applyColorGrade(color c);
void getThemeColors(ThemeMode theme, color& primary, color& secondary, color& accent, color& bg);
void updateThemeTransition();
void triggerThemeChange(ThemeMode newTheme);
void triggerGlitch(int intensity, int duration);
void updateGlitch();
void drawVignette();
void drawScanlines(float intensity);
void drawCRTCurvature();
void drawHologramScanlines();
void drawParticleTrails();
void drawRibbons();
void drawShockwaves();
void drawEnergyOrbs();
void updateAdvancedEffects(int dt);
void drawAdvancedEffects();
void triggerVelocityEffects(const Tel& t);
void triggerThemeByPerformance(const Tel& t);

/* ------------------------------ small helpers ------------------------------ */

inline int clampInt(int v, int lo, int hi) { return v < lo ? lo : v > hi ? hi : v; }
inline int rnd(float v) { return static_cast<int>(v >= 0 ? v + 0.5f : v - 0.5f); }

// Premium easing functions - zero allocation, ultra-fast
inline float easeOutCubic(float t) { return 1.0f - powf(1.0f - t, 3); }
inline float easeOutQuart(float t) { return 1.0f - powf(1.0f - t, 4); }
inline float easeOutExpo(float t) { return t == 1.0f ? 1.0f : 1.0f - powf(2.0f, -10.0f * t); }
inline float easeOutElastic(float t) {
    const float c4 = (2.0f * 3.14159265f) / 3.0f;
    return t == 0.0f ? 0.0f : t == 1.0f ? 1.0f : powf(2.0f, -10.0f * t) * sinf((t * 10.0f - 0.75f) * c4) + 1.0f;
}
inline float easeOutBounce(float t) {
    const float n1 = 7.5625f, d1 = 2.75f;
    if (t < 1.0f / d1) return n1 * t * t;
    else if (t < 2.0f / d1) { t = t - 1.5f / d1; return n1 * t * t + 0.75f; }
    else if (t < 2.5f / d1) { t = t - 2.25f / d1; return n1 * t * t + 0.9375f; }
    else { t = t - 2.625f / d1; return n1 * t * t + 0.984375f; }
}
inline float smoothstep(float t) { return t * t * (3.0f - 2.0f * t); }
inline float smootherstep(float t) { return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f); }

// Fast integer-to-string formatting (avoids snprintf overhead in render loop)
#define FMT_PCT(buf, v) do { \
    int n = (v < 0) ? -v : v; \
    buf[0] = v < 0 ? '-' : ' '; \
    buf[1] = '0' + n / 100; \
    buf[2] = '0' + (n / 10) % 10; \
    buf[3] = '0' + n % 10; \
    buf[4] = '%'; buf[5] = 0; \
} while(0)

#define FMT_TEMP(buf, v) do { \
    int n = (v < 0) ? -v : v; \
    if (n >= 100) { \
        buf[0] = v < 0 ? '-' : ' '; \
        buf[1] = '0' + n / 100; \
        buf[2] = '0' + (n / 10) % 10; \
        buf[3] = '0' + n % 10; \
        buf[4] = 'C'; buf[5] = 0; \
    } else { \
        buf[0] = v < 0 ? '-' : ' '; \
        buf[1] = '0' + n / 10; \
        buf[2] = '0' + n % 10; \
        buf[3] = 'C'; buf[4] = 0; \
    } \
} while(0)

#define FMT_AMP(buf, da) do { \
    int whole = da / 10; \
    int tenth = (da % 10 < 0) ? -(da % 10) : da % 10; \
    buf[0] = '0' + whole / 10; \
    buf[1] = '0' + whole % 10; \
    buf[2] = '.'; \
    buf[3] = '0' + tenth; \
    buf[4] = 'A'; buf[5] = 0; \
} while(0)

#define FMT_RPM(buf, v) do { \
    int n = abs(v); \
    bool neg = v < 0; \
    int idx = neg ? 1 : 0; \
    buf[0] = neg ? '-' : (n >= 10000 ? '0' + n / 10000 : ' '); \
    if (n < 10000 && !neg) idx = 0; \
    buf[idx] = '0' + (n / 1000) % 10; \
    buf[idx+1] = '0' + (n / 100) % 10; \
    buf[idx+2] = '0' + (n / 10) % 10; \
    buf[idx+3] = '0' + n % 10; \
    buf[idx+4] = 0; \
} while(0)

// Integer-only color mix (avoids float in hot path) - optimized with bit shifts
// Integer-only color mix (avoids float in hot path) - optimized with bit shifts
inline color mix8(const Rgb& a, const Rgb& b, int t) {
    t = clampInt(t, 0, 256);
    return color(a.r + ((b.r - a.r) * t >> 8),
                 a.g + ((b.g - a.g) * t >> 8),
                 a.b + ((b.b - a.b) * t >> 8));
}

// Overload for color type
inline color mix8(color a, color b, int t) {
    t = clampInt(t, 0, 256);
    Rgb ra = {(int)((a >> 16) & 0xFF), (int)((a >> 8) & 0xFF), (int)(a & 0xFF)};
    Rgb rb = {(int)((b >> 16) & 0xFF), (int)((b >> 8) & 0xFF), (int)(b & 0xFF)};
    return color(ra.r + ((rb.r - ra.r) * t >> 8),
                 ra.g + ((rb.g - ra.g) * t >> 8),
                 ra.b + ((rb.b - ra.b) * t >> 8));
}

// Premium color mix with alpha blending
inline color mixAlpha(const Rgb& a, const Rgb& b, int t, int alpha) {
    t = clampInt(t, 0, 256);
    alpha = clampInt(alpha, 0, 256);
    int r = a.r + ((b.r - a.r) * t >> 8);
    int g = a.g + ((b.g - a.g) * t >> 8);
    int bl = a.b + ((b.b - a.b) * t >> 8);
    // Apply alpha by mixing with background
    r = (r * alpha + cBgRgb.r * (256 - alpha)) >> 8;
    g = (g * alpha + cBgRgb.g * (256 - alpha)) >> 8;
    bl = (bl * alpha + cBgRgb.b * (256 - alpha)) >> 8;
    return color(r, g, bl);
}

// Helper to extract RGB from color
inline Rgb colorToRgb(color c) {
    return Rgb{(int)((c >> 16) & 0xFF), (int)((c >> 8) & 0xFF), (int)(c & 0xFF)};
}

// Alias for convenience
inline Rgb rgbToColor(color c) { return colorToRgb(c); }

const Rgb& heatRgb(int temp) {
    return temp >= 60 ? cDangerRgb : temp >= 45 ? cWarnRgb : cGoodRgb;
}

// Premium gradient color for RPM/speed visualization
inline color speedColor(float speed, float maxSpeed) {
    float ratio = clampInt(speed * 256 / maxSpeed, 0, 256) / 256.0f;
    if (ratio < 0.33f) return mix8(cGoodRgb, cCyanRgb, ratio * 3 * 256);
    if (ratio < 0.66f) return mix8(cCyanRgb, cGoldRgb, (ratio - 0.33f) * 3 * 256);
    return mix8(cGoldRgb, cDangerRgb, (ratio - 0.66f) * 3 * 256);
}

// Precompute arc gauge lookup table - 48 segments for ultra-smooth arcs
void buildArcLUT(int cx, int cy, int r, int thick) {
    if (arcLUTValid && arcCenterX == cx && arcCenterY == cy && arcRadius == r && arcThick == thick) return;
    arcCenterX = cx; arcCenterY = cy; arcRadius = r; arcThick = thick;
    const double deg2rad = 0.01745329252;
    for (int i = 0; i < kArcSegments; ++i) {
        double a = (135.0 + 270.0 * i / (kArcSegments - 1)) * deg2rad;
        double ca = cos(a), sa = sin(a);
        int ri = r - thick, ro = r;
        arcLUT[i][0] = {cx + int(ri * ca), cy + int(ri * sa), cx + int(ro * ca), cy + int(ro * sa)};
        arcLUT[i][1] = {cx + int(ri * ca) + int(0.03 * -sa), cy + int(ri * sa) + int(0.03 * ca),
                        cx + int(ro * ca) + int(0.03 * -sa), cy + int(ro * sa) + int(0.03 * ca)};
        arcRingLUT[i] = {cx + int((r - 2) * ca), cy + int((r - 2) * sa)};
    }
    arcLUTValid = true;
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

// Premium HUD card with subtle glow and depth
void gCard(int x, int y, int w, int h, const char* title) {
    // Subtle shadow/depth
    gRect(x + 1, y + 1, w, h, color(0, 0, 0));
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

// Premium gradient segmented bar with glow
void barG(int x, int y, int w, int h, int v, const Rgb& lo, const Rgb& hi) {
    v = clampInt(v, 0, 100);
    gRect(x, y, w, h, cBg);
    int segs = (w - 2) / 6;  // Finer segments for smoother gradient
    if (segs < 1) segs = 1;
    int lit = v * segs / 100;
    for (int i = 0; i < lit; ++i) {
        gRect(x + 1 + i * 6, y + 1, 5, h - 2, mix8(lo, hi, segs > 1 ? i * 256 / (segs - 1) : 0));
    }
    // Glow top edge
    if (lit > 0) {
        gLine(x + 1, y, x + 1 + lit * 6, y, mix8(hi, cTextRgb, 180));
    }
    gBox(x, y, w, h, cLine);
}

// Premium signed gradient bar
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
        color c = v < 0 ? mix8(loN, hiN, t) : mix8(loP, hiP, t);
        int px = v < 0 ? center - 1 - (i + 1) * 6 + 1 : center + 2 + i * 6;
        gRect(px, y + 1, 5, h - 2, c);
    }
    // Center glow line
    gLine(center, y, center, y + h, mix8(cMutedRgb, cTextRgb, 100));
    gBox(x, y, w, h, cLine);
}

// Ultra-smooth 270-degree arc gauge - 48 segments, no trig in hot path
void arcGauge(int cx, int cy, int r, int thick, int value, const Rgb& lo, const Rgb& hi, int peak) {
    buildArcLUT(cx, cy, r, thick);
    value = clampInt(value, 0, 100);
    int lit = value * kArcSegments / 100;
    for (int i = 0; i < kArcSegments; ++i) {
        color c = i < lit ? mix8(lo, hi, i * 256 / (kArcSegments - 1)) : cLine;
        Brain.Screen.setPenColor(c);
        Brain.Screen.drawLine(arcLUT[i][0].x1, arcLUT[i][0].y1, arcLUT[i][0].x2, arcLUT[i][0].y2);
        Brain.Screen.drawLine(arcLUT[i][1].x1, arcLUT[i][1].y1, arcLUT[i][1].x2, arcLUT[i][1].y2);
    }
    if (peak >= 0) {
        int p = clampInt(peak, 0, 100) * (kArcSegments - 1) / 100;
        Brain.Screen.setPenColor(cText);
        Brain.Screen.drawLine(arcLUT[p][0].x2, arcLUT[p][0].y2,
                              arcLUT[p][0].x2 + (arcLUT[p][0].x2 - arcLUT[p][0].x1) * 3,
                              arcLUT[p][0].y2 + (arcLUT[p][0].y2 - arcLUT[p][0].y1) * 3);
    }
    // Subtle tick marks at 25%, 50%, 75%
    for (int m = 25; m <= 75; m += 25) {
        int p = m * (kArcSegments - 1) / 100;
        Brain.Screen.setPenColor(cMuted);
        Brain.Screen.drawLine(arcLUT[p][0].x2, arcLUT[p][0].y2,
                              arcLUT[p][0].x2 + (arcLUT[p][0].x2 - arcLUT[p][0].x1) * 2,
                              arcLUT[p][0].y2 + (arcLUT[p][0].y2 - arcLUT[p][0].y1) * 2);
    }
}

int stickPx(int c, int s, int v) { return c + clampInt(v, -100, 100) * (s / 2 - 6) / 100; }

// Premium stick box with smooth trail and glow
void stickBox(int x, int y, int s, int vx, int vy, bool showDz, const int* tx, const int* ty, int tn) {
    int cx = x + s / 2, cy = y + s / 2;
    gRect(x, y, s, s, cBg);
    gBox(x, y, s, s, cLine);
    gLine(x, cy, x + s, cy, cLine);
    gLine(cx, y, cx, y + s, cLine);
    gRing(cx, cy, s / 2 - 2, cLine);
    // Premium inner ring with subtle glow
    gRing(cx, cy, s / 4, mix8(cLineRgb, cAccentRgb, 60));
    int dz = driveDeadbandPct * (s / 2) / 100;
    if (showDz && dz > 0) gBox(cx - dz, cy - dz, dz * 2, dz * 2, cMuted);
    // Smooth fading trail
    for (int i = 0; i < tn; ++i) {
        float trailAlpha = (i + 1) / (float)tn;
        int sz = 1 + (int)(3 * trailAlpha);
        gDot(stickPx(cx, s, tx[i]), stickPx(cy, s, -ty[i]), sz,
             mix8(cLineRgb, cAccentRgb, (int)(trailAlpha * 256)));
    }
    int px = stickPx(cx, s, vx), py = stickPx(cy, s, -vy);
    bool idle = abs(vx) <= driveDeadbandPct && abs(vy) <= driveDeadbandPct;
    // Premium stick line with glow
    color stickColor = idle ? cLine : cAccent;
    gLine(cx, cy, px, py, stickColor);
    gLine(cx + 1, cy, px + 1, py, mix8(stickColor, cText, 100));
    // Stick handle with pulse
    int handleSize = idle ? 5 : 5 + ((uiFrame / 6) % 2);
    gDot(px, py, handleSize, idle ? cMuted : cAccent);
        gDot(px, py, handleSize - 2, mix8(cAccentRgb, cTextRgb, 200));
    }

    /* -------------------------------- telemetry ------------------------------- */

    static inline void readTel(Tel& t) {
    t.rawFwd = Controller1.Axis3.position(pct);
    t.rawTurn = Controller1.Axis1.position(pct);
    t.rawLx = Controller1.Axis4.position(pct);
    t.rawRy = Controller1.Axis2.position(pct);
    
    // Compute deadbanded values ONCE, reuse everywhere
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
    
    // Cache motor pointers locally to avoid repeated array indexing
    for (int i = 0; i < 4; ++i) {
        motor* m = motorDevices[i];
        if (!m->installed()) {
            t.mTemp[i] = t.mRpm[i] = t.mDa[i] = 0;
            t.mInst[i] = 0;
            t.mOut[i] = i < 2 ? t.left : t.right;
            continue;
        }
        t.mTemp[i] = static_cast<int>(m->temperature(celsius));
        t.mRpm[i] = static_cast<int>(m->velocity(rpm));
        t.mDa[i] = static_cast<int>(m->current(amp) * 10.0);
        t.mInst[i] = 1;
        t.mOut[i] = i < 2 ? t.left : t.right;
        ++t.online;
        t.totalDa += abs(t.mDa[i]);
        if (t.mTemp[i] > t.mTemp[t.hot]) t.hot = i;
    }
    t.warn = (t.online < 4) + (t.mTemp[t.hot] >= 60) + (t.bat <= 20) + (t.btemp >= 50);
    
    // Direct assignment avoids temporary array
    t.btn[0] = Controller1.ButtonL1.pressing();
    t.btn[1] = Controller1.ButtonL2.pressing();
    t.btn[2] = Controller1.ButtonR1.pressing();
    t.btn[3] = Controller1.ButtonR2.pressing();
    t.btn[4] = Controller1.ButtonA.pressing();
    t.btn[5] = Controller1.ButtonB.pressing();
    t.btn[6] = Controller1.ButtonX.pressing();
    t.btn[7] = Controller1.ButtonY.pressing();
    t.btn[8] = Controller1.ButtonUp.pressing();
    t.btn[9] = Controller1.ButtonDown.pressing();
    t.btn[10] = Controller1.ButtonLeft.pressing();
    t.btn[11] = Controller1.ButtonRight.pressing();
}

/* animation / history state */
int histL[kHist], histR[kHist], histA[kHist];
int histHead = 0;
int trailX[2][kTrail], trailY[2][kTrail];
float wheelPhase[4] = {0, 0, 0, 0};
int transFrames = 0;

static inline void pushHistory(const Tel& t) {
    histL[histHead] = (t.mRpm[0] + t.mRpm[1]) / 2;
    histR[histHead] = (t.mRpm[2] + t.mRpm[3]) / 2;
    histA[histHead] = t.totalDa;
    histHead = (histHead + 1) % kHist;
}

static inline void pushTrail(int s, int x, int y) {
    for (int i = 0; i < kTrail - 1; ++i) {
        trailX[s][i] = trailX[s][i + 1];
        trailY[s][i] = trailY[s][i + 1];
    }
    trailX[s][kTrail - 1] = x;
    trailY[s][kTrail - 1] = y;
}

/* ---------------------------------- chrome -------------------------------- */

const char* modeName(int mode) { return mode == 2 ? "DRIVER" : mode == 1 ? "AUTO" : "DISABLED"; }

static inline void drawChrome(const Tel& t) {
    // Premium title with layered shadow for depth
    Brain.Screen.setFont(prop20);
    Brain.Screen.setPenColor(color(0, 0, 0));
    Brain.Screen.printAt(10, 22, true, "VEXTOP");
    Brain.Screen.setPenColor(cAccent2);
    Brain.Screen.printAt(9, 21, true, "VEXTOP");
    Brain.Screen.setPenColor(cText);
    Brain.Screen.printAt(8, 20, true, "VEXTOP");
    Brain.Screen.setFont(mono12);
    
    // Team branding in header with premium styling
    Brain.Screen.setFont(mono12);
    gTextC(240, 19, 7, cMuted, "TEAM 5977C  //  THE WARRIORS");
    Brain.Screen.setFont(mono12);
    
    // Premium status indicator with smooth pulse
    float pulsePhase = (uiFrame % 120) / 120.0f * 6.28318f;
    int dotSize = 2 + (int)(sinf(pulsePhase) * 1.5f + 1.5f);
    if (t.warn > 0) {
        float warnPulse = (uiFrame % 30) / 30.0f * 6.28318f;
        dotSize = 3 + (int)(sinf(warnPulse) * 2.0f + 2.0f);
    }
    gDot(88, 13, dotSize, t.warn > 0 ? cDanger : cGood);
    // Glow ring around status dot
    gRing(88, 13, dotSize + 2, mix8(t.warn > 0 ? cDangerRgb : cGoodRgb, cBgRgb, 80));
    gText(100, 19, cMuted, "// %s", tabNames[selectedTab]);

    // Premium mode chip with smooth transitions
    color mc = t.mode == 2 ? cAccent : t.mode == 1 ? cWarn : cMuted;
    if (t.mode != 0) {
        gRect(268, 4, 96, 18, mc);
        // Inner highlight
        gLine(269, 5, 363, 5, mix8(mc, cText, 100));
    } else {
        gBox(268, 4, 96, 18, mc);
    }
    gTextC(316, 18, 8, t.mode != 0 ? cBg : mc, "%s", modeName(t.mode));

    // Premium battery with animated fill
    color bc = t.bat <= 20 ? cDanger : t.bat <= 50 ? cWarn : cGood;
    char pctBuf[8];
    FMT_PCT(pctBuf, t.bat);
    gText(374, 19, cText, "%s", pctBuf);
    // Battery icon with 3D effect
    gBox(414, 7, 30, 12, cMuted);
    gRect(444, 10, 3, 6, cMuted);
    // Animated battery fill with gradient
    int fillWidth = t.bat * 26 / 100;
    for (int i = 0; i < fillWidth; ++i) {
        color fillColor = mix8(bc, cText, i * 256 / 26);
        gRect(416 + i, 9, 1, 8, fillColor);
    }
    // Battery shine
    if (fillWidth > 4) gLine(417, 10, 417 + fillWidth / 3, 10, mix8(cTextRgb, cBgRgb, 100));
    if (t.warn > 0) {
        // Warning pulse
        if ((uiFrame / 15) % 2 == 0) gText(458, 19, cDanger, "!");
    }

    // Premium header rule with flowing gradient
    gLine(0, 26, 480, 26, cLine);
    int sx = (uiFrame * 2) % 540 - 30;
    for (int k = 0; k < 30; ++k) {
        int x = sx + k * 2;
        if (x < 0 || x >= 480) continue;
        float wave = sinf((uiFrame + k * 5) * 0.05f) * 0.5f + 0.5f;
        color accentColor = mix8(cAccentRgb, cAccent2Rgb, (int)(wave * 256));
        gRect(x, 25, 2, 2, accentColor);
    }
    // Subtle bottom glow line
    gLine(0, 27, 480, 27, mix8(cLineRgb, cAccentRgb, 40));

    // Premium sidebar with depth
    gRect(0, 28, kSideW, 212, cPanel);
    // Sidebar gradient edge
    for (int y = 28; y < 240; ++y) {
        float alpha = (y - 28) / 212.0f * 30;
        gLine(kSideW, y, kSideW, y, mix8(cLineRgb, cAccentRgb, (int)alpha));
    }
    gLine(kSideW, 27, kSideW, 240, cLine);
    
    for (int i = 0; i < 7; ++i) {
        int y = kTabTop + i * kTabH;
        bool sel = i == selectedTab;
        if (sel) {
            // Selected tab with premium glow
            gRect(0, y, kSideW, kTabH - 2, cBg);
            gRect(0, y, 3, kTabH - 2, cAccent);
            // Glow gradient
            for (int yy = 0; yy < kTabH - 2; ++yy) {
                float alpha = 1.0f - yy / (float)(kTabH - 2);
                gLine(3, y + yy, 5, y + yy, mix8(cAccentRgb, cBgRgb, (int)(alpha * 100)));
            }
            gLine(kSideW - 1, y, kSideW - 1, y + kTabH - 3, cAccent);
            // Selected tab particles (less frequent for performance)
            if (uiFrame % 40 == 0) {
                spawnParticles(kSideW - 5, y + kTabH / 2, 1, cAccent, 0.2f, 0.8f, 1, 2, 600);
            }
        }
        Brain.Screen.setFont(mono12);
        gText(10, y + 14, sel ? cAccent : cLine, "0%d", i + 1);
        Brain.Screen.setFont(mono15);
        gTextC(kSideW / 2 + 2, y + 33, 9, sel ? cAccent : cMuted, "%s", tabNames[i]);
    }
    // Team branding at bottom of sidebar with premium styling
    Brain.Screen.setFont(mono12);
    gTextC(kSideW / 2, 230, 7, cGold, "THE WARRIORS");
    // Subtle underline
    gLine(kSideW / 2 - 35, 232, kSideW / 2 + 35, 232, cGold);
    Brain.Screen.setFont(mono12);
}

/* ---------------------------------- pages --------------------------------- */

static inline void drawWheel(int x, int y, int out, float phase) {
    const int w = 12, h = 36;
    int mag = clampInt(abs(out), 0, 100);
    // Premium color based on direction and magnitude
    color c = out < 0 ? mix8(cLineRgb, cAccent2Rgb, mag * 256 / 100) : mix8(cLineRgb, cAccentRgb, mag * 256 / 100);
    gRect(x, y, w, h, c);
    // Premium tread animation
    int off = static_cast<int>(phase * 6) % 36;
    for (int k = 0; k < 6; ++k) {
        int yy = y + ((k * 6 - off + 36) % 36);
        gLine(x + 1, yy, x + w - 1, yy, cBg);
    }
    gBox(x, y, w, h, cMuted);
    // Side highlight for 3D effect
    gLine(x, y, x, y + h, mix8(c, cText, 80));
}

static inline void pageDash(const Tel& t) {
    // drivetrain card with live robot
    gCard(76, 32, 190, 200, "DRIVETRAIN");
    const int cx = 171, cy = 106;
    drawWheel(129, 66, t.sLeft, wheelPhase[0]);
    drawWheel(129, 110, t.sLeft, wheelPhase[1]);
    drawWheel(201, 66, t.sRight, wheelPhase[2]);
    drawWheel(201, 110, t.sRight, wheelPhase[3]);
    
    // Premium robot body with depth
    gRect(143, 64, 56, 84, cBg);
    gBox(143, 64, 56, 84, cAccent);
    // 3D highlight
    gLine(143, 64, 199, 64, mix8(cAccentRgb, cTextRgb, 120));
    gLine(143, 64, 143, 148, mix8(cAccentRgb, cTextRgb, 120));
    gRect(161, 67, 20, 3, cAccent);
    gLine(cx - 6, cy, cx + 6, cy, cLine);
    gLine(cx, cy - 6, cx, cy + 6, cLine);
    
    // Motor temps with premium styling
    for (int i = 0; i < 4; ++i) {
        int ty = i % 2 == 0 ? 88 : 132;
        int tx = i < 2 ? 88 : 222;
        char tb[8];
        FMT_TEMP(tb, t.mTemp[i]);
        color tempColor = mix8(cMutedRgb, heatRgb(t.mTemp[i]), 256);
        gText(tx, ty, tempColor, "%s", tb);
        // Temp indicator dot
        gDot(tx + 30, ty - 2, 3, tempColor);
    }
    
    // Premium velocity vector with smooth animation
    float fv = (t.sLeft + t.sRight) / 2.0f;
    float tv = (t.sLeft - t.sRight) / 2.0f;
    float mag = sqrtf(fv * fv + tv * tv);
    if (mag > 6.0f) {
        float dx = tv * 0.44f, dy = -fv * 0.44f;
        int ex = cx + rnd(dx), ey = cy + rnd(dy);
        color ac = fv < 0 ? cAccent2 : cText;
        // Main vector line with glow
        gLine(cx, cy, ex, ey, ac);
        gLine(cx + 1, cy, ex + 1, ey, ac);
        gLine(cx, cy + 1, ex, ey + 1, mix8(ac, cBg, 100));
        double ang = atan2(static_cast<double>(dy), static_cast<double>(dx));
        // Arrowhead with premium styling
        int ax1 = ex + static_cast<int>(9 * cos(ang + 2.6));
        int ay1 = ey + static_cast<int>(9 * sin(ang + 2.6));
        int ax2 = ex + static_cast<int>(9 * cos(ang - 2.6));
        int ay2 = ey + static_cast<int>(9 * sin(ang - 2.6));
        gLine(ex, ey, ax1, ay1, ac);
        gLine(ex, ey, ax2, ay2, ac);
        gLine(ax1, ay1, ax2, ay2, ac);
        // Speed particles for high throttle
        if (mag > 60 && (uiFrame % 3 == 0)) {
            spawnParticles(cx, cy, 2, ac, 1.0f, 3.0f, 1, 2, 300);
        }
    }
    
    // Premium value displays
    char pctBuf[8];
    gText(86, 188, cMuted, "L");
    FMT_PCT(pctBuf, t.sLeft);
    gText(100, 188, cText, "%s", pctBuf);
    gText(176, 188, cMuted, "R");
        FMT_PCT(pctBuf, t.sRight);
    gText(190, 188, cText, "%s", pctBuf);
    gText(86, 208, cMuted, "FWD");
    gText(118, 208, cText, "%4d", t.fw);
    gText(176, 208, cMuted, "TRN");
    gText(208, 208, cText, "%4d", t.tr);

    // Premium scanline aura on drivetrain card when driving
    if (mag > 10) {
        for (int y = 64; y < 148; y += 3) {
            float wave = sinf((uiFrame + y * 0.5f) * 0.1f) * 0.5f + 0.5f;
            int alpha = (int)(wave * 25 + 5);
            gRect(143, y, 56, 1, mix8(cBgRgb, cAccentRgb, alpha));
        }
    }

    // Premium throttle gauge
    gCard(272, 32, 202, 130, "THROTTLE");
    arcGauge(373, 104, 50, 10, t.sThr, cAccentRgb, t.sThr > 80 ? cDangerRgb : cAccent2Rgb, t.peak);
    gRing(373, 104, 34, cLine);
    // Inner glow ring
    gRing(373, 104, 32, mix8(cLineRgb, cAccentRgb, 40));
    Brain.Screen.setFont(prop30);
        FMT_PCT(pctBuf, t.sThr);
    gTextC(373, 114, 17, cText, "%s", pctBuf);
    Brain.Screen.setFont(mono12);
    gTextC(373, 150, 8, cMuted, "PEAK %d%%", t.peak);
    
        // Power card with premium styling
        gCard(272, 168, 202, 64, "POWER");
        gText(282, 202, cMuted, "BAT");
        barG(318, 192, 118, 10, t.bat, t.bat <= 20 ? cDangerRgb : cAccentRgb, t.bat <= 20 ? cDangerRgb : cGoodRgb);
    FMT_PCT(pctBuf, t.bat);
        gText(442, 202, cText, "%s", pctBuf);
        gText(282, 222, cMuted, "AMP");
        barG(318, 212, 118, 10, t.totalDa * 100 / 80, cAccentRgb, cWarnRgb);
        char ampBuf[8];
        FMT_AMP(ampBuf, t.totalDa);
        gText(442, 222, cText, "%s", ampBuf);
    }

    static inline void pageMotors(const Tel& t) {
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
        // Fast format: "P%d %s" - port is single digit, name is const
        title[0] = 'P';
        title[1] = '0' + motorPorts[i];
        title[2] = ' ';
        const char* name = motorNames[i];
        int n = 0;
                while (name[n]) { title[3 + n] = name[n]; n++; }
        title[3 + n] = 0;
        gCard(x, y, w, h, title);
        // Premium status indicator
        gDot(x + w - 12, y + 12, 4, t.mInst[i] ? cGood : cDanger);
        gRing(x + w - 12, y + 12, 6, mix8(t.mInst[i] ? cGoodRgb : cDangerRgb, cBgRgb, 60));

        int rv = (int)t.sRpm[i];
        bool rev = rv < 0;
        // Premium arc gauge with dynamic color
                const Rgb& gaugeLo = rev ? cAccent2Rgb : cAccentRgb;
                const Rgb& gaugeHi = rev ? cWarnRgb : cGoodRgb;
        arcGauge(x + 38, y + 58, 28, 7, abs(rv) * 100 / 200, gaugeLo, gaugeHi, -1);
        // Center RPM display with premium font
        char rpmBuf[8];
        FMT_RPM(rpmBuf, rv);
        gTextC(x + 38, y + 62, 8, cText, "%s", rpmBuf);
        gTextC(x + 38, y + 86, 8, cMuted, "RPM");
        // Direction indicator
        gTextC(x + 38, y + 42, 7, rev ? cAccent2 : cAccent, rev ? "REV" : "FWD");

        int x0 = x + 78, bw = w - 86, rx = x + w - 8;
        gText(x0, y + 34, cMuted, "TEMP");
        char tb[8];
        FMT_TEMP(tb, t.mTemp[i]);
        color tempColor = mix8(cMutedRgb, heatRgb(t.mTemp[i]), 256);
        gText(rx - static_cast<int>(strlen(tb)) * 8, y + 34, tempColor, "%s", tb);
        barG(x0, y + 38, bw, 6, t.mTemp[i] * 100 / 70, cGoodRgb, heatRgb(t.mTemp[i]));
        // Temp warning indicator
        if (t.mTemp[i] >= 55) {
            gDot(rx - 10, y + 34, 3, cDanger);
        }

        // Hot motor particle effect (reduced frequency)
        if (t.mTemp[i] >= 55 && (uiFrame % 12 == 0)) {
            spawnParticles(x + 38, y + 58, 1, cDanger, 0.5f, 1.5f, 1, 2, 500);
        }

        int ad = abs(t.mDa[i]);
        gText(x0, y + 56, cMuted, "AMPS");
        char ab[8];
        FMT_AMP(ab, ad);
        color ampColor = ad > 20 ? cDanger : ad > 10 ? cWarn : cText;
        gText(rx - static_cast<int>(strlen(ab)) * 8, y + 56, ampColor, "%s", ab);
        barG(x0, y + 60, bw, 6, ad * 100 / 25, cAccentRgb, ad > 20 ? cDangerRgb : cWarnRgb);

        // High current particle effect
        if (ad > 20 && (uiFrame % 8 == 0)) {
            spawnParticles(x + 38, y + 58, 1, cWarn, 0.5f, 1.5f, 1, 2, 400);
        }

        gText(x0, y + 78, cMuted, "OUT");
        char ob[8];
        FMT_PCT(ob, t.mOut[i]);
        gText(rx - static_cast<int>(strlen(ob)) * 8, y + 78, cText, "%s", ob);
        sbarG(x0, y + 82, bw, 6, t.mOut[i], cAccentRgb, cGoodRgb, cAccent2Rgb, cWarnRgb);
        
        // Premium aura glow on active motors - smoother
        if (abs(rv) > 100 || ad > 5) {
            for (int yy = y + 2; yy < y + h - 2; yy += 4) {
                float wave = sinf((uiFrame + yy * 0.3f) * 0.08f) * 0.5f + 0.5f;
                int alpha = (int)(wave * 20 + 5);
                gRect(x + 2, yy, w - 4, 1, mix8(cPanelRgb, rev ? cAccent2Rgb : cAccentRgb, alpha));
            }
        }
        
        // Motor efficiency indicator
        if (t.mInst[i] && abs(rv) > 10) {
            float efficiency = 100.0f - (ad * 100.0f / (abs(rv) / 200.0f * 25.0f + 1.0f));
            efficiency = clampInt(efficiency, 0, 100);
            color effColor = efficiency > 70 ? cGood : efficiency > 40 ? cWarn : cDanger;
            gText(x + 5, y + h - 18, cMuted, "EFF");
            char effBuf[8];
            FMT_PCT(effBuf, (int)efficiency);
            gText(x + 35, y + h - 18, effColor, "%s", effBuf);
        }
    }
}

static inline void pageGraph(const Tel& t) {
    gCard(76, 32, 398, 202, "TELEMETRY");
    char rpmBuf[8];
    int lAvg = (t.mRpm[0] + t.mRpm[1]) / 2;
    int rAvg = (t.mRpm[2] + t.mRpm[3]) / 2;
    FMT_RPM(rpmBuf, lAvg);
    gText(168, 48, cAccent, "L %s", rpmBuf);
    FMT_RPM(rpmBuf, rAvg);
    gText(250, 48, cAccent2, "R %s", rpmBuf);
    char ampBuf[8];
    FMT_AMP(ampBuf, t.totalDa);
    gText(332, 48, cWarn, "A %s", ampBuf);

    const int gx = 84, gy = 58, gw = 382, gh = 164;
    gRect(gx, gy, gw, gh, cBg);
    gBox(gx, gy, gw, gh, cLine);
    // Premium grid with subtle lines
    for (int x = gx + 2; x < gx + gw; x += 16) {
        // Vertical grid lines
        for (int y = gy + 2; y < gy + gh - 2; y += 2) {
            gRect(x, y, 1, 1, mix8(cLineRgb, cBgRgb, 100));
        }
    }
    // Horizontal grid lines
    for (int y = gy + 2; y < gy + gh - 2; y += 20) {
        for (int x = gx + 2; x < gx + gw - 2; x += 2) {
            gRect(x, y, 1, 1, mix8(cLineRgb, cBgRgb, 80));
        }
    }
    // Center line prominent
    for (int x = gx + 2; x < gx + gw; x += 4) {
        gRect(x, gy + gh / 2, 2, 1, cMuted);
    }
    gText(gx + 4, gy + 12, cMuted, "+200");
    gText(gx + 4, gy + gh / 2 - 3, cMuted, "0");
    gText(gx + 4, gy + gh - 4, cMuted, "-200");

    // Premium scanline aura
    for (int y = gy + 2; y < gy + gh - 2; y += 3) {
        float wave = sinf((uiFrame + y * 0.4f) * 0.06f) * 0.5f + 0.5f;
        int alpha = (int)(wave * 15 + 3);
        gRect(gx + 2, y, gw - 4, 1, mix8(cBgRgb, cAccentRgb, alpha));
    }

    int mid = gy + gh / 2;
    int prevL = mid, prevR = mid, prevA = gy + gh;
    int lastL = mid, lastR = mid;
    // Premium smooth polyline with anti-aliasing effect
    for (int i = 0; i < kHist; ++i) {
        int idx = (histHead + i) % kHist;
        int l = clampInt(histL[idx], -200, 200);
        int r = clampInt(histR[idx], -200, 200);
        int a = clampInt(histA[idx], 0, 80);
        int yl = mid - l * (gh / 2 - 4) / 200;
        int yr = mid - r * (gh / 2 - 4) / 200;
        int ya = gy + gh - 2 - a * (gh - 6) / 80;
        int x = gx + 2 + i * 4;
        if (i > 0) {
            // Current draw with glow
            gLine(x - 4, prevA, x, ya, cWarn);
            gLine(x - 3, prevA, x - 1, ya, mix8(cWarnRgb, cTextRgb, 60));
            // Left motor RPM
            gLine(x - 4, prevL, x, yl, cAccent);
            gLine(x - 3, prevL, x - 1, yl, mix8(cAccentRgb, cTextRgb, 80));
            // Right motor RPM
            gLine(x - 4, prevR, x, yr, cAccent2);
            gLine(x - 3, prevR, x - 1, yr, mix8(cAccent2Rgb, cTextRgb, 80));
        }
        prevL = yl;
        prevR = yr;
        prevA = ya;
        lastL = yl;
        lastR = yr;
    }
    int ex = gx + 2 + (kHist - 1) * 4;
    // Premium pulsing current position indicators
    float pulse = sinf(uiFrame * 0.1f) * 0.5f + 0.5f;
    int pr = 3 + (int)(pulse * 2);
    gDot(ex, lastL, pr, cAccent);
    gDot(ex, lastL, pr - 1, mix8(cAccentRgb, cTextRgb, 180));
    gDot(ex, lastR, pr, cAccent2);
    gDot(ex, lastR, pr - 1, mix8(cAccent2Rgb, cTextRgb, 180));
    
    // Live value callouts
    char valBuf[16];
    FMT_RPM(valBuf, (int)t.sRpm[0]);
    gText(gx + 4, gy + 28, cAccent, "L: %s", valBuf);
    FMT_RPM(valBuf, (int)t.sRpm[2]);
    gText(gx + 4, gy + 40, cAccent2, "R: %s", valBuf);
    FMT_AMP(valBuf, t.totalDa);
    gText(gx + 4, gy + 52, cWarn, "I: %s", valBuf);
}

static inline void pageInput(const Tel& t) {
    gCard(76, 32, 112, 200, "L STICK");
    stickBox(82, 56, 100, t.rawLx, t.rawFwd, false, trailX[0], trailY[0], kTrail);
    gText(84, 178, cMuted, "X");
    char valBuf[8];
    FMT_PCT(valBuf, t.rawLx);
    gText(110, 178, cText, "%s", valBuf);
    gText(84, 196, cMuted, "Y");
    FMT_PCT(valBuf, t.rawFwd);
    gText(110, 196, cText, "%s", valBuf);

    // Premium aura on active stick
    if (abs(t.rawLx) > 10 || abs(t.rawFwd) > 10) {
        for (int y = 56; y < 156; y += 3) {
            float wave = sinf((uiFrame + y * 0.5f) * 0.08f) * 0.5f + 0.5f;
            int alpha = (int)(wave * 18 + 5);
            gRect(82, y, 100, 1, mix8(cBgRgb, cAccentRgb, alpha));
        }
    }

    gCard(194, 32, 112, 200, "R STICK");
    stickBox(200, 56, 100, t.rawTurn, t.rawRy, false, trailX[1], trailY[1], kTrail);
    gText(202, 178, cMuted, "X");
    FMT_PCT(valBuf, t.rawTurn);
    gText(228, 178, cText, "%s", valBuf);
    gText(202, 196, cMuted, "Y");
    FMT_PCT(valBuf, t.rawRy);
    gText(228, 196, cText, "%s", valBuf);

    // Premium aura on active stick
    if (abs(t.rawTurn) > 10 || abs(t.rawRy) > 10) {
        for (int y = 56; y < 156; y += 3) {
            float wave = sinf((uiFrame + y * 0.5f) * 0.08f) * 0.5f + 0.5f;
            int alpha = (int)(wave * 18 + 5);
            gRect(200, y, 100, 1, mix8(cBgRgb, cAccent2Rgb, alpha));
        }
    }

    gCard(312, 32, 162, 122, "BUTTONS");
    for (int i = 0; i < 12; ++i) {
        int bx = 318 + (i % 4) * 38;
        int by = 46 + (i / 4) * 34;
        bool on = t.btn[i] != 0;
        // Premium button styling
        color btnBg = on ? cAccent : cBg;
        color btnBorder = on ? cText : cLine;
        color btnText = on ? cBg : cText;
        gRect(bx, by, 34, 28, btnBg);
        gBox(bx, by, 34, 28, btnBorder);
        // Button highlight
        if (on) gLine(bx + 1, by + 1, bx + 33, by + 1, mix8(cAccentRgb, cTextRgb, 120));
        gTextC(bx + 17, by + 18, 8, btnText, "%s", buttonNames[i]);
        
        // Pressed button glow particles
        if (on && (uiFrame % 4 == 0)) {
            spawnParticles(bx + 17, by + 14, 1, cAccent, 0.5f, 1.5f, 1, 2, 200);
        }
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

static inline void pageSystem(const Tel& t) {
    gCard(76, 32, 190, 200, "POWER");
    bool low = t.bat <= 20;
    arcGauge(171, 104, 46, 9, t.bat, low ? cDangerRgb : cAccentRgb, low ? cWarnRgb : cGoodRgb, -1);
    gRing(171, 104, 31, cLine);
    // Inner glow ring
    gRing(171, 104, 29, mix8(cLineRgb, low ? cDangerRgb : cGoodRgb, 40));
    Brain.Screen.setFont(prop30);
    char pctBuf[8];
    FMT_PCT(pctBuf, t.bat);
    gTextC(171, 114, 17, cText, "%s", pctBuf);
    Brain.Screen.setFont(mono12);
    gTextC(171, 146, 8, cMuted, "BATTERY");
    
    // Low battery particles
    if (low && (uiFrame % 10 == 0)) {
        spawnParticles(171, 104, 1, cDanger, 0.5f, 1.5f, 1, 2, 600);
    }
    
    // Premium battery details
    gText(84, 172, cMuted, "VOLTAGE");
    gText(176, 172, cText, "%d mV", t.mv);
    gText(84, 190, cMuted, "BAT AMPS");
    char ampBuf[8];
    FMT_AMP(ampBuf, t.bda);
    gText(176, 190, cText, "%s", ampBuf);
    gText(84, 208, cMuted, "BAT TEMP");
    char tempBuf[8];
    FMT_TEMP(tempBuf, t.btemp);
    gText(176, 208, t.btemp >= 50 ? cDanger : t.btemp >= 40 ? cWarn : cText, "%s", tempBuf);
    gText(84, 226, cMuted, "UPTIME");
    gText(176, 226, cText, "%02d:%02d", t.runtime / 60, t.runtime % 60);

    gCard(272, 32, 202, 200, "STATUS");
    char buf[16];
    statusRow(284, 66, "CONTROLLER", t.ctrl ? 1 : 0, t.ctrl ? "OK" : "LOST");
    // Fast format: "%d/4"
    buf[0] = '0' + t.online;
    buf[1] = '/'; buf[2] = '4'; buf[3] = 0;
    statusRow(284, 88, "MOTORS", t.online == 4 ? 1 : 0, buf);
    statusRow(284, 110, "FIELD", t.field ? 1 : 2, t.field ? "FIELD" : "LOCAL");
    statusRow(284, 132, "COMP SW", t.sw ? 1 : 2, t.sw ? "YES" : "NO");
    statusRow(284, 154, "SD CARD", t.sd ? 1 : 2, t.sd ? "READY" : "NONE");
    // Fast format: "P%d %dC"
    buf[0] = 'P';
    buf[1] = '0' + motorPorts[t.hot];
    buf[2] = ' ';
    int temp = t.mTemp[t.hot];
    int n = abs(temp);
    buf[3] = temp < 0 ? '-' : '0' + n / 10;
    buf[4] = '0' + n % 10;
    buf[5] = 'C'; buf[6] = 0;
    statusRow(284, 176, "HOT MOTOR", t.mTemp[t.hot] >= 60 ? 0 : t.mTemp[t.hot] >= 45 ? 2 : 1, buf);
    bool blink = (uiFrame / 6) % 2 == 0;
    color wc = t.warn > 0 ? (blink ? cDanger : cWarn) : cGood;
    gBox(284, 192, 178, 30, wc);
    if (t.warn > 0) gText(296, 212, wc, "%d WARNING%s ACTIVE", t.warn, t.warn > 1 ? "S" : "");
    else gText(296, 212, wc, "ALL SYSTEMS NOMINAL");
    
    // Warning particles
    if (t.warn > 0 && (uiFrame % 15 == 0)) {
        spawnParticles(373, 207, 2, cWarn, 0.5f, 1.5f, 1, 2, 500);
    }
    
    // Premium aura on status card
    for (int y = 32; y < 232; y += 4) {
        float wave = sinf((uiFrame + y * 0.3f) * 0.06f) * 0.5f + 0.5f;
        int alpha = (int)(wave * 12 + 3);
        gRect(272, y, 202, 1, mix8(cPanelRgb, t.warn > 0 ? cWarnRgb : cGoodRgb, alpha));
    }
}

// ============================================================================
// DEBUG / ANIMATION TEST TAB
// ============================================================================

struct DebugState {
    int selectedTest = 0;
    int testFrame = 0;
    bool running = false;
    int lastTouch = 0;
};

static inline void pageDebug(const Tel& t) {
        static DebugState dbg;
    
        gCard(76, 32, 398, 200, "DEBUG // ANIMATION TEST");
    
        // Test selection buttons
        const char* tests[8] = {
            "BOOT ANIM", "OUTRO PHASE 0", "OUTRO PHASE 1", "OUTRO PHASE 2",
            "PARTICLE STRESS", "SHAKE TEST", "THEME PREVIEW", "SCANLINES"
        };
    
        for (int i = 0; i < 8; ++i) {
            int bx = 84 + (i % 4) * 96;
            int by = 50 + (i / 4) * 60;
            bool sel = i == dbg.selectedTest;
            bool hover = sel || (Brain.Screen.pressing() && 
                Brain.Screen.xPosition() >= bx && Brain.Screen.xPosition() <= bx + 88 &&
                Brain.Screen.yPosition() >= by && Brain.Screen.yPosition() <= by + 48);
        
            color bg = sel ? cAccent : hover ? cLine : cBg;
            color border = sel ? cText : cLine;
            color text = sel ? cBg : cText;
        
            gRect(bx, by, 88, 48, bg);
            gBox(bx, by, 88, 48, border);
            Brain.Screen.setFont(mono12);
            gTextC(bx + 44, by + 28, 7, text, "%s", tests[i]);
        }
    
        // Run button
        int runX = 84, runY = 172;
        bool runHover = Brain.Screen.pressing() && 
            Brain.Screen.xPosition() >= runX && Brain.Screen.xPosition() <= runX + 180 &&
            Brain.Screen.yPosition() >= runY && Brain.Screen.yPosition() <= runY + 40;
        color runBg = dbg.running ? cDanger : runHover ? cAccent : cGood;
        color runText = dbg.running ? cBg : cBg;
        gRect(runX, runY, 180, 40, runBg);
        gBox(runX, runY, 180, 40, cText);
        Brain.Screen.setFont(prop20);
        gTextC(runX + 90, runY + 28, 11, runText, dbg.running ? "STOP" : "RUN TEST");
        Brain.Screen.setFont(mono12);
    
        // Info panel
        gCard(272, 172, 202, 60, "INFO");
        gText(282, 196, cMuted, "FRAME: %d", dbg.testFrame);
        gText(282, 214, cMuted, "PARTICLES: %d/%d", particleCount, kMaxParticles);
        gText(382, 196, cMuted, "UI FRAME: %d", uiFrame);
        gText(382, 214, cMuted, "SHAKE: %d", shakeTime > 0 ? 1 : 0);
    
        // Handle touch
        if (Brain.Screen.pressing() && Brain.Timer.time(msec) - dbg.lastTouch > 200) {
            int tx = Brain.Screen.xPosition();
            int ty = Brain.Screen.yPosition();
        
            // Test selection
            for (int i = 0; i < 8; ++i) {
                int bx = 84 + (i % 4) * 96;
                int by = 50 + (i / 4) * 60;
                if (tx >= bx && tx <= bx + 88 && ty >= by && ty <= by + 48) {
                    dbg.selectedTest = i;
                    dbg.testFrame = 0;
                    dbg.running = false;
                    dbg.lastTouch = Brain.Timer.time(msec);
                    break;
                }
            }
        
            // Run/Stop button
            if (tx >= runX && tx <= runX + 180 && ty >= runY && ty <= runY + 40) {
                dbg.running = !dbg.running;
                dbg.testFrame = 0;
                dbg.lastTouch = Brain.Timer.time(msec);
            
                // Reset particles on start
                if (dbg.running) {
                    for (int j = 0; j < kMaxParticles; ++j) particles[j].active = false;
                    particleCount = 0;
                    shakeX = shakeY = shakeTime = 0;
                }
            }
        }
    
        // Run selected test
        if (dbg.running) {
            dbg.testFrame++;
        
            switch (dbg.selectedTest) {
                case 0: // Boot animation preview (condensed)
                    if (dbg.testFrame == 1) {
                        for (int j = 0; j < kMaxParticles; ++j) particles[j].active = false;
                        particleCount = 0;
                    }
                    if (dbg.testFrame % 3 == 0 && particleCount < 30) {
                        spawnParticles(rand() % 480, -10, 2, cAccent, 0.5f, 2.0f, 1, 3, 2000);
                    }
                    if (dbg.testFrame > 120) dbg.running = false;
                    break;
                
                case 1: // Outro Phase 0 - Freeze frame with shockwave
                    if (dbg.testFrame == 1) {
                        for (int j = 0; j < kMaxParticles; ++j) particles[j].active = false;
                        particleCount = 0;
                        shakeX = shakeY = shakeTime = 0;
                    }
                    if (dbg.testFrame <= 30) {
                        float progress = dbg.testFrame / 30.0f;
                        float ease = 1.0f - powf(1.0f - progress, 3);
                        for (int r = 0; r < 5; ++r) {
                            float ringProg = fmodf(ease * 3.0f + r * 0.2f, 1.0f);
                            int radius = (int)(ringProg * 300);
                            color rc = mix8(cAccentRgb, cBgRgb, (int)(ringProg * 256));
                            gRing(240 + shakeX, 120 + shakeY, radius, rc);
                        }
                        if (dbg.testFrame < 8) {
                            int flashSize = dbg.testFrame * 60;
                            gRect(240 - flashSize/2 + shakeX, 120 - flashSize/2 + shakeY, flashSize, flashSize, 
                                  dbg.testFrame % 2 == 0 ? cAccent : cAccent2);
                        }
                    } else {
                        dbg.running = false;
                    }
                    break;
                
                case 2: // Outro Phase 1 - Stats reveal
                    if (dbg.testFrame == 1) {
                        for (int j = 0; j < kMaxParticles; ++j) particles[j].active = false;
                        particleCount = 0;
                        shakeX = shakeY = shakeTime = 0;
                    }
                    if (dbg.testFrame <= 180) {
                        for (int i = 0; i < 6; ++i) {
                            float wave = sinf((dbg.testFrame + i * 30) * 0.05f) * 0.5f + 0.5f;
                            color bgc = mix8(cPanelRgb, cAccentRgb, (int)(wave * 40));
                            gRect(0 + shakeX, 28 + i * 35 + shakeY, 480, 30, bgc);
                        }
                        // Simulated stat cards
                        for (int s = 0; s < 7; ++s) {
                            int revealFrame = 10 + s * 25;
                            if (dbg.testFrame < revealFrame) continue;
                            int prog = clampInt((dbg.testFrame - revealFrame) * 100 / 25, 0, 100);
                            float ease = 1.0f - powf(1.0f - prog / 100.0f, 3);
                            int cardY = 40 + s * 28;
                            color cardBg = mix8(cPanelRgb, cAccentRgb, (int)(ease * 30));
                            gRect(20 + shakeX, cardY + shakeY, 440, 24, cardBg);
                            int lineH = (int)(24 * ease);
                            gRect(20 + shakeX, cardY + 24 - lineH + shakeY, 4, lineH, cAccent);
                        }
                    } else {
                        dbg.running = false;
                    }
                    break;
                
                case 3: // Outro Phase 2 - Epic finale
                    if (dbg.testFrame == 1) {
                        for (int j = 0; j < kMaxParticles; ++j) particles[j].active = false;
                        particleCount = 0;
                        shakeX = shakeY = shakeTime = 0;
                        triggerShake(12, 600);
                        spawnParticles(240, 120, 60, cAccent, 3.0f, 8.0f, 3, 6, 1500);
                        spawnParticles(240, 120, 40, cAccent2, 1.0f, 4.0f, 2, 5, 2000);
                    }
                    if (dbg.testFrame <= 120) {
                        float progress = dbg.testFrame / 120.0f;
                        float ease = progress * progress * (3.0f - 2.0f * progress);
                        for (int r = 0; r < 8; ++r) {
                            float ringProg = fmodf(ease * 2.0f + r * 0.125f, 1.0f);
                            int radius = (int)(ringProg * 400);
                            color rc = mix8(cAccentRgb, cAccent2Rgb, (int)(ringProg * 256));
                            gRing(240 + shakeX, 120 + shakeY, radius, rc);
                        }
                        if (dbg.testFrame > 20) {
                            float logoEase = (dbg.testFrame - 20) / 100.0f;
                            logoEase = clampInt(logoEase * 100, 0, 100) / 100.0f;
                            logoEase = 1.0f - powf(1.0f - logoEase, 4);
                            for (int h = 0; h < 6; ++h) {
                                float angle = (h * 60.0f + dbg.testFrame * 0.5f) * 0.0174533f;
                                int hx = 240 + (int)(80 * logoEase * cosf(angle)) + shakeX;
                                int hy = 120 + (int)(80 * logoEase * sinf(angle)) + shakeY;
                                gDot(hx, hy, (int)(8 * logoEase), cAccent);
                            }
                            Brain.Screen.setFont(prop60);
                            color tc = mix8(cTextRgb, cAccentRgb, (int)(logoEase * 256));
                            gTextC(240 + shakeX, 110 + shakeY, 30, tc, "5977C");
                            Brain.Screen.setFont(prop20);
                            gTextC(240 + shakeX, 155 + shakeY, 12, cAccent2, "THE WARRIORS");
                            Brain.Screen.setFont(mono12);
                        }
                    } else {
                        dbg.running = false;
                    }
                    break;
                
                case 4: // Particle stress test
                    if (dbg.testFrame % 2 == 0 && particleCount < kMaxParticles) {
                        spawnParticles(rand() % 480, rand() % 240, 5, 
                            (rand() % 2) ? cAccent : cAccent2, 1.0f, 5.0f, 1, 4, 2000);
                    }
                    if (dbg.testFrame > 300) dbg.running = false;
                    break;
                
                case 5: // Shake test
                    if (dbg.testFrame == 1) triggerShake(15, 1000);
                    if (dbg.testFrame > 100) dbg.running = false;
                    break;
                
                case 6: // Theme preview
                    if (dbg.testFrame == 1) {
                        for (int j = 0; j < kMaxParticles; ++j) particles[j].active = false;
                        particleCount = 0;
                    }
                    {
                        color themes[3] = {cGood, cDanger, cAccent};
                        color theme = themes[(dbg.testFrame / 60) % 3];
                        Rgb themeRgb = {(int)((theme >> 16) & 0xFF), (int)((theme >> 8) & 0xFF), (int)(theme & 0xFF)};
                        gRect(76 + shakeX, 32 + shakeY, 398, 200, mix8(cPanelRgb, themeRgb, 40));
                        gBox(76 + shakeX, 32 + shakeY, 398, 200, theme);
                        Brain.Screen.setFont(prop60);
                        gTextC(275 + shakeX, 120 + shakeY, 30, theme, "THE WARRIORS");
                        Brain.Screen.setFont(mono12);
                        if (dbg.testFrame % 30 == 0) {
                            spawnParticles(275, 120, 8, theme, 1.0f, 3.0f, 2, 4, 800);
                        }
                    }
                    if (dbg.testFrame > 180) dbg.running = false;
                    break;
                
                case 7: // Scanlines effect
                    for (int y = 32; y < 232; y += 2) {
                        int alpha = (dbg.testFrame + y) % 4 == 0 ? 60 : 20;
                        gRect(76, y, 398, 1, mix8(cBgRgb, cAccentRgb, alpha));
                    }
                    if (dbg.testFrame > 200) dbg.running = false;
                    break;
                                }
        
                                // Update and draw particles for all tests
                                updateParticles(16);
                                applyShake();
                                drawParticles();
                            }
                        }

                        // ============================================================================
                        // MAIN UI RENDER LOOP - Ultra-smooth 60fps target
                        // ============================================================================

                        inline void drawUI() {
            static int lastTab = -1;
            static float sL = 0, sR = 0, sT = 0, sRpm[4] = {0, 0, 0, 0}, peak = 0;
            static int ambientParticleTimer = 0;
                                    static bool colorLUTInitialized = false;
                                    if (!colorLUTInitialized) {
                                        initColorLUT();
                                        colorLUTInitialized = true;
                                    }
                                    Tel t;
                                    readTel(t);
                                    ++uiFrame;
                                    if (uiFrame % 3 == 0) pushHistory(t);
                                    if (uiFrame % 2 == 0) {
                                        pushTrail(0, t.rawLx, t.rawFwd);
                                        pushTrail(1, t.rawTurn, t.rawRy);
                                    }

                                    // Ultra-smooth easing with delta time compensation
                                    float easingFactor = deltaTime / 16.67f; // Normalize to 60fps
                                    sL += (t.left - sL) * 0.35f * easingFactor;
                                    sR += (t.right - sR) * 0.35f * easingFactor;
                                    sT += (t.throttle - sT) * 0.30f * easingFactor;
                                    for (int i = 0; i < 4; ++i) sRpm[i] += (t.mRpm[i] - sRpm[i]) * 0.30f * easingFactor;
                                    t.sLeft = rnd(sL);
                                    t.sRight = rnd(sR);
                                    t.sThr = rnd(sT);
                                    for (int i = 0; i < 4; ++i) t.sRpm[i] = rnd(sRpm[i]);
                                    if (t.sThr > peak) peak = static_cast<float>(t.sThr);
                                    else peak -= 0.4f * easingFactor;
                                    if (peak < 0) peak = 0;
                                    t.peak = rnd(peak);
                                    for (int i = 0; i < 4; ++i) {
                                        float out = i < 2 ? sL : sR;
                                        wheelPhase[i] = fmodf(wheelPhase[i] + out / 25.0f * easingFactor + 600.0f, 6.0f);
                                    }

                                    if (selectedTab != lastTab) {
                                        transFrames = 8;
                                        lastTab = selectedTab;
                                        transitionFramesLeft = 4; // Force high render rate during transition
                                        // Spawn particles on tab switch
                                        spawnParticles(kSideW + 200, 140, 12, cAccent, 1.0f, 3.0f, 2, 4, 600);
                                        // Trigger tab switch shockwave
                                        spawnShockwave(kSideW + 200, 140, 80, cGold, 3, 400);
                                    }

                                    // Ultra-smooth adaptive render rate
                                    bool isDriving = (abs(t.left) > 5 || abs(t.right) > 5);
                                    int now = Brain.Timer.time(msec);
                                    int interval;
                                    if (transitionFramesLeft > 0) {
                                        interval = kTransitionRenderIntervalMs; // 60Hz during transitions
                                        transitionFramesLeft--;
                                    } else if (isDriving) {
                                        interval = kRenderIntervalMs; // 30Hz when driving
                                    } else {
                                        interval = kIdleRenderIntervalMs; // 15Hz when idle
                                    }
                                    if (now - lastRenderTime < interval && !isDriving && wasDriving) {
                                        // Just finished driving - render one more frame to show stopped state
                                    } else if (now - lastRenderTime < interval) {
                                        return;  // Skip this frame
                                    }
                                    lastRenderTime = now;
                                    wasDriving = isDriving;

                                    // UI watchdog: skip frame if rendering takes too long (protects drive loop)
                                    uint32_t renderStart = Brain.Timer.time(msec);
    
                                    // Update ambient particles with delta time
                                    updateParticles((int)deltaTime);
                                    ambientParticleTimer += (int)deltaTime;
                                    if (ambientParticleTimer > 2000 && particleCount < 20) {
                                        // Occasional ambient particles
                                        spawnParticles(rand() % 480, rand() % 240, 1, cMuted, 0.2f, 0.8f, 1, 2, 3000);
                                        ambientParticleTimer = 0;
                                    }
                                    // Driving particles
                                    if (isDriving && (uiFrame % 4 == 0)) {
                                        spawnParticles(171, 106, 2, cAccent, 0.5f, 1.5f, 1, 2, 400);
                                    }
            
                                    // Update advanced effects
                                    updateAdvancedEffects((int)deltaTime);
            
                                    // Trigger velocity-based effects
                                    triggerVelocityEffects(t);
            
                                    // Auto-theme based on performance
                                    triggerThemeByPerformance(t);
    
                                    Brain.Screen.setFillColor(cBg);
                                    Brain.Screen.clearScreen(cBg);
                                    Brain.Screen.setFont(mono12);
                                    drawChrome(t);
                                    switch (selectedTab) {
                                                    case 0: pageDash(t); break;
                                                    case 1: pageMotors(t); break;
                                                    case 2: pageGraph(t); break;
                                                    case 3: pageInput(t); break;
                                                    case 4: pageSystem(t); break;
                                                    case 5: pageDebug(t); break;
                                                                                        case 6: pageDoom(t); break;
                                                                                        default: pageSystem(t); break;
                                                                                    }

                            // Draw ambient particles on top
                            drawParticles();
    
                            // Draw advanced effects on top
                            drawAdvancedEffects();

                            // page transition: wipe reveal left -> right
                            if (transFrames > 0) {
                                int rx = kSideW + 1 + (8 - transFrames) * 52;
                                gRect(rx, 27, 480 - rx, 213, cBg);
                                gRect(rx, 27, 2, 213, cAccent);
                                --transFrames;
                                }
                                Brain.Screen.render();

                                // Warn if UI frame took >20ms (could starve drive at 50Hz)
                                if (Brain.Timer.time(msec) - renderStart > 20) {
                                    Brain.Screen.setPenColor(cDanger);
                                    Brain.Screen.printAt(10, 230, true, "UI LAG %dms", Brain.Timer.time(msec) - renderStart);
                                }
                            }

                            /* ---------------------------------- DOOM page --------------------------------- */

                            static inline void pageDoom(const Tel& t) {
        gCard(76, 32, 398, 200, "DOOM // VEX V5 PORT");

        // DOOM status display
        Brain.Screen.setFont(prop20);
        gTextC(275, 60, 12, cAccent, "DOOM");
        Brain.Screen.setFont(mono12);

        // Instructions
        gText(90, 90, cMuted, "CONTROLS:");
        gText(90, 110, cText, "L-Stick Y: Move Forward/Back");
        gText(90, 125, cText, "R-Stick X: Turn Left/Right");
        gText(90, 140, cText, "L-Stick X: Strafe Left/Right");
        gText(90, 155, cText, "R1: Fire");
        gText(90, 170, cText, "B: Use/Open");
        gText(90, 185, cText, "X: Enter");
        gText(90, 200, cText, "Y: Escape/Menu");
        gText(90, 215, cText, "D-Pad: Arrow Keys");
        gText(90, 230, cText, "L2/R2: Prev/Next Weapon");

        // Status
        gText(300, 90, cMuted, "STATUS:");
        gText(300, 110, cGood, "WAD: doom1.wad (SD Card)");
        gText(300, 125, cGood, "Engine: Chocolate Doom");
        gText(300, 140, cWarn, "Sound: Not Supported");
        gText(300, 155, cWarn, "Multiplayer: Not Supported");
        gText(300, 170, cMuted, "Resolution: 320x200");
        gText(300, 185, cMuted, "FPS: ~30 (V5 Brain)");

        // Launch button
        static bool doomLaunched = false;
        int btnX = 300, btnY = 200, btnW = 150, btnH = 40;
        bool btnHover = Brain.Screen.pressing() &&
            Brain.Screen.xPosition() >= btnX && Brain.Screen.xPosition() <= btnX + btnW &&
            Brain.Screen.yPosition() >= btnY && Brain.Screen.yPosition() <= btnY + btnH;

        color btnBg = doomLaunched ? cDanger : btnHover ? cAccent : cGood;
        color btnText = cBg;

        gRect(btnX, btnY, btnW, btnH, btnBg);
        gBox(btnX, btnY, btnW, btnH, cText);
        Brain.Screen.setFont(prop20);
        gTextC(btnX + btnW/2, btnY + 28, 11, btnText, doomLaunched ? "RUNNING..." : "LAUNCH DOOM");
        Brain.Screen.setFont(mono12);

        // Handle launch button press
        if (Brain.Screen.pressing() && btnHover && !doomLaunched) {
            doomLaunched = true;
            // Note: Actual DOOM launch would require PROS kernel integration
            // This is a placeholder for the UI
        }

        // Animated DOOM logo
        static int logoFrame = 0;
        logoFrame++;
        float pulse = sinf(logoFrame * 0.1f) * 0.5f + 0.5f;
        color logoColor = mix8(cAccentRgb, cAccent2Rgb, (int)(pulse * 256));
        Brain.Screen.setFont(prop60);
        gTextC(275, 60, 30, logoColor, "DOOM");
        Brain.Screen.setFont(mono12);

        // Particle effects for atmosphere
        if (uiFrame % 20 == 0) {
            spawnParticles(275 + rand() % 100 - 50, 60 + rand() % 20 - 10, 1, cAccent2, 0.5f, 1.5f, 1, 2, 800);
        }
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
                    if (idx >= 0 && idx < 7) selectedTab = idx;  // Support all 7 tabs
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
        const char* s = ok ? "ONLINE" : "MISSING";
        for (int j = 0; s[j]; ++j) detail[i][j] = s[j];
        detail[i][ok ? 6 : 7] = 0;
    }
    bool ctrlOk = Controller1.installed();
    state[4] = ctrlOk ? 1 : 0;
    const char* s4 = ctrlOk ? "LINKED" : "NOT FOUND";
    for (int j = 0; s4[j]; ++j) detail[4][j] = s4[j];
    detail[4][ctrlOk ? 6 : 9] = 0;
    int bat = clampInt(Brain.Battery.capacity(pct), 0, 100);
    state[5] = bat > 20 ? 1 : 0;
    char pctBuf[8];
    FMT_PCT(pctBuf, bat);
    for (int j = 0; pctBuf[j]; ++j) detail[5][j] = pctBuf[j];
    detail[5][strlen(pctBuf)] = 0;
    bool sdOk = Brain.SDcard.isInserted();
    state[6] = sdOk ? 1 : 2;
    const char* s6 = sdOk ? "READY" : "NONE";
    for (int j = 0; s6[j]; ++j) detail[6][j] = s6[j];
    detail[6][sdOk ? 5 : 4] = 0;
    bool fieldOk = Competition.isFieldControl();
    state[7] = 2;
    const char* s7 = fieldOk ? "FIELD" : "LOCAL";
    for (int j = 0; s7[j]; ++j) detail[7][j] = s7[j];
    detail[7][fieldOk ? 5 : 5] = 0;

    // Reset particles for boot
    for (int i = 0; i < kMaxParticles; ++i) particles[i].active = false;
    particleCount = 0;

    const int frames = 84;
    for (int f = 0; f <= frames; ++f) {
        Brain.Screen.setFillColor(cBg);
        Brain.Screen.clearScreen(cBg);

        // Update and draw particles
        updateParticles(28);
        if (f % 3 == 0 && particleCount < 30) {
            spawnParticles(rand() % 480, -10, 2, cAccent, 0.5f, 2.0f, 1, 3, 2000);
        }
        drawParticles();

        // falling hex rain - reduced density
        Brain.Screen.setFont(mono12);
        for (int col = 0; col < 12; ++col) {  // 12 cols instead of 16
            int speed = 3 + (col * 5) % 5;
            int head = (f * speed * 2 + col * 53) % 300 - 20;
            for (int k = 0; k < 4; ++k) {  // 4 chars instead of 6
                int y = head - k * 12;
                if (y < 8 || y > 236) continue;
                color c = k == 0 ? mix8(cRainRgb, cAccentRgb, 140) : mix8(cBgRgb, cRainRgb, (4 - k) * 256 / 4);
                gText(6 + col * 40, y, c, "%X", (f * 3 + col * 7 + k * 5) & 15);
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
        for (int j = 0; j < letters; ++j) logo[j] = "VEXTOP"[j];
        logo[letters] = 0;
        Brain.Screen.setFont(prop60);
        bool glitch = f > 22 && (f % 9) < 2;
        if (glitch) {
            gText(136, 106, cAccent2, "%s", logo);
            gText(146, 102, cAccent, "%s", logo);
        }
        gText(141, 104, cText, "%s", logo);
        Brain.Screen.setFont(mono12);

        // checks
                if (f > 20) gText(28, 138, cMuted, "V5 ROBOT MONITOR  //  SYSTEM CHECK  //  TEAM 5977C - THE WARRIORS");
        char progBuf[8];
        FMT_PCT(progBuf, f * 100 / frames);
        gText(400, 138, cAccent, "%s", progBuf);
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
            
            // Particle burst on check complete
            if (!pending && f == start + 3) {
                spawnParticles(x + 100, y + 8, 6, c, 1.0f, 3.0f, 2, 4, 800);
            }
        }

        barG(28, 224, 424, 10, f * 100 / frames, cAccentRgb, cAccent2Rgb);
        Brain.Screen.render();
        this_thread::sleep_for(28);
    }

    // flash + ring burst - fewer rings
    Brain.Screen.setFillColor(cAccent);
    Brain.Screen.clearScreen(cAccent);
    Brain.Screen.render();
    this_thread::sleep_for(40);
    triggerShake(10, 300);
    spawnParticles(240, 120, 40, cAccent, 2.0f, 6.0f, 2, 5, 1200);
    spawnParticles(240, 120, 20, cAccent2, 1.0f, 4.0f, 1, 3, 1500);
    
    for (int i = 0; i < 6; ++i) {  // 6 instead of 10
        Brain.Screen.setFillColor(cBg);
        Brain.Screen.clearScreen(cBg);
        
        applyShake();
        updateParticles(36);
        drawParticles();
        
        gRing(240, 120, 16 + i * 24, i % 2 ? cAccent : cAccent2);
        gRing(240, 120, 8 + i * 14, cLine);
        gRect(0, 96, 480, 48, cBg);
        Brain.Screen.setFont(prop30);
        gTextC(240, 128, 17, cAccent, "SYSTEM ONLINE");
                Brain.Screen.setFont(prop20);
                gTextC(240, 158, 12, cAccent2, "THE WARRIORS");
                Brain.Screen.setFont(mono12);
                gLine(240 - i * 24, 136, 240 + i * 24, 136, cAccent2);
                Brain.Screen.render();
                this_thread::sleep_for(36);
            }

            // diagonal wipe out - fewer steps
            for (int s = 0; s <= 8; ++s) {  // 8 instead of 12
                for (int i = 0; i < 6; ++i) gRect(0, i * 40, clampInt(s * 120 - i * 60, 0, 480), 40, cAccent);
                Brain.Screen.render();
                this_thread::sleep_for(16);
            }
            for (int s = 0; s <= 8; ++s) {
                gRect(0, 0, 480, 240, cAccent);
                for (int i = 0; i < 6; ++i) gRect(0, i * 40, clampInt(s * 120 - i * 60, 0, 480), 40, cBg);
                Brain.Screen.render();
                this_thread::sleep_for(16);
            }
    
            // Final team logo flash
            for (int i = 0; i < 3; ++i) {
                Brain.Screen.setFillColor(cBg);
                Brain.Screen.clearScreen(cBg);
                Brain.Screen.setFont(prop60);
                gTextC(240, 100, 30, cAccent, "5977C");
                Brain.Screen.setFont(prop20);
                gTextC(240, 140, 12, cAccent2, "THE WARRIORS");
                Brain.Screen.setFont(mono12);
                Brain.Screen.render();
                this_thread::sleep_for(100);
                Brain.Screen.setFillColor(cAccent);
                Brain.Screen.clearScreen(cAccent);
                Brain.Screen.render();
                this_thread::sleep_for(50);
            }
                    }

            // ============================================================================
            // PARTICLE SYSTEM - Used by boot, outro, and ambient UI
            // ============================================================================

            void spawnParticles(int cx, int cy, int count, color c, float speedMin, float speedMax, int sizeMin, int sizeMax, int lifeMs) {
                for (int i = 0; i < count && particleCount < kMaxParticles; ++i) {
                    int idx = -1;
                    for (int j = 0; j < kMaxParticles; ++j) {
                        if (!particles[j].active) { idx = j; break; }
                    }
                    if (idx == -1) continue;
                    float angle = (rand() % 628) * 0.01f;
                    float speed = speedMin + (rand() % 1000) * (speedMax - speedMin) / 1000.0f;
                    particles[idx] = {
                        (float)cx, (float)cy,
                        cosf(angle) * speed, sinf(angle) * speed,
                        0, (float)lifeMs, c,
                        sizeMin + rand() % (sizeMax - sizeMin + 1),
                        true
                    };
                    particleCount++;
                }
            }

            void updateParticles(int dt) {
                for (int i = 0; i < kMaxParticles; ++i) {
                    if (!particles[i].active) continue;
                    particles[i].x += particles[i].vx * dt / 16.67f;
                    particles[i].y += particles[i].vy * dt / 16.67f;
                    particles[i].vy += 0.15f * dt / 16.67f; // gravity
                    particles[i].life += dt;
                    if (particles[i].life >= particles[i].maxLife) {
                        particles[i].active = false;
                        particleCount--;
                    }
                }
            }

            void drawParticles() {
                for (int i = 0; i < kMaxParticles; ++i) {
                    if (!particles[i].active) continue;
                    float t = particles[i].life / particles[i].maxLife;
                    int sz = (int)(particles[i].size * (1.0f - t * 0.5f));
                    if (sz > 0) gDot((int)particles[i].x, (int)particles[i].y, sz, particles[i].c);
                }
            }

            // Screen shake effect
void triggerShake(int intensity, int durationMs) {
    shakeX = intensity; shakeY = intensity; shakeTime = durationMs;
}
void applyShake() {
    if (shakeTime > 0) {
        shakeX = (rand() % (shakeX * 2 + 1)) - shakeX;
        shakeY = (rand() % (shakeY * 2 + 1)) - shakeY;
        shakeTime -= 16;
    } else {
        shakeX = shakeY = 0;
    }
}

// ============================================================================
// ADVANCED VISUAL EFFECTS SYSTEMS
// ============================================================================

// Initialize color grading LUT
void initColorLUT() {
    for (int i = 0; i < 256; ++i) {
        float v = i / 255.0f;
        // S-curve for contrast
        float s = v * v * (3.0f - 2.0f * v);
        // Slight blue lift in shadows, warm highlights
        int r = (int)(s * 255 * 1.02f);
        int g = (int)(s * 255 * 1.0f);
        int b = (int)(s * 255 * 0.98f + (1.0f - s) * 10);
        r = clampInt(r, 0, 255);
        g = clampInt(g, 0, 255);
        b = clampInt(b, 0, 255);
        colorLUT[i] = (r << 16) | (g << 8) | b;
    }
}

// Apply color grading
color applyColorGrade(color c) {
    if (!colorGradingEnabled) return c;
    int r = (c >> 16) & 0xFF;
    int g = (c >> 8) & 0xFF;
    int b = c & 0xFF;
    r = (colorLUT[r] >> 16) & 0xFF;
    g = (colorLUT[g] >> 8) & 0xFF;
    b = colorLUT[b] & 0xFF;
    return (r << 16) | (g << 8) | b;
}

// Theme colors
void getThemeColors(ThemeMode theme, color& primary, color& secondary, color& accent, color& bg) {
    switch (theme) {
        case THEME_CYBER:
            primary = cCyan; secondary = cAccent2; accent = cAccent; bg = cBg;
            break;
        case THEME_NEON:
            primary = cAccent2; secondary = cAccent; accent = cCyan; bg = cBg;
            break;
        case THEME_FIRE:
            primary = cDanger; secondary = cGold; accent = cWarn; bg = cBg;
            break;
        case THEME_ICE:
            primary = cCyan; secondary = cAccent; accent = cGood; bg = cBg;
            break;
        case THEME_GOLD:
            primary = cGold; secondary = cWarn; accent = cAccent2; bg = cBg;
            break;
        default:
            primary = cAccent; secondary = cAccent2; accent = cGold; bg = cBg;
    }
}

// Update theme transition
void updateThemeTransition() {
    if (themeTransitioning) {
        themeTransitionFrame++;
        if (themeTransitionFrame >= 60) {
            themeTransitioning = false;
            themeTransitionFrame = 0;
        }
    }
}

// Trigger theme change
void triggerThemeChange(ThemeMode newTheme) {
    if (newTheme != currentTheme && !themeTransitioning) {
        currentTheme = newTheme;
        themeTransitioning = true;
        themeTransitionFrame = 0;
        // Trigger shockwave on theme change
        triggerShake(8, 300);
        spawnParticles(240, 120, 30, cGold, 2.0f, 6.0f, 2, 5, 1000);
    }
}

// Chromatic aberration effect
void drawChromaticAberration(int x, int y, int w, int h, color c) {
    if (!chromaticEnabled) return;
    int offset = chromaticOffset;
    color r = (c & 0xFF0000);
    color g = (c & 0x00FF00);
    color b = (c & 0x0000FF);
    // Draw RGB channels slightly offset
    gRect(x - offset, y, w, h, r);
    gRect(x, y, w, h, g);
    gRect(x + offset, y, w, h, b);
}

// Vignette effect
void drawVignette() {
    if (vignetteStrength <= 0) return;
    for (int y = 0; y < 240; ++y) {
        for (int x = 0; x < 480; ++x) {
            float dx = (x - 240) / 240.0f;
            float dy = (y - 120) / 120.0f;
            float dist = sqrtf(dx * dx + dy * dy);
            float vignette = 1.0f - dist * vignetteStrength;
            vignette = clampInt(vignette * 256, 0, 256) / 256.0f;
            // This would need pixel-level access, so we approximate with rectangles
        }
    }
    // Simplified vignette using corner rectangles
    int vignetteSize = 80;
    color vignetteColor = color(0, 0, 0);
    for (int i = 0; i < vignetteSize; ++i) {
        float alpha = (1.0f - i / (float)vignetteSize) * vignetteStrength;
        color c = mix8(cBgRgb, cBgRgb, (int)(alpha * 256));
        // Top
        gRect(0, i, 480, 1, c);
        // Bottom
        gRect(0, 239 - i, 480, 1, c);
        // Left
        gRect(i, 0, 1, 240, c);
        // Right
        gRect(479 - i, 0, 1, 240, c);
    }
}

// Scanline effect
void drawScanlines(float intensity) {
    if (intensity <= 0) return;
    for (int y = 0; y < 240; y += 2) {
        float alpha = intensity * 0.5f;
        int a = (int)(alpha * 256);
        color c = mix8(cBgRgb, cBgRgb, a);
        gRect(0, y, 480, 1, c);
    }
}

// CRT curvature simulation
void drawCRTCurvature() {
    if (crtCurvature <= 0) return;
    // Draw curved corners
    for (int i = 0; i < 30; ++i) {
        float curve = crtCurvature * (30 - i) / 30.0f;
        int offset = (int)(curve * 30);
        color c = mix8(cBgRgb, cLineRgb, (int)((1.0f - i / 30.0f) * 100));
        // Top-left
        gRect(i, i, offset, 1, c);
        gRect(i, i, 1, offset, c);
        // Top-right
        gRect(479 - i - offset, i, offset, 1, c);
        gRect(479 - i, i, 1, offset, c);
        // Bottom-left
        gRect(i, 239 - i, offset, 1, c);
        gRect(i, 239 - i - offset, 1, offset, c);
        // Bottom-right
        gRect(479 - i - offset, 239 - i, offset, 1, c);
        gRect(479 - i, 239 - i - offset, 1, offset, c);
    }
}

// Glitch effect
void triggerGlitch(int intensity, int duration) {
    glitchIntensity = intensity;
    glitchTimer = duration;
}

void updateGlitch() {
    if (glitchTimer > 0) {
        glitchTimer--;
        if (glitchTimer % 3 == 0) {
            chromaticOffset = (rand() % (glitchIntensity * 2 + 1)) - glitchIntensity;
        } else {
            chromaticOffset = 0;
        }
    } else {
        chromaticOffset = 0;
    }
}

// Particle trail system
void spawnParticleTrail(float x, float y, float vx, float vy, color c, int count) {
    ParticleTrail& trail = particleTrails[trailIndex];
    trail.active = true;
    trail.count = count;
    for (int i = 0; i < count; ++i) {
        trail.x[i] = x + (rand() % 10 - 5);
        trail.y[i] = y + (rand() % 10 - 5);
        trail.vx[i] = vx + (rand() % 20 - 10) * 0.1f;
        trail.vy[i] = vy + (rand() % 20 - 10) * 0.1f;
        trail.life[i] = 1.0f;
        trail.colors[i] = c;
    }
    trailIndex = (trailIndex + 1) % 8;
}

void updateParticleTrails(int dt) {
    float dtSec = dt / 1000.0f;
    for (int t = 0; t < 8; ++t) {
        ParticleTrail& trail = particleTrails[t];
        if (!trail.active) continue;
        bool anyAlive = false;
        for (int i = 0; i < trail.count; ++i) {
            if (trail.life[i] <= 0) continue;
            trail.x[i] += trail.vx[i] * dtSec;
            trail.y[i] += trail.vy[i] * dtSec;
            trail.vy[i] += 50.0f * dtSec; // gravity
            trail.life[i] -= dtSec * 2.0f;
            if (trail.life[i] > 0) anyAlive = true;
        }
        if (!anyAlive) trail.active = false;
    }
}

void drawParticleTrails() {
    for (int t = 0; t < 8; ++t) {
        ParticleTrail& trail = particleTrails[t];
        if (!trail.active) continue;
        for (int i = 0; i < trail.count; ++i) {
            if (trail.life[i] <= 0) continue;
            int alpha = (int)(trail.life[i] * 256);
            color c = mix8(colorToRgb(trail.colors[i]), cBgRgb, alpha);
            int sz = (int)(trail.life[i] * 3) + 1;
            gDot((int)trail.x[i], (int)trail.y[i], sz, c);
        }
    }
}

// Ribbon particles for velocity trails
void spawnRibbon(float x, float y, float width, color c) {
    if (ribbonCount >= 64) return;
    RibbonPoint& r = ribbonPoints[ribbonCount++];
    r.x = x; r.y = y; r.width = width; r.c = c; r.life = 1.0f;
}

void updateRibbons(int dt) {
    float dtSec = dt / 1000.0f;
    for (int i = 0; i < ribbonCount; ++i) {
        RibbonPoint& r = ribbonPoints[i];
        if (r.life <= 0) continue;
        r.life -= dtSec * 3.0f;
        r.width *= 0.98f;
    }
    // Compact array
    int write = 0;
    for (int i = 0; i < ribbonCount; ++i) {
        if (ribbonPoints[i].life > 0) {
            if (write != i) ribbonPoints[write] = ribbonPoints[i];
            write++;
        }
    }
    ribbonCount = write;
}

void drawRibbons() {
    for (int i = 0; i < ribbonCount; ++i) {
        RibbonPoint& r = ribbonPoints[i];
        if (r.life <= 0) continue;
        int alpha = (int)(r.life * 256);
        color c = mix8(colorToRgb(r.c), cBgRgb, (int)(alpha * 0.5f));
        int w = (int)r.width;
        gRect((int)r.x - w/2, (int)r.y - 1, w, 2, c);
    }
}

// Shockwave rings
void spawnShockwave(float x, float y, float maxRadius, color c, int thickness, int durationMs) {
    if (shockwaveCount >= 16) return;
    Shockwave& s = shockwaves[shockwaveCount++];
    s.x = x; s.y = y; s.radius = 0; s.maxRadius = maxRadius;
    s.c = c; s.life = 0; s.maxLife = durationMs; s.thickness = thickness; s.active = true;
}

void updateShockwaves(int dt) {
    for (int i = 0; i < shockwaveCount; ++i) {
        Shockwave& s = shockwaves[i];
        if (!s.active) continue;
        s.life += dt;
        s.radius = s.maxRadius * (s.life / s.maxLife);
        if (s.life >= s.maxLife) {
            s.active = false;
        }
    }
    // Compact
    int write = 0;
    for (int i = 0; i < shockwaveCount; ++i) {
        if (shockwaves[i].active) {
            if (write != i) shockwaves[write] = shockwaves[i];
            write++;
        }
    }
    shockwaveCount = write;
}

void drawShockwaves() {
    for (int i = 0; i < shockwaveCount; ++i) {
        Shockwave& s = shockwaves[i];
        if (!s.active) continue;
        float alpha = 1.0f - s.life / s.maxLife;
        int a = (int)(alpha * 256);
        color c = mix8(colorToRgb(s.c), cBgRgb, a);
        gRing((int)s.x, (int)s.y, (int)s.radius, c);
        if (s.thickness > 1) {
            gRing((int)s.x, (int)s.y, (int)s.radius - 1, c);
        }
    }
}

// Energy orbs
void spawnEnergyOrb(float x, float y, float vx, float vy, float radius, color c) {
    if (orbCount >= 8) return;
    EnergyOrb& o = energyOrbs[orbCount++];
    o.x = x; o.y = y; o.vx = vx; o.vy = vy;
    o.radius = 0; o.targetRadius = radius; o.c = c; o.pulsePhase = 0; o.active = true;
}

void updateEnergyOrbs(int dt) {
    float dtSec = dt / 1000.0f;
    for (int i = 0; i < orbCount; ++i) {
        EnergyOrb& o = energyOrbs[i];
        if (!o.active) continue;
        o.x += o.vx * dtSec;
        o.y += o.vy * dtSec;
        o.vy += 20.0f * dtSec; // gravity
        o.radius += (o.targetRadius - o.radius) * dtSec * 5.0f;
        o.pulsePhase += dtSec * 10.0f;
        if (o.radius > o.targetRadius * 1.5f) o.active = false;
    }
    int write = 0;
    for (int i = 0; i < orbCount; ++i) {
        if (energyOrbs[i].active) {
            if (write != i) energyOrbs[write] = energyOrbs[i];
            write++;
        }
    }
    orbCount = write;
}

void drawEnergyOrbs() {
    for (int i = 0; i < orbCount; ++i) {
        EnergyOrb& o = energyOrbs[i];
        if (!o.active) continue;
        float pulse = sinf(o.pulsePhase) * 0.2f + 0.8f;
        int r = (int)(o.radius * pulse);
        color c = applyColorGrade(o.c);
        // Outer glow
        for (int r2 = r + 4; r2 > r; --r2) {
            float alpha = (r2 - r) / 4.0f * 0.3f;
            color gc = mix8(colorToRgb(o.c), cBgRgb, (int)(alpha * 256));
            gRing((int)o.x, (int)o.y, r2, gc);
        }
        // Core
        gDot((int)o.x, (int)o.y, r, c);
        // Inner highlight
        gDot((int)o.x - r/3, (int)o.y - r/3, r/3, mix8(colorToRgb(o.c), cTextRgb, 200));
    }
}

// Hologram scanlines
void drawHologramScanlines() {
    if (!hologramMode) return;
    hologramPhase = (hologramPhase + 2) % 240;
    for (int y = 0; y < 240; y += 4) {
        int alpha = 30 + (int)(sinf((y + hologramPhase) * 0.1f) * 20);
        color c = mix8(cCyanRgb, cBgRgb, alpha);
        gRect(0, y, 480, 1, c);
    }
    // Horizontal sync lines
    for (int y = hologramPhase % 40; y < 240; y += 40) {
        gRect(0, y, 480, 1, mix8(cCyanRgb, cBgRgb, 80));
    }
}

// Update all advanced effects
void updateAdvancedEffects(int dt) {
    updateThemeTransition();
    updateGlitch();
    updateParticleTrails(dt);
    updateRibbons(dt);
    updateShockwaves(dt);
    updateEnergyOrbs(dt);
    hologramPhase = (hologramPhase + 1) % 240;
    
    // Update profiler
    if (profilerEnabled) {
        profilerFrame++;
    }
    
    // Update easter eggs
    if (easterEggTimer > 0) {
        easterEggTimer -= dt;
        if (easterEggTimer <= 0) {
            activeEasterEgg = EGG_NONE;
        }
    }
    
    // Update hologram elements
    for (int i = 0; i < hologramCount; ++i) {
        HologramElement& h = hologramElements[i];
        if (!h.active) continue;
        h.pulsePhase += dt * 0.005f;
        h.rotation += dt * 0.0005f;
    }
    
    // Update notifications
    for (int i = 0; i < notificationCount; ++i) {
        Notification& n = notifications[i];
        if (!n.active) continue;
        n.life -= dt;
        n.y += (n.targetY - n.y) * 0.1f;
        if (n.life <= 0) n.active = false;
    }
    
    // Update floating numbers
    for (int i = 0; i < floatingNumberCount; ++i) {
        FloatingNumber& fn = floatingNumbers[i];
        if (!fn.active) continue;
        fn.y += fn.vy * dt * 0.001f;
        fn.vy -= 0.05f * dt * 0.001f; // gravity
        fn.scale += fn.scaleVel * dt * 0.001f;
        fn.life -= dt;
        if (fn.life <= 0) fn.active = false;
    }
    
    // Update achievements
    for (int i = 0; i < achievementCount; ++i) {
        Achievement& a = achievements[i];
        if (!a.unlocked) continue;
        if (a.displayProgress < 1.0f) {
            a.displayProgress += dt * 0.002f;
            if (a.displayProgress > 1.0f) a.displayProgress = 1.0f;
        }
    }
    
    // Update combo system
    if (comboTimer > 0) {
        comboTimer -= dt;
        if (comboTimer <= 0) {
            comboCount = 0;
        }
    }
    
    // Update time dilation
    timeScaleVel += (timeScaleTarget - timeScale) * 0.1f;
    timeScale += timeScaleVel * dt * 0.001f;
    timeScaleVel *= 0.9f;
    timeScale = clampInt(timeScale * 1000, 100, 3000) / 1000.0f;
    
    // Update parallax layers
    for (int i = 0; i < 5; ++i) {
        ParallaxLayer& p = parallaxLayers[i];
        p.x += p.speedX * dt * 0.001f;
        p.y += p.speedY * dt * 0.001f;
        if (p.x > 480) p.x -= 480;
        if (p.x < -480) p.x += 480;
        if (p.y > 240) p.y -= 240;
        if (p.y < -240) p.y += 240;
    }
    
    // Update procedural background
    for (int i = 0; i < bgElementCount; ++i) {
        BGElement& b = bgElements[i];
        if (!b.active) continue;
        b.x += b.vx * dt * 0.001f;
        b.y += b.vy * dt * 0.001f;
        b.rotation += b.rotSpeed * dt * 0.001f;
        if (b.x < -50 || b.x > 530 || b.y < -50 || b.y > 290) {
            b.active = false;
        }
    }
    
    // Update input visualization
    inputViz.lx = Controller1.Axis4.position(pct) / 100.0f;
    inputViz.ly = Controller1.Axis3.position(pct) / 100.0f;
    inputViz.rx = Controller1.Axis1.position(pct) / 100.0f;
    inputViz.ry = Controller1.Axis2.position(pct) / 100.0f;
    for (int i = 0; i < 12; ++i) {
        bool pressed = false;
        switch(i) {
            case 0: pressed = Controller1.ButtonL1.pressing(); break;
            case 1: pressed = Controller1.ButtonL2.pressing(); break;
            case 2: pressed = Controller1.ButtonR1.pressing(); break;
            case 3: pressed = Controller1.ButtonR2.pressing(); break;
            case 4: pressed = Controller1.ButtonA.pressing(); break;
            case 5: pressed = Controller1.ButtonB.pressing(); break;
            case 6: pressed = Controller1.ButtonX.pressing(); break;
            case 7: pressed = Controller1.ButtonY.pressing(); break;
            case 8: pressed = Controller1.ButtonUp.pressing(); break;
            case 9: pressed = Controller1.ButtonDown.pressing(); break;
            case 10: pressed = Controller1.ButtonLeft.pressing(); break;
            case 11: pressed = Controller1.ButtonRight.pressing(); break;
        }
        inputViz.buttons[i] = pressed;
        if (pressed) {
            inputViz.pressAnim[i] = 1.0f;
        } else {
            inputViz.pressAnim[i] *= 0.9f;
        }
    }
    
    // Update drift combo
    if (isDrifting) {
        driftCombo++;
        if (driftCombo > maxCombo) maxCombo = driftCombo;
    } else {
        driftCombo = 0;
    }
}

// Draw all advanced effects
void drawAdvancedEffects() {
    if (vignetteStrength > 0) drawVignette();
    if (scanlineIntensity > 0) drawScanlines(scanlineIntensity);
    if (crtCurvature > 0) drawCRTCurvature();
    if (hologramMode) drawHologramScanlines();
    drawParticleTrails();
    drawRibbons();
    drawShockwaves();
    drawEnergyOrbs();
}

// Trigger effects based on game events
void triggerVelocityEffects(const Tel& t) {
    float speed = sqrtf(t.sLeft * t.sLeft + t.sRight * t.sRight) / 100.0f;
    if (speed > 0.7f) {
        // High speed - spawn ribbons
        spawnRibbon(171, 106, speed * 20, cCyan);
        if (rand() % 10 == 0) {
            spawnShockwave(171, 106, 50, cCyan, 2, 200);
        }
    }
    if (speed > 0.9f) {
        spawnEnergyOrb(171 + (rand() % 40 - 20), 106 + (rand() % 40 - 20), 
                       (rand() % 200 - 100) * 0.1f, (rand() % 200 - 100) * 0.1f,
                       15, cGold);
    }
}

void triggerThemeByPerformance(const Tel& t) {
    // Auto-switch theme based on performance
    if (t.bat <= 20 && currentTheme != THEME_FIRE) {
        triggerThemeChange(THEME_FIRE);
    } else if (t.mTemp[t.hot] >= 60 && currentTheme != THEME_ICE) {
        triggerThemeChange(THEME_ICE);
    } else if (t.online == 4 && t.bat > 50 && t.mTemp[t.hot] < 40 && currentTheme != THEME_GOLD) {
        triggerThemeChange(THEME_GOLD);
    } else if (t.online < 4 && currentTheme != THEME_FIRE) {
        triggerThemeChange(THEME_FIRE);
    }
}

// ============================================================================
// NEW ADVANCED EFFECTS IMPLEMENTATIONS
// ============================================================================

// Parallax background
void drawParallaxBackground() {
    for (int i = 0; i < 5; ++i) {
        ParallaxLayer& p = parallaxLayers[i];
        if (p.opacity <= 0) continue;
        
        int layers = 3;
        for (int l = 0; l < layers; ++l) {
            float lx = p.x + l * 480;
            float ly = p.y + l * 240;
            
            switch (p.textureType) {
                case 0: // Stars
                    for (int s = 0; s < 20; ++s) {
                        int sx = (int)(lx + (s * 73 + i * 17) % 480);
                        int sy = (int)(ly + (s * 97 + i * 23) % 240);
                        int sz = 1 + ((s * 13 + i * 7) % 3);
                        color sc = mix8(p.tint, cBg, (int)(p.opacity * 256));
                        gDot(sx, sy, sz, sc);
                    }
                    break;
                case 1: // Grid
                    for (int gx = 0; gx < 480; gx += 40) {
                        for (int gy = 0; gy < 240; gy += 40) {
                            int sx = (int)(lx + gx);
                            int sy = (int)(ly + gy);
                            color gc = mix8(p.tint, cBg, (int)(p.opacity * 128));
                            gDot(sx, sy, 1, gc);
                        }
                    }
                    break;
                case 2: // Particles
                    for (int s = 0; s < 15; ++s) {
                        int sx = (int)(lx + (s * 101 + i * 31 + uiFrame * 2) % 480);
                        int sy = (int)(ly + (s * 113 + i * 41 + uiFrame * 1) % 240);
                        color pc = mix8(p.tint, cBg, (int)(p.opacity * 200));
                        gDot(sx, sy, 2, pc);
                    }
                    break;
                case 3: // Nebula
                    for (int n = 0; n < 5; ++n) {
                        int nx = (int)(lx + (n * 157 + i * 53) % 480);
                        int ny = (int)(ly + (n * 173 + i * 67) % 240);
                        int nr = 30 + (n * 23 + i * 11) % 50;
                        color nc = mix8(p.tint, cBg, (int)(p.opacity * 80));
                        gRing(nx, ny, nr, nc);
                    }
                    break;
            }
        }
    }
}

// Procedural background
void drawProceduralBackground() {
    for (int i = 0; i < bgElementCount; ++i) {
        BGElement& b = bgElements[i];
        if (!b.active) continue;
        
        int x = (int)b.x;
        int y = (int)b.y;
        int sz = (int)b.size;
        color c = b.c;
        
        switch (b.type) {
            case 0: // Circle
                gRing(x, y, sz, c);
                break;
            case 1: // Square
                gBox(x - sz/2, y - sz/2, sz, sz, c);
                break;
            case 2: // Triangle (approximated with lines)
                gLine(x, y - sz/2, x + sz/2, y + sz/2, c);
                gLine(x + sz/2, y + sz/2, x - sz/2, y + sz/2, c);
                gLine(x - sz/2, y + sz/2, x, y - sz/2, c);
                break;
            case 3: // Star
                for (int s = 0; s < 5; ++s) {
                    float angle = s * 2.0f * 3.14159f / 5.0f + b.rotation;
                    int sx = x + (int)(sz * cosf(angle));
                    int sy = y + (int)(sz * sinf(angle));
                    gLine(x, y, sx, sy, c);
                }
                break;
        }
    }
}

// Hologram elements
void drawHologramElements() {
    for (int i = 0; i < hologramCount; ++i) {
        HologramElement& h = hologramElements[i];
        if (!h.active) continue;
        
        int x = (int)h.x;
        int y = (int)h.y;
        int sz = (int)(h.scale * 20);
        color c = h.c;
        
        // Pulsing effect
        float pulse = 1.0f + 0.1f * sinf(h.pulsePhase);
        sz = (int)(sz * pulse);
        
        // Scanline effect for hologram
        for (int sy = y - sz; sy < y + sz; sy += 2) {
            float alpha = 0.5f + 0.5f * sinf(h.pulsePhase + sy * 0.1f);
            color scanC = mix8(colorToRgb(c), cBgRgb, (int)(alpha * 256));
            gRect(x - sz, sy, sz * 2, 1, scanC);
        }
        
        // Core element
        switch (h.type) {
            case 0: // Text
                gTextC(x, y, 8, c, "HUD");
                break;
            case 1: // Icon
                gRing(x, y, sz, c);
                gRing(x, y, sz/2, mix8(c, cText, 128));
                break;
            case 2: // Graph
                for (int g = 0; g < 10; ++g) {
                    int gx = x - sz + g * sz / 5;
                    int gy = y + (int)(sz * 0.5f * sinf(g * 0.5f + h.pulsePhase));
                    gDot(gx, gy, 2, c);
                }
                break;
            case 3: // 3D Model (wireframe cube)
                {
                    float angle = h.rotation;
                    float ca = cosf(angle), sa = sinf(angle);
                    int cx = x, cy = y;
                    int half = sz / 2;
                    // Draw wireframe cube
                    int pts[8][2];
                    for (int v = 0; v < 8; ++v) {
                        float vx = (v & 1) ? half : -half;
                        float vy = (v & 2) ? half : -half;
                        float vz = (v & 4) ? half : -half;
                        // Simple orthographic projection
                        float px = vx * ca - vz * sa;
                        float py = vy;
                        pts[v][0] = cx + (int)px;
                        pts[v][1] = cy + (int)py;
                    }
                    // Edges
                    int edges[12][2] = {{0,1},{1,3},{3,2},{2,0},{4,5},{5,7},{7,6},{6,4},{0,4},{1,5},{2,6},{3,7}};
                    for (int e = 0; e < 12; ++e) {
                        gLine(pts[edges[e][0]][0], pts[edges[e][0]][1], 
                              pts[edges[e][1]][0], pts[edges[e][1]][1], c);
                    }
                }
                break;
        }
    }
}

// Notifications
void drawNotifications() {
    for (int i = 0; i < notificationCount; ++i) {
        Notification& n = notifications[i];
        if (!n.active) continue;
        
        int y = (int)n.y;
        int w = 300;
        int h = 30;
        int x = 480 - w - 10;
        
        // Background with slide-in animation
        float progress = n.life / n.maxLife;
        if (progress > 0.9f) progress = 1.0f - (progress - 0.9f) * 10.0f;
        int slideX = x + (int)((1.0f - progress) * 50);
        
        gRect(slideX, y, w, h, cPanel);
        gBox(slideX, y, w, h, n.c);
        gLine(slideX, y, slideX + 4, y, n.c);
        gLine(slideX, y + h, slideX + 4, y + h, n.c);
        
        gText(slideX + 10, y + 18, cText, "%s", n.text);
    }
}

void addNotification(const char* text, color c, int durationMs) {
    if (notificationCount >= 8) return;
    Notification& n = notifications[notificationCount++];
    strncpy(n.text, text, 63);
    n.text[63] = 0;
    n.c = c;
    n.life = durationMs;
    n.maxLife = durationMs;
    n.y = 30 + (notificationCount - 1) * 35;
    n.targetY = n.y;
    n.active = true;
}

// Floating numbers
void drawFloatingNumbers() {
    for (int i = 0; i < floatingNumberCount; ++i) {
        FloatingNumber& fn = floatingNumbers[i];
        if (!fn.active) continue;
        
        int x = (int)fn.x;
        int y = (int)fn.y;
        int sz = (int)(fn.scale * 12);
        
        Brain.Screen.setFont(prop20);
        gTextC(x, y, sz / 2, fn.c, "%s", fn.text);
        Brain.Screen.setFont(mono12);
    }
}

void spawnFloatingNumber(float x, float y, const char* text, color c) {
    if (floatingNumberCount >= 16) return;
    FloatingNumber& fn = floatingNumbers[floatingNumberCount++];
    fn.x = x;
    fn.y = y;
    fn.vy = -2.0f;
    strncpy(fn.text, text, 15);
    fn.text[15] = 0;
    fn.c = c;
    fn.life = 1000;
    fn.maxLife = 1000;
    fn.scale = 1.0f;
    fn.scaleVel = 0.5f;
    fn.active = true;
}

// Achievements
void drawAchievements() {
    for (int i = 0; i < achievementCount; ++i) {
        Achievement& a = achievements[i];
        if (!a.unlocked) continue;
        
        float progress = a.displayProgress;
        if (progress <= 0) continue;
        
        int y = 30 + i * 40;
        int w = (int)(300 * progress);
        
        gRect(10, y, w, 35, cPanel);
        gBox(10, y, w, 35, a.c);
        gLine(10, y, 10 + 4, y, a.c);
        gLine(10, y + 35, 10 + 4, y + 35, a.c);
        
        gText(20, y + 12, cText, "%s", a.name);
        gText(20, y + 24, cMuted, "%s", a.desc);
    }
}

void unlockAchievement(const char* name, const char* desc, color c) {
    for (int i = 0; i < achievementCount; ++i) {
        if (strcmp(achievements[i].name, name) == 0) return; // Already exists
    }
    if (achievementCount >= 16) return;
    Achievement& a = achievements[achievementCount++];
    strncpy(a.name, name, 31);
    a.name[31] = 0;
    strncpy(a.desc, desc, 63);
    a.desc[63] = 0;
    a.c = c;
    a.unlocked = true;
    a.unlockTime = Brain.Timer.time(msec);
    a.displayProgress = 0.0f;
    
    // Spawn celebration particles
    spawnParticles(240, 120, 20, c, 2.0f, 5.0f, 2, 4, 1500);
    triggerShake(10, 500);
    addNotification(name, c, 3000);
}

// Radial menu
void drawRadialMenu() {
    if (!radialMenuOpen || radialMenuCount == 0) return;
    
    int cx = 240, cy = 120;
    float radius = 80 * radialMenuProgress;
    
    // Background
    gRing(cx, cy, (int)radius + 20, cPanel);
    
    for (int i = 0; i < radialMenuCount; ++i) {
        RadialMenuItem& item = radialMenuItems[i];
        float angle = item.angle - 3.14159f / 2; // Start from top
        int ix = cx + (int)(radius * cosf(angle));
        int iy = cy + (int)(radius * sinf(angle));
        
        color c = item.enabled ? item.c : cMuted;
        gDot(ix, iy, 15, c);
        gRing(ix, iy, 18, c);
        
        // Label
        gTextC(ix, iy + 30, 7, cText, "%s", item.label);
    }
    
    // Center
    gDot(cx, cy, 10, cAccent);
    gTextC(cx, cy + 25, 7, cText, "MENU");
}

void openRadialMenu() {
    radialMenuOpen = true;
    radialMenuProgress = 0.0f;
    // Default items
    radialMenuCount = 4;
    strcpy(radialMenuItems[0].label, "THEME");
    radialMenuItems[0].c = cAccent;
    radialMenuItems[0].angle = 0;
    radialMenuItems[0].enabled = true;
    strcpy(radialMenuItems[1].label, "PARTICLES");
    radialMenuItems[1].c = cAccent2;
    radialMenuItems[1].angle = 90 * 0.0174533f;
    radialMenuItems[1].enabled = true;
    strcpy(radialMenuItems[2].label, "REPLAY");
    radialMenuItems[2].c = cGood;
    radialMenuItems[2].angle = 180 * 0.0174533f;
    radialMenuItems[2].enabled = true;
    strcpy(radialMenuItems[3].label, "SETTINGS");
    radialMenuItems[3].c = cWarn;
    radialMenuItems[3].angle = 270 * 0.0174533f;
    radialMenuItems[3].enabled = true;
}

void closeRadialMenu() {
    radialMenuOpen = false;
}

// Profiler
void drawProfiler() {
    int x = 10, y = 100;
    gCard(x, y, 200, 20 + profilerCount * 18, "PROFILER");
    
    for (int i = 0; i < profilerCount; ++i) {
        ProfilerSection& p = profilerSections[i];
        float avg = p.callCount > 0 ? (float)p.totalTime / p.callCount : 0;
        int py = y + 20 + i * 18;
        
        // Graph bar
        int barW = (int)(avg * 2); // Scale for visibility
        barW = clampInt(barW, 0, 180);
        gRect(x + 10, py, barW, 12, p.graphColor);
        
        gText(x + 10, py + 12, cText, "%s: %.2fms (max %d)", p.name, avg, p.maxTime);
    }
}

void profilerBegin(const char* name) {
    if (!profilerEnabled) return;
    for (int i = 0; i < profilerCount; ++i) {
        if (strcmp(profilerSections[i].name, name) == 0) {
            profilerSections[i].startTime = Brain.Timer.time(msec);
            return;
        }
    }
    if (profilerCount >= 16) return;
    ProfilerSection& p = profilerSections[profilerCount++];
    p.name = name;
    p.startTime = Brain.Timer.time(msec);
    p.totalTime = 0;
    p.maxTime = 0;
    p.callCount = 0;
    p.graphColor = color(rand() % 256, rand() % 256, rand() % 256);
}

void profilerEnd(const char* name) {
    if (!profilerEnabled) return;
    for (int i = 0; i < profilerCount; ++i) {
        if (strcmp(profilerSections[i].name, name) == 0) {
            uint32_t elapsed = Brain.Timer.time(msec) - profilerSections[i].startTime;
            profilerSections[i].totalTime += elapsed;
            if (elapsed > profilerSections[i].maxTime) profilerSections[i].maxTime = elapsed;
            profilerSections[i].callCount++;
            return;
        }
    }
}

// Easter eggs
void drawEasterEggs() {
    switch (activeEasterEgg) {
        case EGG_KONAMI:
            // Rainbow flash
            for (int y = 0; y < 240; y += 4) {
                float hue = (uiFrame * 0.1f + y * 0.05f);
                int r = (int)(128 + 127 * sinf(hue));
                int g = (int)(128 + 127 * sinf(hue + 2.094f));
                int b = (int)(128 + 127 * sinf(hue + 4.188f));
                gRect(0, y, 480, 2, color(r, g, b));
            }
            gTextC(240, 120, 20, cGold, "KONAMI CODE ACTIVATED!");
            break;
        case EGG_RAINBOW:
            // Full screen rainbow
            for (int x = 0; x < 480; x += 2) {
                float hue = (uiFrame * 0.05f + x * 0.02f);
                int r = (int)(128 + 127 * sinf(hue));
                int g = (int)(128 + 127 * sinf(hue + 2.094f));
                int b = (int)(128 + 127 * sinf(hue + 4.188f));
                gRect(x, 0, 2, 240, color(r, g, b));
            }
            break;
        case EGG_MATRIX:
            // Matrix rain
            for (int c = 0; c < 480; c += 12) {
                for (int r = 0; r < 20; ++r) {
                    int y = (r * 12 + uiFrame * 2 + c * 7) % 240;
                    int g = 255 - r * 10;
                    gDot(c, y, 1, color(0, g, 0));
                }
            }
            break;
        case EGG_RETRO:
            // Retro scanlines + green tint
            for (int y = 0; y < 240; y += 2) {
                gRect(0, y, 480, 1, color(0, 50, 0));
            }
            gTextC(240, 120, 15, color(0, 255, 0), "RETRO MODE");
            break;
        case EGG_HYPER:
            // Hyper speed lines
            for (int i = 0; i < 50; ++i) {
                float angle = (uiFrame * 0.1f + i * 0.125f);
                int x1 = 240 + (int)(100 * cosf(angle));
                int y1 = 120 + (int)(100 * sinf(angle));
                int x2 = 240 + (int)(300 * cosf(angle));
                int y2 = 120 + (int)(300 * sinf(angle));
                color lc = color(rand() % 256, rand() % 256, rand() % 256);
                gLine(x1, y1, x2, y2, lc);
            }
            break;
        case EGG_CYBERPUNK:
            // Cyberpunk 2077 style - neon grid with glitch
            for (int y = 0; y < 240; y += 8) {
                float wave = sinf(uiFrame * 0.05f + y * 0.1f) * 0.5f + 0.5f;
                color c = mix8(cAccent2Rgb, cCyanRgb, (int)(wave * 256));
                gRect(0, y, 480, 1, c);
            }
            for (int x = 0; x < 480; x += 16) {
                float wave = sinf(uiFrame * 0.03f + x * 0.05f) * 0.5f + 0.5f;
                color c = mix8(cAccentRgb, cGoldRgb, (int)(wave * 256));
                gRect(x, 0, 1, 240, c);
            }
            // Glitch text
            if ((uiFrame / 10) % 2 == 0) {
                gTextC(240 + (rand() % 6 - 3), 120 + (rand() % 6 - 3), 20, cGold, "CYBERPUNK MODE");
            }
            break;
        case EGG_GLITCH_ART:
            // Glitch art - corrupted visuals
            for (int i = 0; i < 20; ++i) {
                int x = rand() % 480;
                int y = rand() % 240;
                int w = rand() % 100 + 10;
                int h = rand() % 20 + 2;
                color c = color(rand() % 256, rand() % 256, rand() % 256);
                gRect(x, y, w, h, c);
            }
            // Scanline corruption
            for (int y = 0; y < 240; y += 4) {
                if (rand() % 10 == 0) {
                    int x = rand() % 480;
                    int w = rand() % 200;
                    gRect(x, y, w, 2, color(rand() % 256, rand() % 256, rand() % 256));
                }
            }
            break;
        case EGG_NEON_DREAMS:
            // Neon dreams - soft glowing aesthetic
            for (int y = 0; y < 240; y += 2) {
                float hue = (uiFrame * 0.02f + y * 0.01f);
                int r = (int)(128 + 127 * sinf(hue));
                int g = (int)(128 + 127 * sinf(hue + 2.094f));
                int b = (int)(128 + 127 * sinf(hue + 4.188f));
                color c = color(r, g, b);
                gRect(0, y, 480, 1, mix8(c, cBg, 200));
            }
            // Floating orbs
            for (int i = 0; i < 8; ++i) {
                float angle = uiFrame * 0.01f + i * 0.785f;
                int x = 240 + (int)(100 * cosf(angle));
                int y = 120 + (int)(60 * sinf(angle));
                int r = 15 + (int)(10 * sinf(uiFrame * 0.05f + i));
                color c = color(rand() % 128 + 128, rand() % 128 + 128, 255);
                gRing(x, y, r, c);
                gDot(x, y, r/2, mix8(c, cText, 128));
            }
            break;
        case EGG_VOID: {
            // The Void - dark, mysterious
            gRect(0, 0, 480, 240, cBg);
            for (int i = 0; i < 100; ++i) {
                int x = (rand() * 480) % 480;
                int y = (rand() * 240) % 240;
                int sz = rand() % 3;
                color c = color(rand() % 50, rand() % 50, rand() % 80 + 100);
                gDot(x, y, sz, c);
            }
            // Pulsing void center
            float pulse = sinf(uiFrame * 0.1f) * 0.5f + 0.5f;
            int voidR = (int)(50 * pulse + 20);
            gRing(240, 120, voidR, cAccent2);
            gRing(240, 120, voidR + 10, mix8(cAccent2Rgb, cBgRgb, 128));
            gTextC(240, 120, 15, cAccent2, "THE VOID STARES BACK");
            break;
                }
                            case EGG_HOLOGRAM: {
                        // Holographic projection effect
                        for (int y = 0; y < 240; y += 3) {
                            float alpha = 0.3f + 0.2f * sinf(uiFrame * 0.05f + y * 0.1f);
                            color c = mix8(cCyanRgb, cBgRgb, (int)(alpha * 256));
                            gRect(0, y, 480, 1, c);
                        }
                        // Floating hologram elements
                        for (int i = 0; i < 6; ++i) {
                            float angle = uiFrame * 0.02f + i * 1.047f;
                            int x = 240 + (int)(120 * cosf(angle));
                            int y = 120 + (int)(80 * sinf(angle));
                            int r = 10 + (int)(5 * sinf(uiFrame * 0.08f + i));
                            color c = mix8(cCyanRgb, cTextRgb, 128);
                            gRing(x, y, r, c);
                            gLine(240, 120, x, y, mix8(c, cBg, 200));
                        }
                        // Hologram text
                        if ((uiFrame / 15) % 2 == 0) {
                            gTextC(240, 120, 15, cCyan, "HOLOGRAPHIC PROJECTION");
                        }
                        break;
                    case EGG_SYNTHWAVE: {
                        // Synthwave sunset aesthetic
                        for (int y = 0; y < 240; ++y) {
                            float t = y / 240.0f;
                            int r = (int)(20 + 100 * t + 50 * sinf(uiFrame * 0.01f));
                            int g = (int)(10 + 30 * t);
                            int b = (int)(50 + 100 * (1.0f - t) + 50 * cosf(uiFrame * 0.01f));
                            color c = color(clampInt(r, 0, 255), clampInt(g, 0, 255), clampInt(b, 0, 255));
                            gRect(0, y, 480, 1, c);
                        }
                        // Grid lines
                        for (int i = 0; i < 20; ++i) {
                            float y = 240 - i * 12 + uiFrame * 0.5f;
                            if (y < 0) y += 240;
                            color c = mix8(cGoldRgb, cAccent2Rgb, (int)((i / 20.0f) * 256));
                            gRect(0, (int)y, 480, 1, c);
                        }
                        // Sun
                        int sunY = 180 + (int)(20 * sinf(uiFrame * 0.03f));
                        gDot(240, sunY, 30, cGold);
                        gRing(240, sunY, 35, cAccent2);
                        break;
                                        }
                                        case EGG_DIGITAL_RAIN: {
                        // Digital rain - green code falling
                        for (int x = 0; x < 480; x += 10) {
                            for (int i = 0; i < 15; ++i) {
                                int y = (i * 16 + uiFrame * 3 + x * 7) % 240;
                                int brightness = 255 - i * 15;
                                color c = color(0, brightness, 0);
                                gTextC(x + 5, y, 7, c, "%c", '0' + (rand() % 10));
                            }
                        }
                        break;
                                                            }
                                                            case EGG_PLASMA: {
                        // Plasma effect
                        for (int y = 0; y < 240; y += 2) {
                            for (int x = 0; x < 480; x += 2) {
                                float v = sinf(x * 0.05f + uiFrame * 0.02f) + sinf(y * 0.03f + uiFrame * 0.01f) + 
                                          sinf((x + y) * 0.02f + uiFrame * 0.03f) + sinf(sqrtf(x*x + y*y) * 0.05f + uiFrame * 0.01f);
                                v = (v + 4.0f) / 8.0f;
                                int r = (int)(128 + 127 * sinf(v * 6.28f));
                                int g = (int)(128 + 127 * sinf(v * 6.28f + 2.094f));
                                int b = (int)(128 + 127 * sinf(v * 6.28f + 4.188f));
                                gRect(x, y, 2, 2, color(r, g, b));
                            }
                        }
                        break;
                    default:
                        break;
                }
            }

void checkKonamiCode(int button) {
    if (button == konamiCode[konamiIndex]) {
        konamiIndex++;
        if (konamiIndex >= 10) {
            konamiActivated = true;
            activeEasterEgg = EGG_KONAMI;
            easterEggTimer = 5000;
            konamiIndex = 0;
            unlockAchievement("KONAMI", "Up Up Down Down Left Left Right Right A B", cGold);
        }
    } else {
        konamiIndex = 0;
    }
}

// Input visualization
void drawInputVisualization() {
    if (selectedTab != 3) return; // Only on INPUT tab
    
    int cx = 400, cy = 100;
    int r = 30;
    
    // Left stick
    gRing(cx - 50, cy, r, cLine);
    int lx = cx - 50 + (int)(inputViz.lx * r);
    int ly = cy - (int)(inputViz.ly * r);
    gDot(lx, ly, 8, cAccent);
    gLine(cx - 50, cy, lx, ly, cAccent);
    
    // Right stick
    gRing(cx + 50, cy, r, cLine);
    int rx = cx + 50 + (int)(inputViz.rx * r);
    int ry = cy - (int)(inputViz.ry * r);
    gDot(rx, ry, 8, cAccent2);
    gLine(cx + 50, cy, rx, ry, cAccent2);
    
    // Buttons
    for (int i = 0; i < 12; ++i) {
        int bx = 320 + (i % 6) * 25;
        int by = 180 + (i / 6) * 25;
        float anim = inputViz.pressAnim[i];
        color bc = inputViz.buttons[i] ? cAccent : cLine;
        if (anim > 0) bc = mix8(bc, cText, (int)(anim * 256));
        gRect(bx, by, 20, 20, inputViz.buttons[i] ? cAccent : cBg);
        gBox(bx, by, 20, 20, bc);
        gTextC(bx + 10, by + 12, 7, inputViz.buttons[i] ? cBg : cText, "%s", buttonNames[i]);
    }
}

// Minimap
void drawMinimap() {
    int x = minimapX, y = minimapY, s = minimapSize;
    gRect(x, y, s, s, cPanel);
    gBox(x, y, s, s, cLine);
    
    // Draw field elements (simplified)
    // Robot position
    int rx = x + s/2;
    int ry = y + s/2;
    gDot(rx, ry, 4, cAccent);
    
    // Direction indicator
    float angle = 0; // Would use actual heading
    int dx = rx + (int)(6 * cosf(angle));
    int dy = ry + (int)(6 * sinf(angle));
    gLine(rx, ry, dx, dy, cAccent);
}

// ============================================================================
// EPIC MATCH OUTRO - Cinematic match ending sequence
// ============================================================================

// Color shift for dramatic effect
color shiftColor(color base, float phase, float intensity) {
    // Simple hue shift using RGB manipulation
    int r = (base >> 16) & 0xFF;
    int g = (base >> 8) & 0xFF;
    int b = base & 0xFF;
    float s = sinf(phase) * intensity;
    r = clampInt(r + (int)(s * 50), 0, 255);
    g = clampInt(g + (int)(s * 30), 0, 255);
    b = clampInt(b + (int)(s * 80), 0, 255);
    return color(r, g, b);
}

void matchOutro() {
    Tel t;
    readTel(t);
    
    // Determine match outcome vibe
    bool dominated = (t.online == 4 && t.mTemp[t.hot] < 50 && t.bat > 30);
    bool struggled = (t.online < 4 || t.mTemp[t.hot] >= 60 || t.bat <= 20);
    
    color themeAccent = dominated ? cGood : struggled ? cDanger : cAccent;
    color themeAccent2 = dominated ? cAccent : struggled ? cWarn : cAccent2;
    
    // Extract RGB components for mixing (color is 0xRRGGBB)
    Rgb themeAccentRgb = {(int)((themeAccent >> 16) & 0xFF), (int)((themeAccent >> 8) & 0xFF), (int)(themeAccent & 0xFF)};
    Rgb themeAccent2Rgb = {(int)((themeAccent2 >> 16) & 0xFF), (int)((themeAccent2 >> 8) & 0xFF), (int)(themeAccent2 & 0xFF)};
    
    // Reset particles
    for (int i = 0; i < kMaxParticles; ++i) particles[i].active = false;
    particleCount = 0;
    shakeX = shakeY = shakeTime = 0;
    
    // Trigger epic outro effects
    triggerGlitch(10, 500);
    triggerShake(15, 1000);
    spawnShockwave(240, 120, 400, cGold, 5, 1000);
    for (int i = 0; i < 8; ++i) {
        spawnEnergyOrb(240 + (rand() % 80 - 40), 120 + (rand() % 80 - 40),
                       (rand() % 400 - 200) * 0.1f, (rand() % 400 - 200) * 0.1f,
                       20 + rand() % 20, cGold);
    }
    for (int i = 0; i < 32; ++i) {
        spawnParticleTrail(240, 120, (rand() % 400 - 200) * 0.1f, (rand() % 400 - 200) * 0.1f,
                          cGold, 8);
    }
    triggerGlitch(15, 2000);
    triggerShake(20, 3000);
    
    // Phase 0: DRAMATIC FREEZE FRAME (0-500ms)
    for (int frame = 0; frame <= 30; ++frame) {
        Brain.Screen.setFillColor(cBg);
        Brain.Screen.clearScreen(cBg);
        
        float progress = frame / 30.0f;
        float ease = 1.0f - powf(1.0f - progress, 3); // ease out cubic
        
        // Expanding shockwave rings
        int rings = 5;
        for (int r = 0; r < rings; ++r) {
            float ringProg = fmodf(ease * 3.0f + r * 0.2f, 1.0f);
            int radius = (int)(ringProg * 300);
            color rc = mix8(themeAccentRgb, cBgRgb, (int)(ringProg * 256));
            gRing(240 + shakeX, 120 + shakeY, radius, rc);
        }
        
        // Center flash
        if (frame < 8) {
            int flashSize = frame * 60;
            gRect(240 - flashSize/2 + shakeX, 120 - flashSize/2 + shakeY, flashSize, flashSize, 
                  frame % 2 == 0 ? themeAccent : themeAccent2);
        }
        
        // "MATCH COMPLETE" text with scale-in
        if (frame > 10) {
            float textScale = (frame - 10) / 20.0f;
            textScale = clampInt(textScale * 100, 0, 100) / 100.0f;
            Brain.Screen.setFont(prop60);
                        color tc = mix8(cTextRgb, themeAccentRgb, (int)(ease * 256));
                        gTextC(240 + shakeX, 100 + shakeY, 30, tc, "MATCH COMPLETE");
                        Brain.Screen.setFont(mono12);
                    }
        
                    // Draw advanced effects during outro
                    drawAdvancedEffects();
                    updateAdvancedEffects(16);
                    applyShake();
                    drawParticles();
        
                    Brain.Screen.render();
                    this_thread::sleep_for(16);
                }
    
            triggerShake(8, 400);
    
            // Phase 1: STATS REVEAL WITH PARTICLE BURSTS (500-3000ms)
            const int statFrames = 180;
            int statValues[7] = {0, 0, 0, 0, 0, 0, 0};
            int statTargets[7] = {
                t.online,           // motors online
                t.mTemp[t.hot],     // hottest motor temp
                t.bat,              // battery %
                t.mv,               // voltage
                t.totalDa,          // total current (deci-amps)
                t.warn,             // warnings
                t.runtime           // runtime seconds
            };
            const char* statLabels[7] = {
                "MOTORS ONLINE", "HOTTEST MOTOR", "BATTERY", 
                "VOLTAGE", "CURRENT DRAW", "WARNINGS", "MATCH TIME"
            };
            const char* statSuffix[7] = {"/4", "C", "%", "mV", "A", "", "s"};
            int statRevealFrame[7] = {10, 35, 60, 85, 110, 135, 160};
    
            for (int frame = 0; frame <= statFrames; ++frame) {
                Brain.Screen.setFillColor(cBg);
                Brain.Screen.clearScreen(cBg);
        
                applyShake();
                updateParticles(16);
        
                // Background atmosphere - moving gradient bars
                for (int i = 0; i < 6; ++i) {
                    float wave = sinf((frame + i * 30) * 0.05f) * 0.5f + 0.5f;
                    color bgc = mix8(cPanelRgb, themeAccentRgb, (int)(wave * 40));
                    gRect(0 + shakeX, 28 + i * 35 + shakeY, 480, 30, bgc);
                }
        
                // Animated stat cards
                for (int s = 0; s < 7; ++s) {
                    if (frame < statRevealFrame[s]) continue;
            
                    int revealProg = clampInt((frame - statRevealFrame[s]) * 100 / 25, 0, 100);
                    float ease = 1.0f - powf(1.0f - revealProg / 100.0f, 3);
            
                    // Animate value
                    if (statValues[s] < statTargets[s]) {
                        statValues[s] = (int)(statTargets[s] * ease);
                        if (statValues[s] > statTargets[s]) statValues[s] = statTargets[s];
                    }
            
                    int cardY = 40 + s * 28;
                    int cardH = 24;
            
                    // Card background with glow
                    color cardBg = mix8(cPanelRgb, themeAccentRgb, (int)(ease * 30));
                    gRect(20 + shakeX, cardY + shakeY, 440, cardH, cardBg);
            
                    // Accent line on left
                    int lineH = (int)(cardH * ease);
                    gRect(20 + shakeX, cardY + cardH - lineH + shakeY, 4, lineH, themeAccent);
            
                    // Label
                    gText(34 + shakeX, cardY + 16 + shakeY, cMuted, "%s", statLabels[s]);
            
                    // Value with counter animation
                    color valColor = cText;
                    if (s == 0) valColor = statValues[s] == 4 ? cGood : cDanger;
                    else if (s == 1) valColor = statValues[s] >= 60 ? cDanger : statValues[s] >= 45 ? cWarn : cGood;
                    else if (s == 2) valColor = statValues[s] <= 20 ? cDanger : statValues[s] <= 50 ? cWarn : cGood;
                    else if (s == 5) valColor = statValues[s] > 0 ? cWarn : cGood;
            
                    char valBuf[16];
                    if (s == 3) { // voltage
                                    int v = statValues[s];
                                    valBuf[0] = '0' + v / 10000;
                                    valBuf[1] = '0' + (v / 1000) % 10;
                                    valBuf[2] = '0' + (v / 100) % 10;
                                    valBuf[3] = '0' + (v / 10) % 10;
                                    valBuf[4] = '0' + v % 10;
                                    valBuf[5] = 0;
                                } else if (s == 4) { // current
                                    FMT_AMP(valBuf, statValues[s]);
                                } else if (s == 6) { // time
                                    int m = statValues[s] / 60;
                                    int sec = statValues[s] % 60;
                                    valBuf[0] = '0' + m / 10;
                                    valBuf[1] = '0' + m % 10;
                                    valBuf[2] = ':';
                                    valBuf[3] = '0' + sec / 10;
                                    valBuf[4] = '0' + sec % 10;
                                    valBuf[5] = 0;
                                } else {
                                    FMT_PCT(valBuf, statValues[s]);
                                }
                    if (statSuffix[s][0]) {
                        int len = strlen(valBuf);
                        for (int i = 0; statSuffix[s][i]; ++i) valBuf[len + i] = statSuffix[s][i];
                        valBuf[len + strlen(statSuffix[s])] = 0;
                    }
                    gText(380 + shakeX, cardY + 16 + shakeY, valColor, "%s", valBuf);
            
                    // Particle burst when stat locks in
                    if (revealProg >= 95 && revealProg < 100) {
                        spawnParticles(420, cardY + 12, 8, themeAccent, 2.0f, 5.0f, 2, 4, 800);
                    }
                }
        
                // Draw particles
                drawParticles();
        
                // Top header
                gRect(0 + shakeX, 0 + shakeY, 480, 26, cPanel);
                gLine(0 + shakeX, 26 + shakeY, 480 + shakeX, 26 + shakeY, themeAccent);
                Brain.Screen.setFont(prop20);
                gTextC(240 + shakeX, 18 + shakeY, 12, themeAccent, "POST-MATCH ANALYSIS");
                Brain.Screen.setFont(mono12);
        
                // Bottom brand
                gLine(16 + shakeX, 221 + shakeY, 464 + shakeX, 221 + shakeY, cLine);
                        gTextC(240 + shakeX, 232 + shakeY, 8, cAccent2, "VEXTOP  //  TEAM 5977C  //  THE WARRIORS");
        
                Brain.Screen.render();
                this_thread::sleep_for(16);
            }
    
            // Phase 2: EPIC FINALE - Particle explosion + team logo (3000-5000ms)
            triggerShake(12, 600);
            spawnParticles(240, 120, 60, themeAccent, 3.0f, 8.0f, 3, 6, 1500);
            spawnParticles(240, 120, 40, themeAccent2, 1.0f, 4.0f, 2, 5, 2000);
    
            for (int frame = 0; frame <= 120; ++frame) {
                Brain.Screen.setFillColor(cBg);
                Brain.Screen.clearScreen(cBg);
        
                applyShake();
                updateParticles(16);
        
                float progress = frame / 120.0f;
                float ease = progress * progress * (3.0f - 2.0f * progress); // smoothstep
        
                // Expanding rings of glory
                for (int r = 0; r < 8; ++r) {
                    float ringProg = fmodf(ease * 2.0f + r * 0.125f, 1.0f);
                    int radius = (int)(ringProg * 400);
                    color rc = mix8(themeAccentRgb, themeAccent2Rgb, (int)(ringProg * 256));
                    gRing(240 + shakeX, 120 + shakeY, radius, rc);
                }
        
                // Central logo build-up
                if (frame > 20) {
                    float logoEase = (frame - 20) / 100.0f;
                    logoEase = clampInt(logoEase * 100, 0, 100) / 100.0f;
                    logoEase = 1.0f - powf(1.0f - logoEase, 4);
            
                    // Rotating hexagon background
                    for (int h = 0; h < 6; ++h) {
                        float angle = (h * 60.0f + frame * 0.5f) * 0.0174533f;
                        int hx = 240 + (int)(80 * logoEase * cosf(angle)) + shakeX;
                        int hy = 120 + (int)(80 * logoEase * sinf(angle)) + shakeY;
                        gDot(hx, hy, (int)(8 * logoEase), themeAccent);
                    }
            
                    // Team name
                    Brain.Screen.setFont(prop60);
                    color tc = mix8(cTextRgb, themeAccentRgb, (int)(logoEase * 256));
                    gTextC(240 + shakeX, 110 + shakeY, 30, tc, "5977C");
                    Brain.Screen.setFont(prop20);
                            gTextC(240 + shakeX, 155 + shakeY, 12, themeAccent2, "THE WARRIORS");
                    Brain.Screen.setFont(mono12);
                }
        
                drawParticles();
        
                // Bottom tagline
                if (frame > 60) {
                    float tagEase = (frame - 60) / 60.0f;
                    tagEase = clampInt(tagEase * 100, 0, 100) / 100.0f;
                    gTextC(240 + shakeX, 210 + shakeY, 8, mix8(cMutedRgb, themeAccentRgb, (int)(tagEase * 256)), 
                           dominated ? "DOMINANT VICTORY" : struggled ? "HARD FOUGHT" : "MISSION COMPLETE");
                }
        
                Brain.Screen.render();
                this_thread::sleep_for(16);
            }
    
            // Phase 3: FADE TO IDLE (5000-6000ms)
            for (int frame = 0; frame <= 60; ++frame) {
                Brain.Screen.setFillColor(cBg);
                Brain.Screen.clearScreen(cBg);
        
                updateParticles(16);
                float fade = 1.0f - frame / 60.0f;
        
                // Fade particles
                for (int i = 0; i < kMaxParticles; ++i) {
                    if (!particles[i].active) continue;
                    int sz = (int)(particles[i].size * fade);
                    if (sz > 0) gDot((int)particles[i].x, (int)particles[i].y, sz, particles[i].c);
                }
        
                // Fade logo
                Brain.Screen.setFont(prop60);
                color tc = mix8(cTextRgb, cBgRgb, (int)(fade * 256));
                gTextC(240, 110, 30, tc, "5977C");
                Brain.Screen.setFont(prop20);
                        gTextC(240, 155, 12, mix8(themeAccent2Rgb, cBgRgb, (int)(fade * 256)), "THE WARRIORS");
                Brain.Screen.setFont(mono12);
        
                Brain.Screen.render();
                this_thread::sleep_for(16);
            }
    
            // Final hold
            this_thread::sleep_for(2000);
        }

void initializeDrive() {
    LeftDrive.setStopping(brake);
    RightDrive.setStopping(brake);
    
    // Initialize parallax layers
    parallaxLayers[0] = {0, 0, -0.5f, -0.2f, 0, 1.0f, cAccent, 0.3f}; // Stars
    parallaxLayers[1] = {0, 0, -0.2f, -0.1f, 1, 1.0f, cLine, 0.15f}; // Grid
    parallaxLayers[2] = {0, 0, -1.0f, -0.5f, 2, 1.0f, cMuted, 0.2f}; // Particles
    parallaxLayers[3] = {0, 0, -0.1f, -0.05f, 3, 1.0f, cAccent2, 0.1f}; // Nebula
    parallaxLayers[4] = {0, 0, 0.3f, 0.2f, 0, 1.0f, cGood, 0.1f}; // Foreground stars
    
    // Initialize color LUT
    initColorLUT();
    
    // Initialize achievements
    unlockAchievement("FIRST BOOT", "Started VEXTOP for the first time", cAccent);
        unlockAchievement("WARRIOR SPIRIT", "Team 5977C - The Warriors", cGold);
        unlockAchievement("CYBERPUNK", "Neon dreams in a digital world", cAccent2);
        unlockAchievement("VOID WALKER", "Stared into the abyss", cDanger);
        unlockAchievement("GLITCH MASTER", "Corrupted the system", cWarn);
        unlockAchievement("NEON DREAMER", "Chased electric sheep", cCyan);
    
        bootAnimation();
}

int main() {
    competition::bStopTasksBetweenModes = true;
    srand((unsigned int)Brain.Timer.time(msec)); // Seed RNG for particle effects
    initializeDrive();

    bool wasEnabled = Competition.isEnabled();
    bool outroVisible = false;
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
        if (enabled) outroVisible = false;
        if (!outroVisible) {
            drawUI();  // drawUI() now handles its own adaptive rate limiting
        }
        this_thread::sleep_for(5);
    }
}
