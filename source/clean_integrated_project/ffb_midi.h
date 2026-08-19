#ifndef FFB_MIDI_H
#define FFB_MIDI_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum {
    MIDI_ET_NONE = 0x00,
    MIDI_ET_SINE = 0x02,
    MIDI_ET_SQUARE = 0x05,
    MIDI_ET_RAMP = 0x06,
    MIDI_ET_TRIANGLE = 0x08,
    MIDI_ET_SAWTOOTHDOWN = 0x0a,
    MIDI_ET_SAWTOOTHUP = 0x0b,
    MIDI_ET_SPRING = 0x0d,
    MIDI_ET_DAMPER = 0x0e,
    MIDI_ET_INERTIA = 0x0f,
    MIDI_ET_FRICTION = 0x10,
    MIDI_ET_CONSTANT = 0x12,
} MidiEffectType;

#define EFFECT_MEMORY_SIZE 39
#define EFFECT_MEMORY_START 2
#define MAX_SIMULTANEOUS_EFFECTS 10
#define MIDI_ALL_EFFECTS 0x7f

typedef struct {
    bool play_immediately;
    MidiEffectType type;
    uint16_t duration;
    uint16_t button_mask;
    uint16_t direction;
    uint8_t gain;
    uint8_t sample_rate;
    uint8_t attack_level;
    uint8_t sustain_level;
    uint8_t fade_level;
    uint16_t attack_time;
    uint16_t fade_time;
    uint16_t frequency;
    uint16_t amplitude;
    uint8_t strength_x;
    uint8_t strength_y;
    uint16_t offset_x;
    uint16_t offset_y;
} Effect;

int ffb_midi_get_free_effect_id(void);
size_t ffb_midi_get_num_available_effects(void);
bool ffb_midi_last_add_succeeded(void);
uint8_t ffb_midi_last_assigned_effect_id(void);
MidiEffectType ffb_midi_effect_type(uint8_t id);
void ffb_midi_reset_allocator(void);
void ffb_midi_mark_free(uint8_t id);
void ffb_midi_set_autocenter(bool enabled);
int ffb_midi_define_effect(const Effect *effect);

#endif
