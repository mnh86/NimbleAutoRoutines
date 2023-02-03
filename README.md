# NimbleAutoRoutines

> **NOTE:** These are development experiments - USE AT YOUR OWN RISK. There is NO Warranty or Support for this program (see [LICENSE](./LICENSE)). The author(s) are NOT liable for damages that may occur to your device(s).

Basic automated routines for the [NimbleStroker](https://shop.exploratorydevices.com/) and [NimbleConModule](https://shop.exploratorydevices.com/product/connectivity-module-dev-kit/).

Three run modes are included:
- **Constant**: Default Pendant behavior. Stroke speed, length, and texture stay constant. Useful for dialing in max speed, length, and texture settings.
- **Edging**: Stroke speed and length ramp up and slow down over time. Texture ramps up and down.
- **Random**: Varies the stroke speed, length, and texture on random patterns every 2 minutes.

## Usage

1. Set up [VSCode with PlatformIO](https://randomnerdtutorials.com/vs-code-platformio-ide-esp32-esp8266-arduino/)
2. Clone this repo and open the project in VSCode
3. Build and upload this program into the NimbleConModule
4. Attach the NimbleConModule to the actuator (Label A)
5. (Optional) Attached the Control Pendant (Label P)
   - Only useful for adjusting the Air In and Out functions
   - Other stroke/vibration controls from the Pendant are bypassed
6. Double click the Encoder Dial to start/skip to the next routine:
   1. `Constant`: Default Pendant behavior. Stroke speed, length, and texture stay constant.
   2. `Edging`: Stroke speed and length ramp up and slow down over time. Texture ramps up and down.
   3. `Random`: Varies the stroke speed, length, and texture on random patterns every 2 minutes.
7. Single click the Encoder Dial to switch between active settings mode when turning the dial:
   1. `Mode 1: Speed`: Changes the length of the waveform (duration of the sine wave) (0 to 3 hz) (BT/WIFI LEDs OFF)
   2. `Mode 2: Stroke`: Changes the amplitude of the waveform (0 to 750) (BT LED ON)
   3. `Mode 3: Texture`: Changes the intensity of the vibration (WIFI LED ON)
8. Use the dial to adjust the active setting mode
   - The settings are applied to all routines
9. Long press the Encoder Dial (2 seconds) to stop the routine

## Other Info

- During a running routine, air will automatically be let out every 5 mins for a duration of 1.5 seconds to help reduce air buildup in the tube assembly.
- If a temperature fault is detected during a running routine, it will stop and switch to IDLE mode.
- Some protections have been made to reduce wild swings in position when changing speed or cycling through routines, but it is possible not all scenarios have been caught yet. Take care with the settings to your speed, stroke and texture settings. Too high and it can trip a temperature fault, especially on Random mode.

## Attributions

- Used example code (heavily modified) from: <https://github.com/tyrm/nimblestroker/>
- Used and modified NimbleConSDK from: <https://github.com/ExploratoryDevices/NimbleConModule>
- See also [platformio.ini](./platformio.ini) for other 3rd party OSS libraries used in this project
