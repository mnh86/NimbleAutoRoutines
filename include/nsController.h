#include <Arduino.h>
#include "nimbleConModule.h"

#define RUN_MODE_IDLE 0
#define RUN_MODE_EDGING 1
#define RUN_MODE_RANDOM 2

#define EDIT_MODE_SPEED 0
#define EDIT_MODE_STROKE 1
#define EDIT_MODE_TEXTURE 2

const int idleReturnSpeed = 3;

uint8_t runMode = RUN_MODE_IDLE;  // 0 - idle, 1 - edging program, 2 - random
uint16_t nsStroke = 100; // maximum magnitude of stroke (primary wave magnitude will be nsStroke - nsTexture) [0-1000]
float nsSpeed = 0.1;     // speed in hz of the primary wave
uint16_t nsTexture = 0;  // magnitude of the secondary wave [0-1000] (vibration intensity)
float nsNature = 10;     // speed in hz of the secondary wave (vibration speed)

int framePosition = 0;
int frameForce = IDLE_FORCE;

uint8_t editMode = EDIT_MODE_SPEED; // 0 - speed, 1 - stroke, 2 - texture
int64_t encoderRef;

// Initialization fuction
void initNimbleStrokerController()
{
    initNimbleConModule();
    encoderRef = encoder.getCount();
}

void stepModeIdle()
{
    // gradually return the position to zero
    if (framePosition > 0)
    {
        framePosition = framePosition - idleReturnSpeed;
    }
    else if (framePosition < 0)
    {
        framePosition = framePosition + idleReturnSpeed;
    }
    else if (((idleReturnSpeed * -1) < framePosition && framePosition < 0) ||
             (0 < framePosition && framePosition < idleReturnSpeed))
    {
        framePosition = 0;
        frameForce = IDLE_FORCE;
    }
}

int calcWavePosition(float speed, int max)
{
    if (speed == 0) return 0;
    int speedMillis = 1000 / speed;
    int modMillis = millis() % speedMillis;
    float temp_pos = float(modMillis) / speedMillis;
    int wave_deg = temp_pos * 360;
    return (sin(radians(wave_deg)) * max);
}

void stepModeEdging()
{
    int wave1_pos = calcWavePosition(nsSpeed, nsStroke - nsTexture);
    int wave2_pos = calcWavePosition(nsNature, nsTexture);
    framePosition = wave1_pos + wave2_pos;
    frameForce = MAX_FORCE;
}

void stepModeRandom()
{
    framePosition = 0; // TODO
    frameForce = MAX_FORCE;
}

void ledEditModeDisplay(bool isOn) {
    byte wifiVal = 0;
    byte btVal = 0;
    if (isOn) {
        switch (editMode)
        {
        case EDIT_MODE_STROKE:
            btVal = 50;
            break;
        case EDIT_MODE_TEXTURE:
            wifiVal = 50;
            break;
        }
    }
    ledcWrite(BT_LED, btVal);
    ledcWrite(WIFI_LED, wifiVal);
}

int32_t mapEditValues(int32_t val, int32_t min, int32_t max, int8_t delta) {
    int16_t level = map(val, min, max, 0, 100) + delta;
    if (level < 0) level = 0;
    if (level > 100) level = 100;
    return map(level, 0, 100, min, max);
}


#define NS_STROKE_MIN 0
#define NS_STROKE_MAX 750

void setStrokeDelta(int8_t delta) {
    nsStroke = mapEditValues(nsStroke, NS_STROKE_MIN, NS_STROKE_MAX, delta);
}

#define NS_TEXTURE_MIN 0
#define NS_TEXTURE_MAX 50

void setTextureDelta(int8_t delta) {
    nsTexture = mapEditValues(nsTexture, NS_TEXTURE_MIN, NS_TEXTURE_MAX, delta);
}

#define NS_SPEED_MIN 0
#define NS_SPEED_MAX 300

void setSpeedDelta(int8_t delta) {
    int16_t tmpVal = mapEditValues((int)(nsSpeed * 100), NS_SPEED_MIN, NS_SPEED_MAX, delta);
    nsSpeed = (tmpVal > 0) ? tmpVal / 100.0 : 0;
}

#define EDIT_DELTA_AMT 2

void editModeStep() {
    int64_t tmpRef = encoder.getCount();
    int8_t delta = (tmpRef - encoderRef) * EDIT_DELTA_AMT;
    encoderRef = tmpRef;

    if (delta == 0) return;

    switch (editMode)
    {
    case EDIT_MODE_STROKE:
        setStrokeDelta(delta);
        break;
    case EDIT_MODE_TEXTURE:
        setTextureDelta(delta);
        break;
    default:
        setSpeedDelta(delta);
        break;
    }
}

void nsControllerLoop()
{
    // Modify values to be sent to the actuator.
    editModeStep();

    switch (runMode)
    {
    case RUN_MODE_EDGING:
        stepModeEdging();
        break;
    case RUN_MODE_RANDOM:
        stepModeRandom();
        break;
    default:
        stepModeIdle();
        break;
    }

    // Check actuator and pendant serial ports for complete packets and update structs.
    if (readFromPend())
    { // Read values from pendant. If the function returns true, the values were updated so update the pass-through values.
        // DEMO: Pass through data from pendant to actuator
        // actuator.positionCommand = pendant.positionCommand;
        // actuator.forceCommand = pendant.forceCommand;
        actuator.airIn = pendant.airIn;
        actuator.airOut = pendant.airOut;
        actuator.activated = pendant.activated; // TODO: may not be used, verify
    }

    // Send packet of values to the actuator when time is ready
    if (checkTimer())
    {
        actuator.positionCommand = framePosition;
        actuator.forceCommand = frameForce;
        sendToAct();
    }

    if (readFromAct()) // Read current state from actuator.
    { // If the function returns true, the values were updated.
        if (actuator.tempLimiting) {
            runMode = RUN_MODE_IDLE;
        }
    }

    pendant.present ? ledcWrite(PEND_LED, 50) : ledcWrite(PEND_LED, 0); // Display pendant connection status on LED.
    actuator.present ? ledcWrite(ACT_LED, 50) : ledcWrite(ACT_LED, 0);  // Display actuator connection status on LED.
}