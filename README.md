# PicoWinder FFB PRO — FINAL A22

Modern RP2040 USB firmware for the **Microsoft SideWinder Force Feedback Pro (Gameport)**, based on Nolan Nicholson's original **PicoWinder** project.

**PicoWinder FFB PRO** is a continuation of that work, developed through extensive debugging, USB traffic analysis, binary auditing and real hardware testing in order to obtain reliable DirectInput Force Feedback on modern PCs.

The final firmware has been extensively tested with **Condor 3**, where it provides dynamic control forces, trim, stall buffet, flutter and stable joystick operation.

---

## Why this project exists

The original PicoWinder project provided the fundamental work required to connect a Microsoft SideWinder Force Feedback Pro Gameport joystick to a modern computer through an RP2040.

The intention at the beginning of this project was **not to create a new firmware**.

The goal was simply to make the existing PicoWinder implementation work reliably.

During testing on modern Windows systems, the upstream firmware could enumerate as a USB joystick and expose Force Feedback capabilities, but reliable and complete Force Feedback operation could not be obtained in our hardware setup.

The problems were not limited to Condor 3.

During development, generic DirectInput Force Feedback test applications could also fail to correctly create or execute effects. In Condor 3, early firmware builds could exhibit symptoms such as:

* Force Feedback apparently starting and then stopping after a few seconds
* joystick input and FFB freezing together
* persistent native centering force after a reset or simulator exit
* Force Feedback behaving mostly like a simple centering spring
* dynamic effects such as buffet, flutter and trim being absent or difficult to distinguish

At the time this work was undertaken, no reproducible public demonstration was found showing the **unmodified upstream PicoWinder firmware** providing complete and stable DirectInput Force Feedback on a modern Windows PC with a clearly identified firmware build.

The investigation therefore grew into a complete debugging and validation effort.

Multiple independent firmware issues were identified and corrected, eventually producing the stable A22 release documented here.

---

## Final release

**A22** is the current definitive / **GOLDEN** release.

USB Product Name:

**`Picowinder FFB PRO`**

### SHA-256

UF2:

`49279876c95b18d4316cdd96e057593b9fdb733f064c0650adf3d5b9416b8103`

RAW:

`e074ab3b7babb206b532ba035124a6606296e0dab4012b32479f86b8af714f8b`

A22 is functionally based on the validated A21 firmware and changes the USB product name to **Picowinder FFB PRO**.

---

## What works

The final validated firmware provides:

* USB HID joystick operation
* X and Y axes
* buttons and trigger
* throttle / slider
* Rotation Z / twist
* stable input reporting at approximately **1 kHz**
* stable DirectInput Force Feedback
* dynamic Spring force
* speed-dependent control resistance
* trim represented as movement of the force equilibrium point
* stall buffet / periodic effects
* high-speed flutter
* clean Force Feedback shutdown when leaving the simulator
* no trigger-kick stale-effect-ID collision
* no persistent native SideWinder auto-center after reset
* X/Y jitter filtering
* stable full-range throttle / slider filtering
* wide Rotation-Z center dead zone
* smooth one-count Z-axis slew
* no FFB/input freeze observed during the final validated captures

---

## Condor 3 Force Feedback

Condor 3 does **not** simply send a fixed centering spring.

USB captures taken during development showed Condor dynamically generating and updating:

* Spring effects
* periodic / Triangle effects
* effect gain
* Spring center position
* effect START / STOP operations

The final firmware translates those effects to the SideWinder in a stable and usable way.

The resulting physical behavior includes:

### Speed-dependent stick force

Control resistance changes with flight speed.

At low speed the stick remains relatively light, while force progressively increases with airspeed and retains the ability to reach full force at high speed.

### Trim

Trim is not simulated merely by changing Spring strength.

Condor moves the Spring equilibrium point, and PicoWinder FFB PRO transfers that displacement to the SideWinder.

The pilot can therefore physically feel the trimmed stick position move.

### Stall buffet

Periodic effects become clearly perceptible near the stall instead of being hidden underneath an excessively strong centering Spring.

### Flutter

High-speed periodic effects are transferred strongly enough to produce clearly perceptible flutter.

---

## Major problems found and fixed

The final firmware is the result of a long sequence of targeted fixes rather than one large rewrite.

Important issues identified during development include:

### Reset / native auto-center

A DirectInput device reset could cause the SideWinder's own native centering behavior to become active again.

The final firmware explicitly disables native auto-center after reset.

### Effect table handling

The effect assignment table was being accessed using an incorrect data width in part of the original implementation.

This could cause the firmware to associate the wrong physical effect type with a DirectInput effect ID.

### Blocking USB/HID callbacks

Some Force Feedback operations could directly communicate with the SideWinder while inside USB HID callbacks.

The final architecture uses queued, asynchronous communication so that USB handling does not have to wait for the physical joystick.

### Post-reset stabilization

The SideWinder requires a short stabilization period after reset.

The final firmware uses an asynchronous reset guard before resuming normal message processing.

### SideWinder message pacing

Logical SideWinder commands are paced rather than transmitted continuously back-to-back.

This was found empirically to provide excellent stability on the tested hardware.

### Stale kick effect ID

The original diagnostic trigger-kick feature could retain an effect ID after a device reset.

Condor could subsequently reuse that same ID for its Spring effect.

Pressing and releasing the trigger could therefore accidentally PLAY and PAUSE Condor's Spring, making the Force Feedback appear to die.

The artificial kick feature was removed completely.

### Periodic effect gain

Periodic effect gain was being attenuated a second time even though the HID descriptor already used the required range.

Correcting this made stall buffet and flutter clearly perceptible.

### Spring response

The original Spring response could dominate the other effects.

A progressive response curve was developed so that low-speed control forces remain lighter while full high-speed force remains available.

### Input filtering

Separate filtering strategies were developed for:

* X/Y
* throttle / slider
* Rotation Z

rather than applying one generic smoothing algorithm to every axis.

This eliminated visible jitter while retaining full range and responsive control movement.

### Input concurrency

A race condition was identified where filtering could modify a report buffer while the SideWinder input interrupt was updating it.

The final architecture takes a local snapshot before filtering and leaves the raw interrupt-owned report untouched.

---

## Development philosophy

One rule became fundamental during this project:

**change one variable at a time and verify it.**

Development included combinations of:

* real SideWinder hardware testing
* Condor 3 testing
* generic DirectInput Force Feedback utilities
* USBPcap captures
* Wireshark / packet analysis
* ELF symbol inspection
* linker placement verification
* disassembly
* UF2 → RAW reconstruction
* binary comparison
* SHA-256 validation

Several apparently reasonable firmware changes were rejected because testing showed that they introduced regressions or were based on an incorrect diagnosis.

A22 therefore intentionally remains frozen unless a concrete, reproducible defect is discovered.

---

## Why there are two source paths

The final firmware evolved through binary-audited wrappers and surgical patches.

It was **not** produced from one monolithic edited C project.

Pretending otherwise would make this repository less reproducible, not more.

This repository therefore contains several complementary source paths:

### `exact_rebuild/`

Contains the material required to reproduce the exact physically validated A22 firmware.

This is the authoritative path when binary reproducibility matters.

### `source/history/`

Contains the actual C, ASM, linker and Python artifacts created throughout the A2 → A22 development process.

This preserves the real development history rather than presenting a reconstructed history after the fact.

### `source/final_modules/`

Contains the final logical firmware modules in readable form.

These are useful for understanding the individual final algorithms and fixes.

### `source/clean_integrated_project/`

Contains a maintainable integrated C source reconstruction intended for future development.

**Important:** a firmware newly compiled from the clean integrated tree must be considered a **new firmware version** until it has been validated again on real hardware.

It should not automatically be assumed to be binary- or behavior-identical to the physically validated A22 release.

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
local snapshot
        ↓
X/Y filtering
Slider filtering
Z mapping + slew
        ↓
USB HID
        ↓
Windows
```

Validated USB input reporting is approximately **1 kHz**.

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
SideWinder message parser
        ↓
message pacing
        ↓
SideWinder protocol
        ↓
physical joystick motors
```

The USB/HID path is never intentionally blocked while waiting for physical SideWinder communication.

---

## Flashing A22

1. Connect the SideWinder / Gameport side of the adapter.
2. Hold **BOOTSEL** while connecting the RP2040 / Pico to USB.
3. The RP2040 should appear as a USB mass-storage drive.
4. Copy:

   `firmware/PicoWinder_FFB_PRO_FINAL_A22.uf2`

   to that drive.
5. The RP2040 will reboot automatically.
6. Reconnect normally if necessary.

Windows should enumerate the USB Product String as:

**`Picowinder FFB PRO`**

Windows may cache the older friendly name of an existing USB device instance. Removing the old device instance and allowing Windows to enumerate it again can refresh the displayed name.

---

## Hardware

Target hardware:

* Microsoft **SideWinder Force Feedback Pro**
* Gameport / DA-15 version
* RP2040 or RP2040-Zero
* appropriate DA-15 interface wiring
* SideWinder external power supply

This project is specifically intended for the **Force Feedback Pro Gameport joystick**, not the later native-USB SideWinder Force Feedback 2.

See the hardware documentation in this repository before wiring the joystick.

---

## Development rule — A22 is frozen

**A22 is frozen because it works.**

Do not casually "clean up" or optimize:

* SideWinder message pacing
* reset guard timing
* native auto-center handling
* Condition semantics
* effect table handling
* periodic gain translation
* Spring response / LUT
* X/Y filtering
* slider state machine
* Z-axis mapping and slew
* USB/HID execution paths

without first identifying a reproducible defect.

Any future modification should ideally include:

1. reproduction of the problem
2. USBPcap capture
3. identification of the exact failure point
4. one controlled firmware change
5. before/after comparison
6. binary audit
7. real SideWinder hardware test

A successful compile alone is **not** sufficient validation.

---

## Upstream project and credits

PicoWinder FFB PRO is based on the original **PicoWinder** project by **Nolan Nicholson**:

`https://github.com/NolanNicholson/picowinder`

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

### FINAL / GOLDEN

**PicoWinder FFB PRO A22**

The final validated firmware provides a practical way to use a **Microsoft SideWinder Force Feedback Pro Gameport joystick on a modern PC with real dynamic Force Feedback**.

For the complete technical history, validation process and source mapping, see:

* `docs/SOURCE_MAP.md`
* `CHANGELOG.md`
* `docs/MEGA_HANDOFF_FINAL_A22.txt`
* `exact_rebuild/`
* `source/history/`

A22 remains the recommended release until a reproducible defect justifies further development.

---

*Microsoft and SideWinder are trademarks of Microsoft Corporation.*

*Condor is a trademark of its respective owner.*

*This is an independent open-source community project and is not affiliated with or endorsed by Microsoft, Condor, or the original PicoWinder author.*
