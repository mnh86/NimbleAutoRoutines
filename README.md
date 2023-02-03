# NimbleEdgeRoutine

> **NOTE:** These are development experiments - USE AT YOUR OWN RISK. There is NO Warranty or Support for this program (see [LICENSE](./LICENSE)). The author(s) are NOT liable for damages that may occur to your device(s).

Basic automated routines for the NimbleConModule and NimbleStroker.
- **Constant**: Default Pendant behavior. Stroke speed, length, and texture stay constant.
- **Edging**: Stroke speed and length ramp up and slow down over time. Texture ramps up and down.
- **Random**: Varies the stroke speed, length, and texture on random patterns every 2 minutes.

## Usage

1. Set up [VSCode with PlatformIO](https://randomnerdtutorials.com/vs-code-platformio-ide-esp32-esp8266-arduino/)
2. Clone this repo and open the project in VSCode
3. Build and upload this program into the NimbleConModule
4. Attach the NimbleConModule to the actuator (Label A) and Control Pendant (Label P)
5. Double click the Encoder Dial to start/skip to the next routine:
   1. `Constant`: Default Pendant behavior. Stroke speed, length, and texture stay constant.
   2. `Edging`: Stroke speed and length ramp up and slow down over time. Texture ramps up and down.
   3. `Random`: Varies the stroke speed, length, and texture on random patterns every 2 minutes.
6. Single click the Encoder Dial to switch between setting modes when turning the dial:
   1. `Mode 1: Speed`: Changes the length of the waveform (duration of the sine wave) (0 to 3 hz)
   2. `Mode 2: Stroke`: Changes the amplitude of the waveform (0 to 750)
   3. `Mode 3: Texture`: Changes the intensity of the vibration
7. Use the dial to adjust the active setting mode
8. Long press the Encoder Dial (2 seconds) to stop the routine

## Attributions

- Used example code (heavily modified) from: <https://github.com/tyrm/nimblestroker/>
- Used and modified NimbleConSDK from: <https://github.com/ExploratoryDevices/NimbleConModule>
