#include "tusb.h"
#include "usb_report_ids.h"
#include "ffb_midi.h"
#include "ffb_engine.h"

static const uint8_t effect_type_usb_to_midi[]={
    MIDI_ET_NONE,MIDI_ET_CONSTANT,MIDI_ET_RAMP,MIDI_ET_SQUARE,MIDI_ET_SINE,
    MIDI_ET_TRIANGLE,MIDI_ET_SAWTOOTHUP,MIDI_ET_SAWTOOTHDOWN,MIDI_ET_SPRING,
    MIDI_ET_DAMPER,MIDI_ET_INERTIA,MIDI_ET_FRICTION
};

uint16_t tud_hid_get_report_cb(uint8_t instance,uint8_t report_id,hid_report_type_t report_type,uint8_t*buffer,uint16_t reqlen){
    (void)instance;(void)reqlen;
    if(report_type==HID_REPORT_TYPE_FEATURE){
        if(report_id==REPORT_ID_FEATURE_BLOCK_LOAD){
            buffer[0]=ffb_midi_last_assigned_effect_id();
            buffer[1]=ffb_midi_last_add_succeeded()?1u:2u;
            buffer[2]=(uint8_t)ffb_midi_get_num_available_effects();buffer[3]=0u;return 4u;
        }
        if(report_id==REPORT_ID_FEATURE_POOL_REPORT){
            buffer[0]=EFFECT_MEMORY_SIZE;buffer[1]=0u;buffer[2]=MAX_SIMULTANEOUS_EFFECTS;buffer[3]=0xffu;return 4u;
        }
    }
    return 0u;
}

void tud_hid_set_report_cb(uint8_t instance,uint8_t report_id,hid_report_type_t report_type,uint8_t const*buffer,uint16_t bufsize){
    (void)instance;
    if(report_type==HID_REPORT_TYPE_OUTPUT){
        /* Final A22 route: custom nonblocking engine, no stock echo path. */
        ffb_engine_handle_output(report_id,buffer,bufsize);
        return;
    }
    if(report_type==HID_REPORT_TYPE_FEATURE && report_id==REPORT_ID_FEATURE_CREATE_NEW_EFFECT && buffer && bufsize>=1u){
        uint8_t t=buffer[0];
        if(t<sizeof(effect_type_usb_to_midi)){
            Effect e={
                .play_immediately=false,.type=(MidiEffectType)effect_type_usb_to_midi[t],
                .duration=0,.button_mask=0,.direction=0,.gain=0x7f,.sample_rate=100,
                .attack_level=0x7f,.sustain_level=0x7f,.fade_level=0x7f,
                .attack_time=0,.fade_time=0,.frequency=1,.amplitude=0x7f,
                .strength_x=0,.strength_y=0,.offset_x=0,.offset_y=0
            };
            ffb_midi_define_effect(&e);
        }
    }
}
