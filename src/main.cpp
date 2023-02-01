// Use "ESP32 Dev Module" as board
#include <Arduino.h> 
#include <BfButton.h>
#include <loopTimer.h>
#include <BufferedOutput.h>
#include <millisDelay.h>
#include "nimbleCon.h"

const int idle_return_speed = 3;

uint8_t ns_mode = 0;      // 0 - idle, 1 - wave
uint16_t ns_stroke = 500; // maximum magnitude of stroke (primary wave magnitude will be ns_stroke - ns_texture) [0-1000]
float ns_speed = 1.0;     // speed in hz of the primary wave
uint16_t ns_texture = 0;  // magnitude of the secondary wave [0-1000]
float ns_nature = 10;     // speed in hz of the secondary wave

int frame_position = 0;
int frame_force = 1023;

createBufferedOutput(bufferedOut, 80, DROP_UNTIL_EMPTY);

millisDelay printDelay;
millisDelay ledCycleDelay;

BfButton btn(BfButton::STANDALONE_DIGITAL, ENC_BUTT, true, LOW);

void pressHandler (BfButton *btn, BfButton::press_pattern_t pattern) {
  switch (pattern) {
    case BfButton::SINGLE_PRESS:
      bufferedOut.printf("Btn %d pressed\n", btn->getID());
      bufferedOut.printf("Dial level: %d\n", encoder.getCount());
      bufferedOut.printf("Frame pos: %d\n", frame_position);
      ns_mode = !ns_mode;
      break;
    case BfButton::DOUBLE_PRESS:
      bufferedOut.printf("Btn %d double pressed\n", btn->getID());
      break;
    case BfButton::LONG_PRESS:
      bufferedOut.printf("Btn %d long pressed\n", btn->getID());
      break;
  }
}

/**
 * Put your setup code here, to run once
 */
void setup()
{ 
  initNimbleSDK();

  Serial.setDebugOutput(true);
  delay(3000);
  Serial.println("\nNimbleEdgeRoutine starting...");
  bufferedOut.connect(Serial);  // connect buffered stream to Serial

  btn.onPress(pressHandler)
     .onDoublePress(pressHandler) // default timeout
     .onPressFor(pressHandler, 2000); // custom timeout for 2 seconds

  actuator.forceCommand = frame_force;
  actuator.airIn = false;
  actuator.airOut = false;
  actuator.activated = true;

  printDelay.start(200); // start the printDelay in ms
  ledCycleDelay.start(30);
}

void print_position() {
  if (!printDelay.justFinished()) return;

  printDelay.repeat(); // start delay again without drift
  bufferedOut.printf("Pos %5d\n", frame_position);
}

void cycle_leds() {
  if (!ledCycleDelay.justFinished()) return;
  ledCycleDelay.repeat();
  
  byte ledState1 = 0;
  byte ledState2 = 0;
  byte ledScale = map(abs(frame_position), 1, MAX_ACTUATOR_POS, 1, MAX_LED_DUTY);

  if (ns_mode) {
    if (frame_position > 0) {
      ledState1 = ledScale;
      ledState2 = 0;
    } else {
      ledState1 = 0;
      ledState2 = ledScale;
    }
  } 

  ledcWrite(ENC_LED_N, ledState1);
  ledcWrite(ENC_LED_NE, ledState2);
  ledcWrite(ENC_LED_E, ledState1);
  ledcWrite(ENC_LED_SE, ledState2);
  ledcWrite(ENC_LED_S, ledState1);
  ledcWrite(ENC_LED_SW, ledState2);
  ledcWrite(ENC_LED_W, ledState1);
  ledcWrite(ENC_LED_NW, ledState2);
}

void stepModeIdle() {
  // gradually return the position to zero
  if ((idle_return_speed * -1) < frame_position && frame_position < 0) {
    frame_position = 0;
  }
  else if (0 < frame_position && frame_position < idle_return_speed) {
    frame_position = 0;
  }
  else if (frame_position > 0) {
    frame_position = frame_position - idle_return_speed;
  }
  else if (frame_position < 0) {  
    frame_position = frame_position + idle_return_speed;
  }
}

int calcWavePosition(int speed, int max) {
  int speed_millis = 1000 / speed;
  int mod_millis = millis() % speed_millis;
  float temp_pos = float(mod_millis) / speed_millis;
  int wave_deg = temp_pos * 360;
  return (sin(radians(wave_deg)) * max);
}

void stepModeWave() {  
  int wave1_pos = calcWavePosition(ns_speed, ns_stroke - ns_texture);
  int wave2_pos = calcWavePosition(ns_nature, ns_texture);
  frame_position = wave1_pos + wave2_pos;
}

/**
 * Put your main code here, to run repeatedly. 
 * Do NOT use delays. nimbleCon uses non-blocking mode.
 */
void loop()
{
  bufferedOut.nextByteOut(); // call at least once per loop to release chars
  loopTimer.check(bufferedOut);

  // Pause the actuator when the encoder button is pressed.
  btn.read();

  // Modify values to be sent to the actuator.
  switch (ns_mode) {
    case 1:
      stepModeWave();
      //driveLEDs(encoder.getCount());
      break;
    default:
      stepModeIdle();
      //driveLEDs(0);
      break;
  }

  // Check actuator and pendant serial ports for complete packets and update structs.
  if (readFromPend())
  { // Read values from pendant. If the function returns true, the values were updated so update the pass-through values.
    // DEMO: Pass through data from pendant to actuator
    // actuator.positionCommand = pendant.positionCommand;
    actuator.forceCommand = pendant.forceCommand;
    actuator.airIn = pendant.airIn;
    actuator.airOut = pendant.airOut;
    actuator.activated = pendant.activated; // TODO: may not be used, verify
  }

  readFromAct(); // Read values from actuator. If the function returns true, the values were updated. Otherwise there was nothing new.

  // Send packet of values to the actuator when time is ready
  if (checkTimer())
  {
    actuator.positionCommand = frame_position;
    actuator.forceCommand = frame_force;
    sendToAct();
  }

  pendant.present ? ledcWrite(PEND_LED, 50) : ledcWrite(PEND_LED, 0); // Display pendant connection status on LED.
  actuator.present ? ledcWrite(ACT_LED, 50) : ledcWrite(ACT_LED, 0);  // Display actuator connection status on LED.
  
  // ledcWrite(PEND_LED, 50);
  // ledcWrite(ACT_LED, 50);
  // ledcWrite(BT_LED, 50);
  // ledcWrite(WIFI_LED, 50);

  print_position();
  cycle_leds();
}
