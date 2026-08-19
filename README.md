# PicoWinder FFB PRO — FINAL

Modern RP2040 USB firmware for the **Microsoft SideWinder Force Feedback Pro (Gameport)**, based on Nolan Nicholson's original **PicoWinder** project.

**PicoWinder FFB PRO** is a continuation and extensively debugged development of PicoWinder, created to provide reliable **USB HID joystick input and DirectInput Force Feedback on modern PCs**.

The firmware has been tested with standard Force Feedback utilities and extensively validated in real applications, including **Condor 3**.

---

## Why this project exists

The original PicoWinder project provided the fundamental RP2040 interface required to connect a Microsoft SideWinder Force Feedback Pro Gameport joystick to a modern computer.

The original goal of this work was not to create a new firmware, but simply to make the existing implementation work reliably.

During testing, the upstream firmware could enumerate as a USB joystick and expose Force Feedback capabilities, but reliable and complete FFB operation could not be obtained on our hardware.

Problems were not limited to Condor 3: generic DirectInput Force Feedback test applications could also fail to correctly create or execute effects.

Typical symptoms included:

* FFB stopping after a few seconds
* joystick input and FFB freezing together
* persistent native centering after reset
* FFB behaving mostly as a centering spring
* dynamic effects being weak, missing or unreliable

At the time this work was undertaken, no reproducible public demonstration was found showing the **unmodified upstream PicoWinder firmware** providing complete and stable DirectInput Force Feedback on a modern Windows PC with a clearly identified firmware build.

The investigation eventually became a complete debugging and validation effort.

---

## What works

The final validated firmware provides:

* USB HID joystick operation
* X / Y axes
* throttle / slider
* Rotation Z / twist
* buttons and trigger
* approximately **1 kHz** input reporting
* stable DirectInput Force Feedback
* dynamic Spring force
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

## Force Feedback validation

Condor 3 was used extensively as a demanding real-world FFB test environment.

USB captures showed the simulator dynamically creating and updating:

* Spring effects
* periodic / Triangle effects
* effect gain
* Spring center position
* START / STOP operations

The final firmware correctly transfers these effects to the SideWinder.

In practice this produces:

### Speed-dependent control force

The stick remains relatively light at low speed and becomes progressively heavier as airspeed increases.

### Trim

The equilibrium point of the Spring physically moves with trim, so the pilot can feel the trimmed stick position.

### Stall buffet

Periodic effects are clearly perceptible close to the stall.

### Flutter

High-speed periodic effects produce clearly noticeable flutter.

Condor 3 is one of the main validation applications, but **PicoWinder FFB PRO is not simulator-specific**: it implements standard Windows DirectInput Force Feedback.

---

## Main fixes and improvements

The final firmware resulted from many targeted corrections rather than one large rewrite.

Major areas addressed include:

* native auto-center reactivation after reset
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

One especially important issue involved the original diagnostic trigger-kick effect: after a reset, its old effect ID could be reused by an application for another effect. The trigger could then accidentally pause that effect. The artificial kick was therefore removed completely.

---

## Development and validation

Development relied heavily on measurement rather than trial-and-error tuning.

Tools and methods included:

* real SideWinder hardware testing
* DirectInput Force Feedback utilities
* Condor 3
* USBPcap
* Wireshark / USB packet analysis
* ELF symbol inspection
* linker-placement verification
* disassembly
* UF2 → RAW reconstruction
* binary comparison
* SHA-256 verification

The basic development rule was:

**Change one variable at a time and verify it.**

Several apparently reasonable changes were rejected because testing showed regressions or an incorrect diagnosis.

---

## Source structure

The final firmware was developed incrementally through binary-audited wrappers and surgical patches.

It was **not** produced from one monolithic edited C project.

For transparency and reproducibility, the repository contains several source paths.

### `exact_rebuild/`

Reproduces the exact physically validated final binary.

This is the authoritative path when binary reproducibility matters.

### `source/history/`

Contains the actual C, ASM, linker and Python artifacts produced throughout development.

### `source/final_modules/`

Contains the final logical firmware modules in a more readable form.

### `source/clean_integrated_project/`

Contains a maintainable integrated C-source reconstruction intended for future development.

**Important:** a newly compiled firmware from the clean integrated tree must be treated as a new firmware build until it has been validated again on real hardware.

The public **FINAL** release corresponds internally to the validated **A22** build.

---

## Firmware architecture

### Input

```text
SideWinder Force Feedback Pro
        ↓
Gameport protocol
        ↓
RP2040 PIO / decoder
        ↓
RAW input report
        ↓
local snapshot + filtering
        ↓
USB HID
        ↓
Windows
```

Validated USB input reporting is approximately **1 kHz**.

### Force Feedback

```text
Windows / DirectInput PID
        ↓
USB HID OUTPUT / FEATURE
        ↓
FFB translation
        ↓
non-blocking FIFO
        ↓
SideWinder message pacing
        ↓
SideWinder protocol
        ↓
physical joystick motors
```

The USB/HID path does not wait synchronously for physical SideWinder communication.

---

## Flashing

1. Connect the SideWinder / Gameport side of the adapter.
2. Hold **BOOTSEL** while connecting the RP2040 / Pico to USB.
3. The RP2040 appears as a USB mass-storage drive.
4. Copy:

   `firmware/PicoWinder_FFB_PRO_FINAL_A22.uf2`

   to the RP2040 drive.
5. The board reboots automatically.

Windows should enumerate the device as:

**`Picowinder FFB PRO`**

Windows can cache an older friendly name for an existing USB device instance. Removing and re-enumerating the device can refresh it.

### Firmware SHA-256

`49279876c95b18d4316cdd96e057593b9fdb733f064c0650adf3d5b9416b8103`

---

## Hardware

Target hardware:

* Microsoft **SideWinder Force Feedback Pro**
* Gameport / DA-15 version
* RP2040 or RP2040-Zero
* appropriate DA-15 interface wiring
* original or compatible SideWinder external power supply

This project is intended for the **Force Feedback Pro Gameport joystick**, not the later native-USB SideWinder Force Feedback 2.

See the repository hardware documentation before wiring the adapter.

---

## Development rule

The current firmware is **FINAL because it works**.

Do not casually modify or "clean up":

* message pacing
* reset timing
* auto-center handling
* DirectInput/PID semantics
* effect gain translation
* Spring response
* axis filters
* USB/HID paths

without first finding a reproducible defect.

Future changes should ideally include:

1. reproduction of the problem
2. USBPcap capture
3. identification of the failure
4. one controlled modification
5. before/after comparison
6. binary audit
7. real hardware testing

A successful compile alone is **not** sufficient validation.

---

## Upstream project and credits

PicoWinder FFB PRO is based on the original **PicoWinder** project by **Nolan Nicholson**:

`https://github.com/NolanNicholson/picowinder`

The upstream project established the fundamental RP2040 hardware interface, SideWinder communication and USB HID / PID implementation on which this work is based.

PicoWinder FFB PRO is therefore a **continuation and extensively debugged development of PicoWinder**, not an unrelated project.

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
