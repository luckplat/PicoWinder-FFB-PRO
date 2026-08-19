# Clean integrated A22 source

This folder is a **maintainable source reconstruction** of the final A22 behavior.
It folds the validated A2..A22 changes into normal C modules instead of absolute-address
binary wrappers.

Important distinction:

- `firmware/PicoWinder_FFB_PRO_FINAL_A22.uf2` is the physically validated release.
- `exact_rebuild/` reproduces that exact UF2 bit-for-bit from the exact A21 base.
- this folder expresses the same final logic in readable, maintainable C for future work.

The historical A22 binary was not produced from one monolithic C source tree; it evolved
through carefully audited binary wrappers/patches. Therefore it would be misleading to
claim this clean tree is already bit-identical without rebuilding it with the original
Pico SDK/toolchain and doing a fresh hardware/PCAP validation.

## Build preparation

1. Install Raspberry Pi Pico SDK / toolchain.
2. Run `python vendor/fetch_upstream_unmodified.py`.
3. Configure/build with CMake in the usual Pico SDK way.

## Final behavior represented here

- 31,250 baud SideWinder FFB transport.
- nonblocking 4096-byte queue.
- 1 ms inter-message pacing; 75 ms after SideWinder Reset.
- Device Reset followed by native auto-center OFF.
- no local trigger kick.
- direct periodic gain (no accidental second halving).
- A21 final progressive Spring curve.
- A20 X/Y, slider and Rotation-Z filtering.
- race-safe local HID report snapshot.
- USB Product String: `Picowinder FFB PRO`.
