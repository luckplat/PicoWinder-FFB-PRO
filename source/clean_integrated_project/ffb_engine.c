#include "ffb_engine.h"
#include "ffb_midi.h"
#include "ffb_transport.h"
#include "spring_curve_a21.h"

#include <stdbool.h>
#include <stdint.h>

#define RID_SET_EFFECT       2u
#define RID_SET_ENVELOPE     3u
#define RID_SET_CONDITION    4u
#define RID_SET_PERIODIC     5u
#define RID_SET_CONSTANT     6u
#define RID_SET_RAMP         7u
#define RID_EFFECT_OPERATION 8u
#define RID_BLOCK_FREE       9u
#define RID_DEVICE_CONTROL  10u
#define RID_DEVICE_GAIN     11u

#define MOD_DURATION    0x40u
#define MOD_TRIGGER     0x44u
#define MOD_DIRECTION   0x48u
#define MOD_GAIN        0x4cu
#define MOD_SAMPLE_RATE 0x50u
#define MOD_ATTACK_TIME 0x5cu
#define MOD_FADE_TIME   0x60u
#define MOD_ATTACK      0x64u
#define MOD_MAGNITUDE   0x68u
#define MOD_FADE        0x6cu
#define MOD_FREQUENCY   0x70u
#define MOD_PARAM1      0x74u
#define MOD_PARAM2      0x78u
#define MOD_DEVICE_GAIN 0x7cu
#define MOD_COEFF_X     0x48u
#define MOD_COEFF_Y     0x4cu
#define MOD_OFFSET_X    0x50u
#define MOD_OFFSET_Y    0x54u

#define EFFECT_COUNT 41u

typedef struct {
    uint8_t usb_gain;
    uint8_t usb_direction;
    uint16_t sample_period;
    uint16_t valid_mask;
    uint16_t last[16];
} effect_state_t;

static effect_state_t es[EFFECT_COUNT];
static uint16_t global_valid;
static uint16_t global_last[16];

static inline uint16_t join16(const uint8_t *p) {
    return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}

void ffb_engine_init(void) {
    global_valid=0u;
    for (uint32_t j=0;j<16u;++j) global_last[j]=0u;
    for (uint32_t i=0;i<EFFECT_COUNT;++i) {
        es[i].usb_gain=255u;
        es[i].usb_direction=0u;
        es[i].sample_period=0u;
        es[i].valid_mask=0u;
        for (uint32_t j=0;j<16u;++j) es[i].last[j]=0u;
    }
}

static inline uint8_t addr_index(uint8_t address) {
    if (address < 0x40u || address > 0x7cu || (address & 3u)) return 0xffu;
    return (uint8_t)((address-0x40u)>>2);
}

static bool queue_msg(const uint8_t *m, uint32_t n) {
    return ffb_transport_enqueue(m,n);
}

/* A3/A14 engine packed representation: low 7 bits live in bits 0..6 and
 * the high MIDI byte lives in bits 8..14. */
static bool modify_cached_packed(uint8_t id,uint8_t address,uint16_t packed) {
    uint8_t ix=addr_index(address);
    if(ix==0xffu) return false;
    uint16_t *valid;
    uint16_t *last;
    if(id==0x7fu){ valid=&global_valid; last=global_last; }
    else {
        if(id>=EFFECT_COUNT) return false;
        valid=&es[id].valid_mask; last=es[id].last;
    }
    uint16_t bit=(uint16_t)(1u<<ix);
    if((*valid&bit) && last[ix]==packed) return true;
    if(ffb_transport_occupancy()>3072u) return false;
    uint8_t m[6]={0xb5u,address,(uint8_t)(id&0x7fu),0xa5u,
                  (uint8_t)(packed&0x7fu),(uint8_t)((packed>>8)&0x7fu)};
    if(!queue_msg(m,6u)) return false;
    last[ix]=packed; *valid|=bit;
    return true;
}

/* A10/A15 condition path: a conventional 14-bit integer split at bit 7. */
static void modify_raw14(uint8_t id,uint8_t param,uint16_t value) {
    uint8_t m[6]={0xb5u,param,(uint8_t)(id&0x7fu),0xa5u,
                  (uint8_t)(value&0x7fu),(uint8_t)((value>>7)&0x7fu)};
    queue_msg(m,6u);
}

static void op3(uint8_t op,uint8_t id){uint8_t m[3]={0xb5u,op,(uint8_t)(id&0x7fu)};queue_msg(m,3u);}
static void ctrl2(uint8_t c){uint8_t m[2]={0xc5u,c};queue_msg(m,2u);}

static inline uint16_t usb_u16_to_midi14(uint16_t v){
    if(v==0xffffu)return 0u;
    return (uint16_t)(((v<<1)&0x7f00u)|(v&0x007fu));
}
static inline uint16_t usb_time_to_midi14(uint16_t v){
    if(v==0xffffu)return 0u;
    return (uint16_t)((v&0x7f00u)|((v&0x00ffu)>>1));
}
static inline uint16_t usb_s8_to_midi14(int8_t v){
    int16_t t=v; if(t<0)t=(int16_t)(t+0x7f80); return (uint16_t)t;
}
static inline uint16_t convert_direction(uint8_t usbdir,uint8_t reciprocal){
    uint16_t d=(uint16_t)usbdir*2u;
    if(reciprocal){d=(uint16_t)(d+180u);if(d>=360u)d=(uint16_t)(d-360u);}
    return (uint16_t)((d&0x007fu)|((d&0x0180u)<<1));
}

static void set_effect(const uint8_t*b,uint16_t n){
    if(n<13u)return;
    uint8_t id=b[0]; if(id>=EFFECT_COUNT)return;
    uint8_t usb_type=b[1];
    uint16_t duration=join16(&b[2]);

    /* A10/A15 final condition-effect semantics: for USB Spring/Damper/
     * Inertia/Friction, SetEffect updates duration only. Strength and center
     * are taken from SetCondition below. */
    if(usb_type>=8u && usb_type<=11u){
        uint16_t md=(duration==0xffffu)?0u:(uint16_t)(duration>>1);
        if(md>0x3fffu)md=0x3fffu;
        modify_raw14(id,MOD_DURATION,md);
        return;
    }

    uint16_t sample_period=join16(&b[6]);
    uint8_t gain=b[8],flags=b[10],dir=b[11];
    es[id].usb_gain=gain; es[id].usb_direction=dir; es[id].sample_period=sample_period;
    modify_cached_packed(id,MOD_DURATION,usb_time_to_midi14(duration));

    MidiEffectType type=ffb_midi_effect_type(id);
    if(type==MIDI_ET_CONSTANT||type==MIDI_ET_SINE||type==MIDI_ET_SQUARE||
       type==MIDI_ET_RAMP||type==MIDI_ET_TRIANGLE||type==MIDI_ET_SAWTOOTHDOWN||
       type==MIDI_ET_SAWTOOTHUP){
        /* A14: descriptor already supplies gain 0..127, so do NOT halve again. */
        modify_cached_packed(id,MOD_GAIN,(uint16_t)(gain&0x7fu));
        if(flags&0x04u)modify_cached_packed(id,MOD_DIRECTION,convert_direction(dir,0u));
        if(sample_period){
            uint32_t sr=1000u/sample_period;if(sr<1u)sr=1u;if(sr>127u)sr=127u;
            modify_cached_packed(id,MOD_SAMPLE_RATE,usb_u16_to_midi14((uint16_t)sr));
        }
    }
}

static void set_envelope(const uint8_t*b,uint16_t n){
    if(n<7u)return;uint8_t id=b[0];
    modify_cached_packed(id,MOD_ATTACK,(uint16_t)(b[1]>>1));
    modify_cached_packed(id,MOD_FADE,(uint16_t)(b[2]>>1));
    modify_cached_packed(id,MOD_ATTACK_TIME,usb_time_to_midi14(join16(&b[3])));
    modify_cached_packed(id,MOD_FADE_TIME,usb_time_to_midi14(join16(&b[5])));
}

static void set_condition(const uint8_t*b,uint16_t n){
    if(n<8u)return;
    uint8_t id=b[0];
    uint8_t axis=(uint8_t)(b[1]&0x0fu);
    int8_t center=(int8_t)b[2];
    uint8_t strength=(uint8_t)(b[3]>>1);

    if(id<EFFECT_COUNT && ffb_midi_effect_type(id)==MIDI_ET_SPRING)
        strength=spring_curve_A21[strength];

    uint16_t offset=(uint16_t)(int16_t)center;
    if(axis==0u){
        modify_raw14(id,MOD_OFFSET_X,offset);
        modify_raw14(id,MOD_COEFF_X,strength);
    }else if(axis==1u){
        modify_raw14(id,MOD_OFFSET_Y,offset);
        modify_raw14(id,MOD_COEFF_Y,strength);
    }
}

static uint8_t periodic_range_and_params(uint8_t id,int8_t off){
    int16_t p1,p2;
    if(off>=0){p1=127;p2=(int16_t)-128+(int16_t)off*2;}
    else{p1=(int16_t)127+((int16_t)off+1)*2;p2=-128;}
    int16_t range=p1-p2;if(range<1)range=1;if(range>255)range=255;
    modify_cached_packed(id,MOD_PARAM1,usb_s8_to_midi14((int8_t)p1));
    modify_cached_packed(id,MOD_PARAM2,usb_s8_to_midi14((int8_t)p2));
    return (uint8_t)range;
}
static uint8_t scale_level_for_range(uint8_t range,uint8_t level){
    uint32_t v=((uint32_t)level*255u)/(uint32_t)(range?range:1u);
    if(v>255u)return 127u;return (uint8_t)((v>>1)&0x7fu);
}
static void set_periodic(const uint8_t*b,uint16_t n){
    if(n<6u)return;uint8_t id=b[0],mag=b[1];int8_t off=(int8_t)b[2];uint16_t period=join16(&b[4]);
    uint16_t freq=1u;if(period<=13u)freq=77u;else if(period<1000u&&period)freq=(uint16_t)(((2000u/period)+1u)>>1);
    modify_cached_packed(id,MOD_FREQUENCY,usb_u16_to_midi14(freq));
    uint16_t sr=100u;
    if(id<EFFECT_COUNT && es[id].sample_period){uint32_t t=1000u/es[id].sample_period;if(t<1u)t=1u;if(t>127u)t=127u;sr=(uint16_t)t;}
    else if(freq>25u){sr=(uint16_t)(freq*4u);if(sr>127u)sr=127u;}
    modify_cached_packed(id,MOD_SAMPLE_RATE,usb_u16_to_midi14(sr));
    uint8_t range=periodic_range_and_params(id,off);
    modify_cached_packed(id,MOD_MAGNITUDE,(uint16_t)scale_level_for_range(range,mag));
}
static void set_constant(const uint8_t*b,uint16_t n){
    if(n<3u)return;uint8_t id=b[0];if(id>=EFFECT_COUNT)return;int16_t m=(int16_t)join16(&b[1]);
    uint16_t am=(m>=0)?(uint16_t)m:(uint16_t)(-(m+1));
    modify_cached_packed(id,MOD_MAGNITUDE,(uint16_t)((am>>1)&0x7fu));
    modify_cached_packed(id,MOD_DIRECTION,convert_direction(es[id].usb_direction,(uint8_t)(m<0)));
    modify_cached_packed(id,MOD_PARAM1,0x007fu);modify_cached_packed(id,MOD_PARAM2,0x0000u);
}
static void set_ramp(const uint8_t*b,uint16_t n){
    if(n<3u)return;uint8_t id=b[0];int8_t s=(int8_t)b[1],e=(int8_t)b[2];
    int8_t off=(int8_t)(((int16_t)s+(int16_t)e)/2);uint8_t mag=(s>e)?(uint8_t)((int16_t)s-e):(uint8_t)((int16_t)e-s);
    uint8_t range=periodic_range_and_params(id,off);modify_cached_packed(id,MOD_MAGNITUDE,(uint16_t)scale_level_for_range(range,mag));
}
static void effect_operation(const uint8_t*b,uint16_t n){if(n<2u)return;uint8_t id=b[0],op=b[1];if(op==1u)op3(0x20u,id);else if(op==2u)op3(0x00u,id);else if(op==3u)op3(0x30u,id);}
static void block_free(const uint8_t*b,uint16_t n){if(n<1u)return;uint8_t id=b[0];op3(0x10u,id);if(id<EFFECT_COUNT){es[id].valid_mask=0u;es[id].usb_gain=255u;}ffb_midi_mark_free(id);}
static void device_control(const uint8_t*b,uint16_t n){
    if(n<1u)return;uint8_t c=b[0];
    if(c==1u)ctrl2(0x02u);
    else if(c==2u)ctrl2(0x03u);
    else if(c==3u)ctrl2(0x06u);
    else if(c==4u){
        /* A2 final reset fix. A12 transport recognizes C5 01 and inserts the
         * 75 ms asynchronous guard before allowing the queued C5 06 through. */
        ctrl2(0x01u);ctrl2(0x06u);
        for(uint32_t i=0;i<EFFECT_COUNT;++i){es[i].valid_mask=0u;es[i].usb_gain=255u;}
        global_valid=0u;ffb_midi_reset_allocator();
    }else if(c==5u)ctrl2(0x05u);
    else if(c==6u)ctrl2(0x04u);
}
static void device_gain(const uint8_t*b,uint16_t n){if(n<1u)return;modify_cached_packed(0x7fu,MOD_DEVICE_GAIN,(uint16_t)((b[0]>>1)&0x7fu));}

void ffb_engine_handle_output(uint8_t report_id,const uint8_t*buffer,uint16_t n){
    if(!buffer)return;
    switch(report_id){
        case RID_SET_EFFECT:set_effect(buffer,n);break;
        case RID_SET_ENVELOPE:set_envelope(buffer,n);break;
        case RID_SET_CONDITION:set_condition(buffer,n);break;
        case RID_SET_PERIODIC:set_periodic(buffer,n);break;
        case RID_SET_CONSTANT:set_constant(buffer,n);break;
        case RID_SET_RAMP:set_ramp(buffer,n);break;
        case RID_EFFECT_OPERATION:effect_operation(buffer,n);break;
        case RID_BLOCK_FREE:block_free(buffer,n);break;
        case RID_DEVICE_CONTROL:device_control(buffer,n);break;
        case RID_DEVICE_GAIN:device_gain(buffer,n);break;
        default:break;
    }
}
