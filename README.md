# PicoWinder-FFB-PRO

Modern USB adapter firmware for the **Microsoft SideWinder Force Feedback Pro (Gameport)** using an RP2040 / RP2040-Zero.

This project is a continuation of the original **PicoWinder** project by Nolan Nicholson:

https://github.com/NolanNicholson/picowinder

## Why this project exists

The original PicoWinder project provided the fundamental hardware interface and USB HID / Force Feedback implementation required to connect the Gameport Microsoft SideWinder Force Feedback Pro to a modern PC.

However, during extensive testing on modern Windows systems, the original firmware did not provide reliable and complete Force Feedback operation.

The project therefore evolved from an attempt to make the existing firmware work correctly into a substantially debugged, corrected and experimentally validated version.

## Current release

**PicoWinder FFB PRO A22 – FINAL / GOLDEN**

The A20 firmware has been validated with:

- Windows 10 / Windows 11
- Microsoft SideWinder Force Feedback Pro Gameport
- RP2040 / RP2040-Zero
- DirectInput Force Feedback test applications
- Condor 3

Validated functionality includes:

- USB HID joystick input
- X / Y axes
- throttle
- twist / Z axis
- buttons and trigger
- approximately 1 kHz input reports
- stable DirectInput Force Feedback
- dynamic spring force
- speed-dependent control force
- trim force / equilibrium displacement
- stall buffet
- flutter
- periodic effects
- correct Force Feedback cleanup
- no residual centering spring after application exit
- no FFB/input freeze observed in the final validated firmware

## Major fixes

Development from the original PicoWinder code included fixes and improvements to:

- SideWinder reset and native autocenter handling
- HID / Force Feedback effect handling
- non-blocking communication
- effect table handling
- SideWinder message pacing
- post-reset stabilization
- stale Force Feedback effect IDs
- periodic effect gain
- spring force response
- input jitter filtering
- throttle filtering
- Z-axis filtering
- input report concurrency
- firmware build and binary validation

Full technical documentation and development history will be added to this repository.

## License and credits

Based on the original **PicoWinder** project by **Nolan Nicholson**.

Original project:
https://github.com/NolanNicholson/picowinder

This project retains the **GPL-3.0** license of the upstream project.

Microsoft and SideWinder are trademarks of Microsoft Corporation.

This is an independent community project and is not affiliated with or endorsed by Microsoft.
