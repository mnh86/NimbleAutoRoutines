#include <Arduino.h>
#include "nimbleConModule.h"

#define RUN_MODE_IDLE 0
#define RUN_MODE_CONSTANT 1
#define RUN_MODE_EDGING 2
#define RUN_MODE_RANDOM 3

uint8_t runMode = RUN_MODE_IDLE;
unsigned long int runModeStartTime;
int maxPositionDelta = 0; // largest jumps in position (for debugging)
bool easeInToChange = true;

unsigned long int airReleaseTimer;
#define AIR_RELEASE_TIMEOUT  300000 // 5 min
#define AIR_RELEASE_DURATION 1500   // 1.5 sec

unsigned long int randChangeTimer;
#define RAND_MODE_TIMEOUT 120000  // 2 min
float randVibRampFreq = 80.f;     // 60-120
float randStrokeModFreq = 10.f;   //  5-15
float randStrokeModAmp = -8.f;    //  6-10
float randStrokeDepthFreq = 40.f; // 30-50

double randomDouble(double minf, double maxf)
{
    return minf + random(1UL << 31) * (maxf - minf) / (1UL << 31);  // use 1ULL<<63 for max double values)
}

void setRandRunModeValues()
{
    randChangeTimer = millis() + RAND_MODE_TIMEOUT;
    randVibRampFreq = randomDouble(20.f, 120.f);
    randStrokeModFreq = randomDouble(5.f, 20.f);
    randStrokeModAmp = randomDouble(6.f, 10.f);
    randStrokeDepthFreq = randomDouble(30.f, 120.f);
}

char * getRunModeName()
{
    if (runMode == RUN_MODE_IDLE) return (char *)"Idle";
    if (runMode == RUN_MODE_CONSTANT) return (char *)"Constant";
    if (runMode == RUN_MODE_EDGING) return (char *)"Edging";
    if (runMode == RUN_MODE_RANDOM) return (char *)"Random";
    return (char *)"Unknown";
}

void setRunMode(byte rm)
{
    if (rm > RUN_MODE_RANDOM) rm = RUN_MODE_CONSTANT;
    runModeStartTime = millis();
    if (rm != RUN_MODE_IDLE && runMode == RUN_MODE_IDLE) {
        // only reset air release timer when transitioning from IDLE
        actuator.airOut = false;
        airReleaseTimer = runModeStartTime + AIR_RELEASE_TIMEOUT;
    }
    runMode = rm;
    maxPositionDelta = 0;
    easeInToChange = true;
    if (rm == RUN_MODE_RANDOM) setRandRunModeValues();
}

void autoAirRelease()
{
    if (pendant.present || runMode == RUN_MODE_IDLE) return;
    unsigned long int t = millis();
    if (t < airReleaseTimer) return; // Timer not ready yet

    actuator.airOut = true;
    if (t > airReleaseTimer + AIR_RELEASE_DURATION) {
        actuator.airOut = false;
        airReleaseTimer = t + AIR_RELEASE_TIMEOUT; // schedule next timer
    }
}

#define EDIT_MODE_SPEED 0
#define EDIT_MODE_STROKE 1
#define EDIT_MODE_TEXTURE 2

uint8_t editMode = EDIT_MODE_SPEED; // 0 - speed, 1 - stroke, 2 - texture
int64_t lastEncoderPos;

char * getEditModeName()
{
    if (editMode == EDIT_MODE_SPEED) return (char *)"Speed";
    if (editMode == EDIT_MODE_STROKE) return (char *)"Stroke";
    if (editMode == EDIT_MODE_TEXTURE) return (char *)"Texture";
    return (char *)"Unknown";
}

void setEditMode(byte em)
{
    if (em > EDIT_MODE_TEXTURE) em = EDIT_MODE_SPEED;
    editMode = em;
}

const int idleReturnSpeed = 3;

uint16_t nsStroke = 100; // maximum magnitude of stroke (primary wave magnitude will be nsStroke - nsTexture) [0-1000]
float nsSpeed = 0.1;     // speed in hz of the primary wave
uint16_t nsTexture = 0;  // magnitude of the secondary wave [0-1000] (vibration intensity)
float nsNature = 20;     // speed in hz of the secondary wave (vibration speed)

int16_t framePosition = 0; // -1000 to 1000
int16_t lastFramePos = 0;  // -1000 to 1000
int16_t frameForce = IDLE_FORCE; // (0 to 1023)

// Initialization fuction
void initNimbleStrokerController()
{
    initNimbleConModule();
    lastEncoderPos = encoder.getCount();
}

int16_t easePositionToTarget(int16_t from, int16_t to = 0) {
    if ((to + (idleReturnSpeed * -1)) <= from && from <= (to + idleReturnSpeed)) {
        return to;
    } else if (to < from) {
        return from - idleReturnSpeed;
    } else { // (from < to)
        return from + idleReturnSpeed;
    }
}

void runModeIdleStep()
{
    // gradually return the position to zero
    framePosition = easePositionToTarget(framePosition);
    if (framePosition == 0) {
        maxPositionDelta = 0;
        frameForce = IDLE_FORCE;
    }
}

float calcSinWavePosition(unsigned long t, float speed, float max, bool useCos = false)
{
    if (speed == 0) return 0;
    int speedMillis = 1000 / speed;
    int modMillis = t % speedMillis;
    float tempPos = float(modMillis) / speedMillis;
    int waveDeg = tempPos * 360;
    return (useCos) ? (sin(radians(waveDeg)) * max)
                    : (cos(radians(waveDeg)) * max);
}

float calcCosWavePosition(unsigned long t, float speed, float max)
{
    return calcSinWavePosition(t, speed, max, true);
}

void runModeConstantStep(unsigned long t)
{
    float strokeWavePos = calcSinWavePosition(t, nsSpeed, nsStroke - nsTexture);
    float vibrationWavePos = calcSinWavePosition(t, nsNature, nsTexture);
    framePosition = round(strokeWavePos + vibrationWavePos);
    frameForce = MAX_FORCE;
}

// Stroke modulation ramps the speed of the strokes
// (ie. ramps up to back down over 10 seconds)
double strokeModulationWave(unsigned long t, float strokeModFreq = 10.f, float strokeModAmp = -8.f)
{
    // Use Frequency Modulation (FM) for alternating the speed of the strokes:
    //     x(t) = cA * sin(cw * t + cp + (mA * sin(mw * t + mp))
    //
    //   c = carrier
    //   m = modulator
    //
    // Oscillator:
    //     x(t) = A * sin((w * t) + p)
    //
    //           A = peak amplitude (nonnegative)
    //           w = radian frequency (rad/sec)
    //             = 2 * PI * frequency(hz)
    //           t = time (sec)
    //           p = initial phase (radians)
    // (w * t) + p = instantaneous phase (radians)
    //
    float tMillis = t / 1000.f;
    float strokeCarPhase = (2.f * PI * nsSpeed) * tMillis;
    float strokeModOsc = strokeModAmp * sin((2.f * PI * (nsSpeed/strokeModFreq) * tMillis) + 0.f);
    return (1.f * sin(strokeCarPhase + strokeModOsc));
}

// Vibration ramps up and down over time
// (ie. ramps up to back down over 40 seconds)
float vibrationRampWave(unsigned long t, float vibRampFreq = 80.f)
{
    float vibRampWave = calcCosWavePosition(t, nsSpeed/vibRampFreq, 1);
    float vibBaseWave = calcSinWavePosition(t, nsNature, nsTexture);
    return (vibBaseWave * vibRampWave);
}

void runModeEdgingStep(unsigned long t)
{
    // Stroke depth wave ramps the amplitude of the strokes up and down
    // (ie. ramps up to long strokes and back down to short strokes over 20 seconds)
    static float strokeDepthFreq = 40.f;
    float strokeDepthWave = calcSinWavePosition(t, nsSpeed/strokeDepthFreq, nsStroke - nsTexture);
    framePosition = (strokeDepthWave * strokeModulationWave(t)) + vibrationRampWave(t);
    frameForce = MAX_FORCE;
}

void runModeRandomStep(unsigned long t)
{
    static float strokeDepthFreq = 40.f;
    float strokeDepthWave = calcSinWavePosition(t, nsSpeed/randStrokeDepthFreq, nsStroke - nsTexture);
    framePosition = (strokeDepthWave * strokeModulationWave(t, randStrokeModFreq, randStrokeModAmp)) + vibrationRampWave(t, randVibRampFreq);
    frameForce = MAX_FORCE;

    // Re-randomize whenever timer is ready
    // and when position is near midpoint (to ease transition somewhat)
    if (framePosition >= -5 && framePosition <= 5 && millis() > randChangeTimer) {
        setRunMode(RUN_MODE_RANDOM);
    }
}

void runModeStep()
{
    unsigned long t = millis() - runModeStartTime;
    // TODO: Consider a better implementation to transition back to idle
    //       before switching to a new run mode to avoid
    //       large jumping differences in position.
    switch (runMode)
    {
        case RUN_MODE_CONSTANT: runModeConstantStep(t); break;
        case RUN_MODE_EDGING: runModeEdgingStep(t); break;
        case RUN_MODE_RANDOM: runModeRandomStep(t); break;
        default: runModeIdleStep(); break;
    }
}

void ledEditModeDisplay(bool isOn)
{
    byte wifiVal = 0;
    byte btVal = 0;
    if (isOn) {
        switch (editMode)
        {
            case EDIT_MODE_STROKE: btVal = 50; break;
            case EDIT_MODE_TEXTURE: wifiVal = 50; break;
        }
    }
    ledcWrite(BT_LED, btVal);
    ledcWrite(WIFI_LED, wifiVal);
}

int32_t mapEditValues(int32_t val, int32_t min, int32_t max, int8_t delta)
{
    int16_t level = map(val, min, max, 0, 100) + delta;
    if (level < 0) level = 0;
    if (level > 100) level = 100;
    return map(level, 0, 100, min, max);
}

#define NS_STROKE_MIN 0
#define NS_STROKE_MAX 750

void setStrokeDelta(int8_t delta)
{
    nsStroke = mapEditValues(nsStroke, NS_STROKE_MIN, NS_STROKE_MAX, delta);
}

#define NS_TEXTURE_MIN 0
#define NS_TEXTURE_MAX 25

void setTextureDelta(int8_t delta)
{
    nsTexture = mapEditValues(nsTexture, NS_TEXTURE_MIN, NS_TEXTURE_MAX, delta);
}

#define NS_SPEED_MIN 0
#define NS_SPEED_MAX 300

void setSpeedDelta(int8_t delta)
{
    float prevSpeed = nsSpeed;
    int16_t tmpVal = mapEditValues((int)(nsSpeed * 100), NS_SPEED_MIN, NS_SPEED_MAX, delta);
    nsSpeed = (tmpVal > 0) ? tmpVal / 100.0 : 0;

    if (runMode != RUN_MODE_IDLE && nsSpeed != prevSpeed) {
        easeInToChange = true; // speed changes cause big jumps in position
        runModeStartTime = millis(); // reset start time to reset wave calculations
    }
}

#define EDIT_DELTA_AMT 4

void editModeStep()
{
    int64_t tmpRef = encoder.getCount();
    int8_t delta = (tmpRef - lastEncoderPos) * EDIT_DELTA_AMT;
    lastEncoderPos = tmpRef;

    if (delta == 0) return;
    maxPositionDelta = 0;
    switch (editMode)
    {
        case EDIT_MODE_SPEED: setSpeedDelta(delta); break;
        case EDIT_MODE_STROKE: setStrokeDelta(delta); break;
        case EDIT_MODE_TEXTURE: setTextureDelta(delta); break;
    }
}

#define MAX_POSITION_DELTA 50
/**
 * Basic protection against large position changes
 */
int16_t clampPositionDelta()
{
    if (easeInToChange) {
        int16_t easePos = easePositionToTarget(lastFramePos, framePosition);
        if (easePos == framePosition) {
            easeInToChange = false;
        }
        return easePos;
    } else {
        int16_t delta = framePosition - lastFramePos;
        maxPositionDelta = max(abs(delta), maxPositionDelta);
        if (delta >= 0)
            return (delta > MAX_POSITION_DELTA) ? lastFramePos + MAX_POSITION_DELTA : framePosition;
        else
            return (delta < -MAX_POSITION_DELTA) ? lastFramePos - MAX_POSITION_DELTA : framePosition;
    }
}

void nsControllerLoop()
{
    // Modify values to be sent to the actuator.
    editModeStep();
    runModeStep();
    autoAirRelease();

    // Check actuator and pendant serial ports for complete packets and update structs.
    if (readFromPend())
    { // Read values from pendant. If the function returns true, the values were updated so update the pass-through values.
        // Pass through data from pendant to actuator
        // actuator.positionCommand = pendant.positionCommand; // We control this
        // actuator.forceCommand = pendant.forceCommand;       // We control this
        actuator.airIn = pendant.airIn;
        actuator.airOut = pendant.airOut;
        actuator.activated = pendant.activated; // TODO: may not be used, verify
    }

    // Send packet of values to the actuator when time is ready
    if (checkTimer())
    {
        lastFramePos = clampPositionDelta();
        actuator.positionCommand = lastFramePos;
        actuator.forceCommand = frameForce;
        sendToAct();
    }

    if (readFromAct()) // Read current state from actuator.
    { // If the function returns true, the values were updated.
        // if (actuator.tempLimiting) {
        //     setRunMode(RUN_MODE_IDLE);
        // }
    }

    pendant.present ? ledcWrite(PEND_LED, 50) : ledcWrite(PEND_LED, 0); // Display pendant connection status on LED.
    actuator.present ? ledcWrite(ACT_LED, 50) : ledcWrite(ACT_LED, 0);  // Display actuator connection status on LED.
}