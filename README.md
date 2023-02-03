# NimbleEdgeRoutine

Automated edging routine for the NimbleConModule.

## Usage

1. Build and upload this program into the NimbleConModule
2. Attach the NimbleConModule to the actuator (Label A) and Control Pendant (Label P)
3. Double click the Encoder Dial to start/skip to the next routine:
   1. `Constant mode` (default) - Runs at constant speed, stroke, texture settings
   2. `Edging mode` - Starts slow, speeds up, slow down, repeat
   3. `Random mode` - Varies speed, stroke, and texture
4. Single click the Encoder Dial to switch between setting modes when turning the dial:
   1. `Mode 1: Speed` - Length of the waveform (duration of the sine wave) (0 to 3 hz)
   2. `Mode 2: Stroke` - Max amplitude of the waveform (0 to 750)
   3. `Mode 3: Texture` - Intensity of the vibration
5. Use the dial to adjust the active setting
6. Long press the Encoder Dial (2 seconds) to stop the routine

## Notes

Control Pendant controls:

| Action          | Result                                                                                                 |
|-----------------|--------------------------------------------------------------------------------------------------------|
| on/off          | When off, position is 0. When on, position wave runs at speed/length/vibration                         |
| stroke speed    | Postion fluctuates between 0 and length at speed rate                                                  |
| stroke length   | Position max range setting                                                                             |
| vibration level | Position fluctuates along main wave. A second wave amplitude is applied to the stroke wave             |
| active texture  | A third wave that adjusts the speed of the vibration level, causing fluctuations in the vibration wave |


TODO:

- [ ] Verify: Actuator parameter `bool activated;` might not be used. Position may just lock at 0.
- [ ] Verify: Readout of pendant values: on/off, stroke length min/max, speed low/high, vibration settings.
- [ ] Verify: `long positionFeedback; // (range: -1000 to 1000)` Errata: actuator delivered before January 2023 this signal always reads positive.
- [ ] Read about multitasking: https://www.instructables.com/Simple-Multi-tasking-in-Arduino-on-Any-Board/
- [ ] Implement: [SafeString](https://www.forward.com.au/pfod/ArduinoProgramming/Serial_IO/index.html)
- [ ] Might be useful? https://github.com/ben-jacobson/non_blocking_timers
- [ ] Research dual core multitasking: https://randomnerdtutorials.com/esp32-dual-core-arduino-ide/
- [ ] Wifi opportunities? https://github.com/khoih-prog/ESPAsync_WiFiManager


Comments:

- The code at this point does not have much care taken to ensure consistent timing. The structures are there but the timing of the calls still needs to be massaged. I suspect it will make sense to have the read/send functions running on the second core with all the "app" stuff running on the first. [Source](https://github.com/ExploratoryDevices/NimbleConModule/issues/2#issuecomment-1368532392)
- The included pendant rails the force command. It does not modify it at any time. Force control is offered for advanced/custom applications but is not necessary for the basic function of the actuator. Users will probably want to leave force maxed out (value `1023`).

References:
- <https://github.com/tyrm/nimblestroker/>
- <https://github.com/ExploratoryDevices/NimbleConModule>