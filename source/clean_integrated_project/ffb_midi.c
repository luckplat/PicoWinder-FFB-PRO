#include "ffb_midi.h"
#include "ffb_transport.h"

static uint8_t effects_assigned[EFFECT_MEMORY_START + EFFECT_MEMORY_SIZE];
static bool last_add_succeeded;
static uint8_t last_assigned_effect_id;

static inline uint8_t lo7(uint16_t v) { return (uint8_t)(v & 0x7fu); }
static inline uint8_t hi7(uint16_t v) { return (uint8_t)((v >> 7) & 0x7fu); }

int ffb_midi_get_free_effect_id(void) {
    for (int i = EFFECT_MEMORY_START; i < EFFECT_MEMORY_START + EFFECT_MEMORY_SIZE; ++i)
        if (effects_assigned[i] == MIDI_ET_NONE) return i;
    return -1;
}

size_t ffb_midi_get_num_available_effects(void) {
    size_t n = 0;
    for (int i = EFFECT_MEMORY_START; i < EFFECT_MEMORY_START + EFFECT_MEMORY_SIZE; ++i)
        if (effects_assigned[i] == MIDI_ET_NONE) ++n;
    return n;
}

bool ffb_midi_last_add_succeeded(void) { return last_add_succeeded; }
uint8_t ffb_midi_last_assigned_effect_id(void) { return last_assigned_effect_id; }
MidiEffectType ffb_midi_effect_type(uint8_t id) {
    return id < EFFECT_MEMORY_START + EFFECT_MEMORY_SIZE ? (MidiEffectType)effects_assigned[id] : MIDI_ET_NONE;
}

void ffb_midi_reset_allocator(void) {
    for (uint32_t i = EFFECT_MEMORY_START; i < EFFECT_MEMORY_START + EFFECT_MEMORY_SIZE; ++i)
        effects_assigned[i] = MIDI_ET_NONE;
}

void ffb_midi_mark_free(uint8_t id) {
    if (id < EFFECT_MEMORY_START + EFFECT_MEMORY_SIZE) effects_assigned[id] = MIDI_ET_NONE;
}

void ffb_midi_set_autocenter(bool enabled) {
    uint8_t msg[2] = {0xc5u, enabled ? 0x01u : 0x06u};
    ffb_transport_enqueue(msg, sizeof msg);
}

int ffb_midi_define_effect(const Effect *effect) {
    int effect_id = ffb_midi_get_free_effect_id();
    if (!effect || effect_id < 0) {
        last_add_succeeded = false;
        return -1;
    }

    uint8_t d[33] = {
        0xf0u, 0x00u, 0x01u, 0x0au, 0x01u,
        effect->play_immediately ? 0x24u : 0x23u,
        (uint8_t)effect->type,
        0x7fu,
        lo7(effect->duration), hi7(effect->duration),
        lo7(effect->button_mask), hi7(effect->button_mask)
    };

    uint8_t next = 0u;
    switch (effect->type) {
        case MIDI_ET_CONSTANT:
        case MIDI_ET_SINE:
        case MIDI_ET_SQUARE:
        case MIDI_ET_RAMP:
        case MIDI_ET_TRIANGLE:
        case MIDI_ET_SAWTOOTHDOWN:
        case MIDI_ET_SAWTOOTHUP:
            d[12]=lo7(effect->direction); d[13]=hi7(effect->direction);
            d[14]=effect->gain;
            d[15]=lo7(effect->sample_rate); d[16]=hi7(effect->sample_rate);
            d[17]=0x10u; d[18]=0x4eu;
            d[19]=effect->attack_level;
            d[20]=lo7(effect->attack_time); d[21]=hi7(effect->attack_time);
            d[22]=effect->sustain_level;
            d[23]=lo7(effect->fade_time); d[24]=hi7(effect->fade_time);
            d[25]=effect->fade_level;
            d[26]=lo7(effect->frequency); d[27]=hi7(effect->frequency);
            d[28]=lo7(effect->amplitude); d[29]=hi7(effect->amplitude);
            d[30]=0x01u; d[31]=0x01u;
            next=32u;
            break;
        case MIDI_ET_SPRING:
        case MIDI_ET_DAMPER:
        case MIDI_ET_INERTIA:
            d[12]=effect->strength_x; d[13]=0x00u;
            d[14]=effect->strength_y; d[15]=0x00u;
            d[16]=lo7(effect->offset_x); d[17]=hi7(effect->offset_x);
            d[18]=lo7(effect->offset_y); d[19]=hi7(effect->offset_y);
            next=20u;
            break;
        case MIDI_ET_FRICTION:
            d[12]=effect->strength_x; d[13]=0x00u;
            d[14]=effect->strength_y; d[15]=0x00u;
            next=16u;
            break;
        default:
            last_add_succeeded=false;
            return -1;
    }

    uint8_t checksum=0u;
    for (uint8_t i=5u; i<next; ++i) checksum=(uint8_t)(checksum+d[i]);
    d[next++]=(uint8_t)(0x80u-(checksum&0x7fu));
    d[next++]=0xf7u;

    if (!ffb_transport_enqueue(d,next)) {
        last_add_succeeded=false;
        return -1;
    }

    last_add_succeeded=true;
    last_assigned_effect_id=(uint8_t)effect_id;
    effects_assigned[effect_id]=(uint8_t)effect->type;
    return effect_id;
}
