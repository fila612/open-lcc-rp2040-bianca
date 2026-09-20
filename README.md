# OpenLCC RP2040 Firmware for Lelit Bianca

This is an evolution of [magnusnordlander/smart-lcc](https://github.com/magnusnordlander/smart-lcc), which in turn is based on the protocol dissection I did in [magnusnordlander/lelit-bianca-protocol](https://github.com/magnusnordlander/lelit-bianca-protocol).

## Compatibility

This firmware is compatible with Open LCC Board R1A through R2B. Furthermore, it's *known* to be compatible with the Bianca V2, but it's strongly suspected that it is compatible with Bianca V1, and Bianca V3 (with exception for the new Power LED). If you have a Bianca V3 and is interested in installing this project, let me (@magnusnordlander) know and we can work together on getting it fully compatible.

## Disclaimer

Considering this plugs in to an expensive machine it bears to mention: Anything you do with this, you do at your own risk. Components have been fried already during the course of this project. Your machine uses both line voltage power, high pressured hot water, steam and other dangerous components. There is a risk of both damaging the machine, personal injury and property damage, the liability for which you assume yourself. This is not the stage to get on board with this project if you aren't willing to deal with those risks.

## Status

Consider this project beta quality.

### A note on Bianca versions

There are (at the time of writing) three versions of the Bianca, V1, V2, and V3. To my understanding it should work on a Bianca V1, but it's untested. As for the Bianca V3, it features upgraded hardware. Lelit sells an upgrade kit to upgrade a V1 or V2. The differences are as follows:

* A new solenoid to control full/low flow from the pump (part of the upgrade kit)
  * Has the same P/N as the V2 one, but at least in my machine the actual parts were different.
* An LCC with new firmware (part of the upgrade kit)
  * Available as either P/N 9600147, or 9600124. V2 one was 9600045.
* The power light now an LED and software controlled (not part of the upgrade kit)
  * Available as P/N 3000056, but it's ridiculously expensive

It also uses a different Gicar Control Box, but since the V3 upgrade kit doesn't include it, my suspicion is that the changes in it are marginal (it could be as simple as the box having a different sticker). The new part number is 9600125, and the old one was 9600046. One known change is the new Standby mode, which should be disabled when using Open LCC. It is unknown if this is the only change. I would love photos of the V3 Control Board internals, just to confirm that there are no relevant hardware differences.

#### V3 Bianca suppport

My Bianca is a V2, but it's been upgraded with a V3 solenoid valve. I've implemented support for the Low Flow valve, and will implement support for the power LED. To my knowledge, this has not been tested in a real V3 though. 

As for the new features of the V3:

* The new V3 standby mode might never be supported, but it's kind of a useless mode. Just disable it.
* You can use routines to do a low flow start and low flow finish.
* Brew temperature offset – I might implement this, but it's somewhat emulatable with routines
   * I'm not sure how much of a difference this makes to water temperature in the cup, the E61 group holds a *lot* of heat
   * Not to be confused with the brew temperature offset setting that already exists though. That one is just an offset from the true boiler temperature to the temperature shown on the display (default in the stock LCC is -10 degrees celsius, but I use -16.5, as that is a truer representation of the temperature at the group

### Versioning
This project uses Semver. The major version number is increased whe RP2040 <-> ESP32 protocol version is increased (as that is a BC break).

This fork tracks its own version in `src/version.h` (`RP2040_FIRMWARE_VERSION_STRING`), separate from upstream: `0.0.1` was the untouched upstream state, `1.0.0` is reached once the original LCC's functions are fully reproduced and confirmed on the machine. Published versions are tagged as `v.MAJOR.MINOR.PATCH` on the corresponding commit. The current version is `0.5.0` (tag `v.0.5.0`). See [CHANGELOG.md](CHANGELOG.md) for release notes.

### 0.5.0 changes

- A lever raised before operational readiness cannot start a shot automatically when
  readiness is reached. Lower and raise it again to request a new shot. This also
  applies to reheating after sleep. Existing accepted shots retain shot-saving behavior.
- Reuse the existing wire field `currentlyBrewing` for an accepted shot, including
  pump-off pre-infusion. It is false on lever release or during a bail. The status
  packet layout and length are identical to RP2040 0.4.0; no extra entity or fields
  are transmitted. Internal lever state is retained for existing wake/automations.
- The existing post-error recovery policy and minimum-duration counting rule remain
  unchanged. Consumers that used Currently Brewing as a raw lever signal must now
  account for its accepted-shot semantics.

There is no new update-order requirement from this change. ESPHome versions that
accept the RP2040 0.4.0 status format can also read this version. Older ESPHome may
still apply its own readiness/tank inference, so newer firmware does not retroactively
fix their display/counting logic. ESPHome 0.10.0 recognizes the accepted-shot semantics
using the existing firmware version bytes and uses legacy inference for older RP2040
versions. The physical lever interlock requires RP2040 0.5.0. Historical formats predating
0.4.0 are not all guaranteed compatible in both directions by this change.

#### Reliability fixes included in 0.5.0

- Read the JEDEC flash ID using the actual four-byte buffer length, avoiding a one-byte stack overrun.
- Cancel pending Core 1 recovery/retry timers once its status queue is no longer full. A continuous stall still triggers a restart after two seconds and retries every five seconds until recovery.
- Reject manual and automatic sleep requests during the two cold-start heat-up stages. The sequence keeps running with its original timer; sleep becomes available in normal operation. A sleep state restored at startup after a watchdog reboot remains supported. Sleep/wake resets the readiness stability clock.

The sleep gate does not reset the existing auto-sleep countdown. If it expires during
heat-up, auto-sleep can take effect once the cold-start sequence reaches normal operation
(which can precede `operationalReady` while the temperatures settle).

### Upstream rationale and behavior changes

The flash-ID transfer length is a memory-safety correction. Canceling the Core 1
recovery timer after queue recovery restores behavior already present in the
[Arduino predecessor](https://github.com/magnusnordlander/smart-lcc/blob/d8d17833ced2435663ca961b1937ad237947fcb5/firmware-arduino/src/SystemController/SystemController.cpp).

Blocking sleep during cold-start heat-up is an intentional policy of this fork.
The predecessor allowed sleep during heat-up and transitioned to a separate sleeping
state. The later RP2040 implementation retained the timer reset without that state
transition. Rejecting sleep keeps the heat-up timer valid; it is not a claim that
upstream intended to prohibit sleep. After normal operation has been reached,
sleep/wake does not restart accelerated heat-up and requires a fresh 30 seconds
of stable target temperatures before a new brew is allowed.

### Host regression checks

```sh
python3 tests/host/run.py
```

Requires a host Clang compiler with AddressSanitizer and UndefinedBehaviorSanitizer.
The tests compile the production controller, settings, automation and flash code with
small clock/queue/UART/SPI substitutes. They cover JEDEC transfer bounds, Core 1 recovery
timing, sleep rejection in both heat-up stages, sleep/wake readiness, restored sleep,
manual/automatic sleep settings, lever re-arming, accepted brew status and shot saving. They do not simulate hardware concurrency, electrical
signals or the thermal response of the machine; on-machine validation is still outstanding.

`esp-protocol.h`'s `ESPSystemStatusMessage` is kept backward compatible on purpose: new fields are always appended at the end, never inserted, and the companion [fila612/open-lcc-esphome-bianca](https://github.com/fila612/open-lcc-esphome-bianca) ESPHome firmware accepts a shorter message than it knows about, leaving newer fields at their default. That means an older RP2040 firmware (including the unpatched upstream) still works with a newer ESPHome build — features that depend on a newer field (e.g. `operational_ready`, added in `v.0.2.0`) just don't activate. The ESPHome README lists which firmware version each such feature needs.

## Project goals

Create a firmware for using the Open LCC in a Lelit Bianca to its fullest extent.

## Architecture

#### RP2040 Core 0
* System controller
    * Safety critical, uses the entire core for itself
    * Communicates with the Control Board
    * Performs a safety check, ensuring that temperatures in the boilers never exceed safe limits, and that both boilers are never running simultaneously.
    * Responsible for PID, keeping water in the boiler, running pumps etc.

#### RP2040 Core 1
* Communication with the ESP32-S3
* Reading from external sensors (via I2C etc)
* Handling Automations

### Extension boards
The Open LCC hardware has QWIIC interfaces to allow for extension. Currently the RP2040 firmware supports additional
MCP9600 thermocouple readers. 

## Building

The project is built using CMake. There are two relevant targets. `smart_lcc` and `smart_lcc_combined`. You need to first 
build the `smart_lcc` target, and *then* build the `smart_lcc_combined` target. I'm sure it would be possible to roll both
of these targets into one, but I haven't put enough effort into it yet. The reason for the two targets is the Serial
Bootloader.

### Defines

There are a number of define flags to be aware of. Firstly, there are `HARDWARE_REVISION_*` flags to set which revision
of the Open LCC Main Board you are using. Current options are `HARDWARE_REVISION_OPENLCC_R1A`, `HARDWARE_REVISION_OPENLCC_R2A`
and `HARDWARE_REVISION_OPENLCC_R2B`. You need to set one (and only one) of these.

**[MOD] R2C boards**: there is no `HARDWARE_REVISION_OPENLCC_R2C` flag, and none is needed. Per the
[open-lcc-board](https://github.com/variegated-coffee/open-lcc-board) README, R2B and R2C only changed
power-supply circuitry relative to R2A (Schottky diodes replaced by LM5050-1 OR-ing controllers, `SD_DET_A`
hardwired to GND instead of driven by the RP2040, added pulldown resistors) — no RP2040 GPIO pinout change
across R2A → R2B → R2C. Build R2C boards with `HARDWARE_REVISION_OPENLCC_R2A` (as this project's
`CMakeLists.txt` already does).

Secondly, there's `USB_DEBUG`. It enables debug output via USB-CDC, and should not be used inside an actual machine.
Outside of an actual machine (e.g. using a control board emulator), it can be useful, but it delays startup by 5 seconds
and if both cores try to print debug output at the same time, the RP2040 crashes, so it's very much just for debugging.

### Rebooting into BOOTSEL or Serial Boot

The RP2040 on the Open LCC board is controlled by the ESP32-S3; both the RESETn pin and the CSn for the flash. While the
debug board (especially the R2A debug board) has facilities to control RESETn and CSn, the recommended way to bootstrap
the project is to install the ESP32-S3 firmware first, and use that to reboot the RP2040 into BOOTSEL. Once you have
flashed the firmware (through the USB ROM bootloader), you can henceforth use Serial Boot to update it. That being said,
if you have the main board outside of your machine and the debug board connected, the USB ROM Bootloader is faster to use.

### A note on Serial Boot
This project includes a [serial third stage bootloader](https://github.com/usedbytes/rp2040-serial-bootloader). This is 
to be able to update the firmware of the RP2040 over Wi-fi via the ESP32-S3. You can still update firmware via USB, and 
in that case you should use the `smart_lcc_combined.uf2` file.

To update the firmware via Wi-fi, use [serial-flash](https://github.com/usedbytes/serial-flash) the following command:

```sh
serial-flash tcp:192.168.1.10:6638 smart_lcc_app.bin 0x10008000
```

Obviously, replace the IP address and port to match the IP address of the ESP32-S3, and the port of the serial bridge
you're using for the ESP32-S3.

## Licensing

The firmware is MIT licensed (excepting dependencies, which have their own, compatible licenses).
