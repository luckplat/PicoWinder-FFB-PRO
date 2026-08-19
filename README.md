# PicoWinder FFB PRO — FINAL

Modern RP2040 USB firmware for the **Microsoft SideWinder Force Feedback Pro (Gameport)**, based on Nolan Nicholson's original **PicoWinder** project.

**PicoWinder FFB PRO** is a continuation and extensively debugged development of PicoWinder, created to provide reliable **USB HID joystick input and DirectInput Force Feedback on modern PCs**.

The firmware has been tested with standard DirectInput Force Feedback utilities and extensively validated in real applications, including **Condor 3**.

<p align="center">
  <img src="Images/Picowinder_FFB_PRO_01.jpg" width="650" alt="Microsoft SideWinder Force Feedback Pro with PicoWinder FFB PRO adapter">
</p>

<p align="center">
  <em>Microsoft SideWinder Force Feedback Pro Gameport with the PicoWinder FFB PRO RP2040 USB adapter.</em>
</p>

---

## Why this project exists

The original PicoWinder project provided the fundamental RP2040 interface required to connect a Microsoft SideWinder Force Feedback Pro Gameport joystick to a modern computer.

The original goal of this work was **not to create a new firmware**.

The goal was simply to make the existing PicoWinder implementation work reliably.

During testing on modern Windows systems, the upstream firmware could enumerate as a USB joystick and expose Force Feedback capabilities, but reliable and complete DirectInput FFB operation could not be obtained on our hardware.

The problems were not limited to Condor 3: generic DirectInput Force Feedback test applications could also fail to correctly create or execute effects.

Typical symptoms included:

* FFB stopping after a few seconds
* joystick input and FFB freezing together
* persistent native centering after reset
* Force Feedback behaving mostly as a simple centering spring
* dynamic effects being weak, missing or unreliable

At the time this work was undertaken, no reproducible public demonstration was found showing the **unmodified upstream PicoWinder firmware** providing complete and stable DirectInput Force Feedback on a modern Windows PC with a clearly identified firmware build.

The investigation therefore evolved into a complete debugging and validation effort.

---

## What works

The final validated firmware provides:

* Microsoft SideWinder Force Feedback Pro Gameport → USB
* USB HID joystick operation
* X / Y axes
* throttle / slider
* Rotation Z / twist
* buttons and trigger
* approximately **1 kHz** input reporting
* stable Windows DirectInput Force Feedback
* dynamic Spring effects
* periodic effects
* speed-dependent control resistance
* trim / moving equilibrium point
* stall buffet
* high-speed flutter
* clean effect shutdown
* correct reset and native auto-center handling
* X/Y jitter filtering
* stable full-range throttle filtering
* smooth Rotation-Z filtering
* no FFB/input freeze observed during final validation

USB Product Name:

**`Picowinder FFB PRO`**

---

## The adapter

The hardware is deliberately simple: an **RP2040 / RP2040-Zero** acts as the interface between the original SideWinder Gameport connection and USB.

<p align="center">
  <img src="Images/Picowinder_FFB_PRO_02.jpg" width="700" alt="PicoWinder FFB PRO complete adapter">
</p>

The adapter retains the original SideWinder's own electronics and Force Feedback motors. The RP2040 handles the communication between the legacy SideWinder protocol and modern USB HID / DirectInput PID.

<p align="center">
  <img src="Images/Picowinder_FFB_PRO_03.jpg" width="700" alt="PicoWinder FFB PRO RP2040 USB-C enclosure">
</p>

The RP2040 is housed in a small enclosure and connects to the PC through USB.

---

## Force Feedback validation

Condor 3 was used extensively as a demanding real-world Force Feedback test environment.

USB captures showed the simulator dynamically creating and updating:

* Spring effects
* periodic / Triangle effects
* effect gain
* Spring center position
* START / STOP operations

The final firmware correctly transfers those effects to the physical SideWinder.

### Speed-dependent control force

The stick remains relatively light at low speed and becomes progressively heavier as airspeed increases.

### Trim

The Spring equilibrium point physically moves with trim, allowing the pilot to feel the trimmed stick position rather than merely experiencing a change in overall force.

### Stall buffet

Periodic effects become clearly perceptible close to the stall.

### Flutter

High-speed periodic effects produce clearly noticeable flutter.

**Condor 3 is one of the main validation applications, but PicoWinder FFB PRO is not simulator-specific.**

The firmware implements standard **Windows DirectInput Force Feedback** and can therefore be used by other compatible applications and simulators.

---

## Main fixes and improvements

The final firmware resulted from many targeted corrections rather than one large rewrite.

The major areas addressed include:

* native SideWinder auto-center reactivation after reset
* Force Feedback effect-table handling
* blocking communication inside USB/HID callbacks
* asynchronous SideWinder communication
* post-reset stabilization
* SideWinder message pacing
* stale Force Feedback effect IDs
* periodic-effect gain translation
* Spring response
* X/Y input jitter
* throttle / slider filtering
* Rotation-Z filtering
* input-buffer concurrency

### Stale Force Feedback effect ID

One particularly important problem involved the original diagnostic trigger-kick effect.

After a device reset, its old effect ID could remain locally while an application reused the same ID for another Force Feedback effect.

The trigger could then unintentionally PLAY or PAUSE the application's effect, making the FFB appear to stop working.

The artificial diagnostic kick was therefore removed completely.

### Reset and native auto-center

A DirectInput device reset could cause the SideWinder's own native centering behavior to become active again.

The final firmware explicitly disables native auto-center after reset and includes a stabilization period before normal FFB communication resumes.

### USB / SideWinder communication

Force Feedback communication is queued and asynchronous.

USB HID processing therefore does not have to wait for the much slower physical SideWinder communication path.

Logical SideWinder messages are also paced rather than being transmitted continuously back-to-back.

### Periodic effects and Spring response

Periodic-effect gain handling was corrected, making effects such as buffet and flutter clearly perceptible.

The Spring response was also reshaped so that it no longer unnecessarily hides weaker dynamic effects while still retaining full force when required.

---

## Input stability

The SideWinder's original analog controls can exhibit small amounts of raw jitter.

Instead of applying one generic smoothing algorithm to every control, the final firmware uses specific strategies for:

* X / Y
* throttle / slider
* Rotation Z

The result is stable joystick input without sacrificing useful range or introducing noticeable control latency.

Validated USB input reporting is approximately **1 kHz**.

---

## Firmware architecture

### Input path

```text
SideWinder Force Feedback Pro
        ↓
Gameport protocol
        ↓
RP2040 PIO / decoder
        ↓
RAW input report
        ↓
local snapshot + axis filtering
        ↓
USB HID
        ↓
Windows
```

### Force Feedback path

```text
Windows / DirectInput PID
        ↓
USB HID OUTPUT / FEATURE reports
        ↓
Force Feedback translation
        ↓
non-blocking FIFO
        ↓
SideWinder message pacing
        ↓
SideWinder protocol
        ↓
physical joystick motors
```

The USB/HID path does not intentionally block while waiting for physical SideWinder communication.

---

## Hardware

Target hardware:

* Microsoft **SideWinder Force Feedback Pro**
* Gameport / DA-15 version
* RP2040 or RP2040-Zero
* appropriate DA-15 interface wiring
* original or compatible SideWinder external power supply

This project is specifically intended for the **Microsoft SideWinder Force Feedback Pro Gameport joystick**.

It is not intended for the later native-USB **SideWinder Force Feedback 2**, which does not require this type of Gameport-to-USB conversion.

See the hardware documentation in this repository before wiring the adapter.

---

## Flashing

The ready-to-use firmware is available from the **FINAL GitHub Release**.

To flash it manually:

1. Connect the SideWinder / Gameport side of the adapter.
2. Hold **BOOTSEL** while connecting the RP2040 / Pico to USB.
3. The RP2040 should appear as a USB mass-storage drive.
4. Copy:

   `firmware/PicoWinder_FFB_PRO_FINAL_A22.uf2`

   to the RP2040 drive.
5. The board reboots automatically.
6. Reconnect normally if necessary.

Windows should enumerate the USB Product String as:

**`Picowinder FFB PRO`**

Windows can cache the friendly name of an existing USB device instance. Removing and re-enumerating the device can refresh the displayed name.

### Firmware SHA-256

`49279876c95b18d4316cdd96e057593b9fdb733f064c0650adf3d5b9416b8103`

The public **FINAL** release corresponds internally to the validated **A22** development build.

---

## Development and validation

Development relied heavily on measurement and reproducible testing rather than trial-and-error tuning.

Tools and methods included:

* real SideWinder hardware testing
* generic DirectInput Force Feedback utilities
* Condor 3
* USBPcap
* Wireshark / USB packet analysis
* ELF symbol inspection
* linker-placement verification
* disassembly
* UF2 → RAW reconstruction
* binary comparison
* SHA-256 verification

The fundamental development rule became:

**Change one variable at a time and verify it.**

Several apparently reasonable firmware changes were rejected because testing showed regressions or because the original diagnosis proved incorrect.

---

## Source structure

The final firmware was developed incrementally through binary-audited wrappers and targeted patches.

It was **not** produced from one monolithic edited C project.

For transparency and reproducibility, this repository contains several complementary source paths.

### `exact_rebuild/`

Contains the material required to reproduce the exact physically validated FINAL firmware.

This is the authoritative path when binary reproducibility matters.

### `source/history/`

Contains the actual C, ASM, linker and Python artifacts produced throughout development.

### `source/final_modules/`

Contains the final logical firmware modules in a more readable form.

### `source/clean_integrated_project/`

Contains a maintainable integrated C-source reconstruction intended for future development.

**Important:** firmware newly compiled from the clean integrated tree must be considered a new firmware build until it has been validated again on real hardware.

It should not automatically be assumed to be binary- or behavior-identical to the validated FINAL firmware.

---

## Development rule

The current firmware is **FINAL because it works**.

Do not casually modify or "clean up":

* SideWinder message pacing
* reset timing
* native auto-center handling
* DirectInput / PID semantics
* effect-table handling
* periodic gain translation
* Spring response
* axis filters
* USB/HID execution paths

without first identifying a reproducible defect.

Future changes should ideally include:

1. reproduction of the problem
2. USBPcap capture
3. identification of the exact failure
4. one controlled modification
5. before/after comparison
6. binary audit
7. real SideWinder hardware testing

A successful compile alone is **not** sufficient validation.

---

## Upstream project and credits

PicoWinder FFB PRO is based on the original **PicoWinder** project by **Nolan Nicholson**:

https://github.com/NolanNicholson/picowinder

The upstream project established the fundamental RP2040 hardware interface, SideWinder communication and USB HID / PID implementation on which this work is based.

PicoWinder FFB PRO should therefore be understood as a **continuation and extensively debugged development of PicoWinder**, not as an unrelated project.

Full credit for the original PicoWinder work remains with Nolan Nicholson.

---

## License

The original PicoWinder project is distributed under the **GNU General Public License v3.0 (GPL-3.0)**.

This modified work is distributed under **GPL-3.0** as well.

When redistributing this project or derivatives, preserve the upstream attribution and comply with the GPL-3.0 license terms.

---

## Project status

### FINAL

The current validated firmware provides a practical way to use a **Microsoft SideWinder Force Feedback Pro Gameport joystick on a modern PC with real DirectInput Force Feedback**.

For the complete technical history and validation process, see:

* `docs/SOURCE_MAP.md`
* `CHANGELOG.md`
* `docs/MEGA_HANDOFF_FINAL_A22.txt`
* `exact_rebuild/`
* `source/history/`

The current FINAL release remains recommended until a reproducible defect justifies further development.

---

*Microsoft and SideWinder are trademarks of Microsoft Corporation.*

*Condor is a trademark of its respective owner.*

*This is an independent open-source community project and is not affiliated with or endorsed by Microsoft, Condor, or the original PicoWinder author.*
