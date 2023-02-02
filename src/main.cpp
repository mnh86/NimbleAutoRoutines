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
    if (runMode != RUN_MODE_IDLE)
        bufferedOut.printf("    Pos %5d\n", framePosition);
    bufferedOut.printf("  Speed %5f\n", nsSpeed);
    bufferedOut.printf(" Stroke %5d\n", nsStroke);
    bufferedOut.printf("Texture %5d\n", nsTexture);
    //bufferedOut.printf("Encoder %5d\n", encoder.getCount());
}

BfButton btn(BfButton::STANDALONE_DIGITAL, ENC_BUTT, true, LOW);

void pressHandler(BfButton *btn, BfButton::press_pattern_t pattern)
{
    switch (pattern)
    {
    case BfButton::SINGLE_PRESS: // Cycle through edit modes
        editMode++; if (editMode > EDIT_MODE_TEXTURE) editMode = EDIT_MODE_SPEED;

        bufferedOut.printf("Btn %d pressed\n", btn->getID());
        printState();
        break;
    case BfButton::DOUBLE_PRESS: // Start and switch run modes
        runMode++; if (runMode > RUN_MODE_RANDOM) runMode = RUN_MODE_EDGING;

        bufferedOut.printf("Btn %d double pressed\n", btn->getID());
        break;
    case BfButton::LONG_PRESS: // Stop
        runMode = RUN_MODE_IDLE;
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

    ledPositionPulse(framePosition, (runMode != RUN_MODE_IDLE));
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
