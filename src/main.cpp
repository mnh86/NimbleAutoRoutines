// Use "ESP32 Dev Module" as board
#include <Arduino.h> 
#include <BfButton.h>
#include "nimbleCon.h"

BfButton btn(BfButton::STANDALONE_DIGITAL, ENC_BUTT, true, LOW);

void pressHandler (BfButton *btn, BfButton::press_pattern_t pattern) {
  switch (pattern) {
    case BfButton::SINGLE_PRESS:
      Serial.printf("Button %d pressed.\n", btn->getID());
      Serial.printf("Pendant dial level: %d\n", encoder.getCount());
      break;
    case BfButton::DOUBLE_PRESS:
      Serial.printf("Button %d double pressed.\n", btn->getID());
      break;
    case BfButton::LONG_PRESS:
      Serial.printf("Button %d long pressed.\n", btn->getID());
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
  while (!Serial);
  delay(3000);
  Serial.println("\nNimbleEdgeRoutine starting...");

  btn.onPress(pressHandler)
     .onDoublePress(pressHandler) // default timeout
     .onPressFor(pressHandler, 2000); // custom timeout for 2 seconds
}

/**
 * Put your main code here, to run repeatedly. 
 * Do NOT use delays. nimbleCon uses non-blocking mode.
 */
void loop()
{
  // Modify values to be sent to the actuator.

  // Check actuator and pendant serial ports for complete packets and update structs.
  if (readFromPend())
  { // Read values from pendant. If the function returns true, the values were updated so update the pass-through values.
    // DEMO: Pass through data from pendant to actuator
    actuator.positionCommand = pendant.positionCommand; // this is the main property we will be adjusting
    actuator.forceCommand = pendant.forceCommand;
    actuator.airIn = pendant.airIn;
    actuator.airOut = pendant.airOut;
    actuator.activated = pendant.activated;
  }

  readFromAct(); // Read values from actuator. If the function returns true, the values were updated. Otherwise there was nothing new.

  // This DEMO code pauses the actuator (in a very crude way) when the encoder button is pressed
  // (it will jump to whatever position the pendant is commanding at the moment the button is released)
  btn.read();
  if (digitalRead(ENC_BUTT))
  {                                // Encoder button reads low when pressed.
    driveLEDs(encoder.getCount()); // Show LEDs as demo
  }
  else
  {
    driveLEDs(0);              // Blank LEDs when button is pressed
    actuator.forceCommand = 0; // Set force command to 0 when button is pressed.
  }

  // Send packet of values to the actuator when time is ready
  if (checkTimer())
    sendToAct();

  pendant.present ? ledcWrite(PEND_LED, 50) : ledcWrite(PEND_LED, 0); // Display pendant connection status on LED.
  actuator.present ? ledcWrite(ACT_LED, 50) : ledcWrite(ACT_LED, 0);  // Display actuator connection status on LED.
  
  // ledcWrite(PEND_LED, 50);
  // ledcWrite(ACT_LED, 50);
  // ledcWrite(BT_LED, 50);
  // ledcWrite(WIFI_LED, 50);
}
