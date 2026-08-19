#ifndef CONFIG_H
#define CONFIG_H

/* Hardware mapping inherited from upstream PicoWinder. */
#define PIN_MIDI_TX 0
#define PIN_TRIGGER 2
#define PIN_CLK     3
#define PIN_D0      4
#define PIN_D1      5
#define PIN_D2      6

/* A22 keeps the SideWinder native auto-center disabled. */
#define DISABLE_AUTO_CENTER

/* IMPORTANT: EXAMPLE_EFFECTS is intentionally NOT enabled.
 * The old trigger kick caused a stale effect-ID collision after Device Reset.
 */

/* Uncomment only if the shifted-button mode is wanted. */
/* #define FIRMWARE_SHIFT */

#endif
