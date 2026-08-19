# Source map: FINAL A22

The final firmware is the result of an audited evolution, not one blind rewrite.

| Final behavior | Source/reference in this package |
|---|---|
| Nonblocking FFB engine / effect state | `source/history/*A3*ffb_output_engine*` and `clean_integrated_project/ffb_engine.c` |
| Device Reset + native autocenter off | A2/A3 history, `ffb_engine.c` |
| Correct effect table byte ABI | A3 history |
| Nonblocking DEFINE | A4 rebuild/proof history, `ffb_midi.c` |
| Stock-style Condition semantics | `final_modules/condition_semantics_A10_REFERENCE.c`, `ffb_engine.c` |
| 75 ms Reset guard | `final_modules/reset_guard_A11_REFERENCE.c` |
| 1 ms message pacing | `final_modules/ffb_message_pacing_A12_FINAL.c`, `ffb_transport.c` |
| Remove trigger kick | A13 rebuild history, no EXAMPLE_EFFECTS in final `config.h`/`main.c` |
| Periodic gain fix | A14 rebuild history, `ffb_engine.c` |
| Spring tuning | A21 LUT in `final_modules/spring_curve_A21_FINAL.c` and `spring_curve_a21.h` |
| X/Y + slider + Rotation Z filters | `final_modules/input_filter_A20_FINAL.c`, `input_filter.c` |
| Input report race fix | local snapshot in A20 source / `input_filter.c` |
| USB name `Picowinder FFB PRO` | A22 exact rebuild + `usb_descriptors.c` |

## Two kinds of source are included

### 1. Historical/exact development source
These are the C/assembly/linker/Python artifacts actually used to create and audit the firmware lineage. They preserve the real development history and absolute-address wrappers.

### 2. Clean integrated source
A maintainable C reconstruction that removes absolute addresses and folds the final behavior into normal modules. This is the recommended tree for future development, but after any rebuild it must be hardware-tested again before calling it a replacement for the published A22 UF2.
