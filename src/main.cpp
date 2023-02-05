// Use "ESP32 Dev Module" as board
#include <Arduino.h>
#include <BfButton.h>
#include <loopTimer.h>
#include <BufferedOutput.h>
#include <millisDelay.h>
#include "nsController.h"

createBufferedOutput(bufferedOut, 80, DROP_UNTIL_EMPTY);

millisDelay printDelay;
millisDelay ledCycleDelay;

void printState() {
    bufferedOut.printf("------------------\n");
    bufferedOut.printf(" Run Mode: %s\n", getRunModeName());
    if (runMode != RUN_MODE_IDLE) {
    bufferedOut.printf("      Pos: %5d\n", lastFramePos);
    bufferedOut.printf("  MaxPosΔ: %5d\n", maxPositionDelta);
    bufferedOut.printf("    Force: %5d\n", frameForce);
    bufferedOut.printf("   AirOut: %5d\n", actuator.airOut);
    bufferedOut.printf("TempLimit: %s", actuator.tempLimiting ? "true" : "false");
    maxPositionDelta = 0;
    }
    bufferedOut.printf("Edit Mode: %s\n", getEditModeName());
    bufferedOut.printf("    Speed: %0.3f\n", nsSpeed);
    bufferedOut.printf("   Stroke: %5d\n", nsStroke);
    bufferedOut.printf("  Texture: %5d\n", nsTexture);
    //bufferedOut.printf("Encoder %5d\n", encoder.getCount());
}

BfButton btn(BfButton::STANDALONE_DIGITAL, ENC_BUTT, true, LOW);

void pressHandler(BfButton *btn, BfButton::press_pattern_t pattern)
{
    switch (pattern)
    {
    case BfButton::SINGLE_PRESS: // Cycle through edit modes
        setEditMode(editMode + 1);
        bufferedOut.printf("Btn %d pressed\n", btn->getID());
        break;
    case BfButton::DOUBLE_PRESS: // Start and switch run modes
        setRunMode(runMode + 1);
        bufferedOut.printf("Btn %d double pressed\n", btn->getID());
        break;
    case BfButton::LONG_PRESS: // Stop
        setRunMode(RUN_MODE_IDLE);
        bufferedOut.printf("Btn %d long pressed\n", btn->getID());
        break;
    }
}

/**
 * Put your setup code here, to run once
 */
void setup()
{
    initNimbleStrokerController();

    Serial.setDebugOutput(true);
    delay(3000);
    Serial.println("\nNimbleEdgeRoutine starting...");
    bufferedOut.connect(Serial); // connect buffered stream to Serial

    btn.onPress(pressHandler)
        .onDoublePress(pressHandler)     // default timeout
        .onPressFor(pressHandler, 2000); // custom timeout for 2 seconds

    printDelay.start(2000); // start the printDelay in ms
    ledCycleDelay.start(30);
}

void logState()
{
    if (!printDelay.justFinished())
        return;
    printDelay.repeat(); // start delay again without drift

    printState();
}

void driveLEDs()
{
    if (!ledCycleDelay.justFinished())
        return;
    ledCycleDelay.repeat(); // start delay again without drift

    ledPositionPulse(lastFramePos, (runMode != RUN_MODE_IDLE));
    ledEditModeDisplay(true);
}

/**
 * Put your main code here, to run repeatedly.
 * Do NOT use delays. nimbleCon uses non-blocking mode.
 */
void loop()
{
    bufferedOut.nextByteOut(); // call at least once per loop to release chars
    loopTimer.check(bufferedOut);

    btn.read();
    nsControllerLoop();
    driveLEDs();
    logState();
}
