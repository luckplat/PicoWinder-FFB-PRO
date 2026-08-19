#include <stdint.h>

/* BASE23: output-only FFB engine for PicoWinder TEST10 anti-freeze platform.
 * The stock PicoWinder callback still handles FEATURE CreateNewEffect and all
 * GET_REPORT/BlockLoad/Pool behaviour. We intercept OUTPUT reports only.
 */

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

#define MIDI_NONE       0x00u
#define MIDI_SINE       0x02u
#define MIDI_SQUARE     0x05u
#define MIDI_RAMP       0x06u
#define MIDI_TRIANGLE   0x08u
#define MIDI_SAW_DOWN   0x0au
#define MIDI_SAW_UP     0x0bu
#define MIDI_SPRING     0x0du
#define MIDI_DAMPER     0x0eu
#define MIDI_INERTIA    0x0fu
#define MIDI_FRICTION   0x10u
#define MIDI_CONSTANT   0x12u

/* SideWinder FFP modify addresses, aligned with adapt-ffb-joy 0.5.0beta1. */
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

/* Known-good TEST10 queue writer and integer divide helper. */
typedef void (*queue_write_fn)(const uint8_t *buf, uint32_t len);
typedef uint32_t (*udiv_fn)(uint32_t a, uint32_t b);
static const queue_write_fn qwrite = (queue_write_fn)0x10005931u;
static const udiv_fn udiv32 = (udiv_fn)0x100022a1u;

/* Stock PicoWinder effect type table in RAM: enum MidiEffectType[41]. */
#define EFFECTS_ASSIGNED ((volatile uint32_t*)0x200016d0u)

/* TEST10 FIFO state: head:u16, tail:u16, dropped:u32, data[4096]. */
#define QSTATE_BASE ((volatile uint8_t*)0x20010000u)
static inline uint16_t q_head(void) { return *(volatile uint16_t*)(QSTATE_BASE + 0); }
static inline uint16_t q_tail(void) { return *(volatile uint16_t*)(QSTATE_BASE + 2); }
static inline uint32_t q_dropped(void) { return *(volatile uint32_t*)(QSTATE_BASE + 4); }
static inline uint16_t q_occupancy(void) { return (uint16_t)((q_head() - q_tail()) & 0x0fffu); }

/* Separate RAM, outside PicoWinder globals and TEST10 FIFO. */
#define STATE_ADDR 0x20012000u
#define STATE_MAGIC 0x42323331u /* B231 */

struct EffectState {
    uint8_t usb_gain;
    uint8_t usb_direction;
    int8_t coeff_x;
    int8_t coeff_y;
    uint16_t sample_period;
    uint16_t valid_mask;       /* cache for addresses 0x40..0x7c */
    uint16_t last[16];
};
struct EngineState {
    uint32_t magic;
    struct EffectState e[41];
    uint16_t global_valid;
    uint16_t global_last[16];
};
#define ST ((volatile struct EngineState*)STATE_ADDR)

static inline uint16_t join16(const uint8_t *p) {
    return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}

static void init_state(void) {
    if (ST->magic == STATE_MAGIC) return;
    ST->magic = STATE_MAGIC;
    ST->global_valid = 0u;
    for (uint32_t j=0;j<16;j++) ST->global_last[j]=0u;
    for (uint32_t i=0;i<41;i++) {
        ST->e[i].usb_gain=255u;
        ST->e[i].usb_direction=0u;
        ST->e[i].coeff_x=0;
        ST->e[i].coeff_y=0;
        ST->e[i].sample_period=0u;
        ST->e[i].valid_mask=0u;
        for (uint32_t j=0;j<16;j++) ST->e[i].last[j]=0u;
    }
}

static inline uint8_t addr_index(uint8_t address) {
    if (address < 0x40u || address > 0x7cu || (address & 3u)) return 0xffu;
    return (uint8_t)((address - 0x40u) >> 2);
}

static int queue_msg(const uint8_t *m, uint32_t n) {
    uint32_t before=q_dropped();
    qwrite(m,n);
    return q_dropped()==before;
}

/* Avoid runaway backlog. If the FIFO is already >75% full, skip streaming
 * modifications; the next DirectInput update will retry with a newer value.
 * Operations/control messages are still queued separately below. */
static int modify_cached(uint8_t id, uint8_t address, uint16_t packed) {
    uint8_t ix=addr_index(address);
    if (ix==0xffu) return 0;
    volatile uint16_t *valid;
    volatile uint16_t *last;
    if (id==0x7fu) { valid=&ST->global_valid; last=ST->global_last; }
    else {
        if (id>=41u) return 0;
        valid=&ST->e[id].valid_mask; last=ST->e[id].last;
    }
    uint16_t bit=(uint16_t)(1u<<ix);
    if ((*valid & bit) && last[ix]==packed) return 1;
    if (q_occupancy() > 3072u) return 0;
    uint8_t m[6]={0xb5u,address,(uint8_t)(id&0x7fu),0xa5u,
                  (uint8_t)(packed&0x7fu),(uint8_t)((packed>>8)&0x7fu)};
    if (!queue_msg(m,6u)) return 0;
    last[ix]=packed; *valid |= bit;
    return 1;
}

static inline void op3(uint8_t address,uint8_t id) {
    uint8_t m[3]={0xb5u,address,(uint8_t)(id&0x7fu)};
    queue_msg(m,3u);
}
static inline void ctrl2(uint8_t command) {
    uint8_t m[2]={0xc5u,command};
    queue_msg(m,2u);
}

static inline uint16_t usb_u16_to_midi14(uint16_t v) {
    if (v==0xffffu) return 0u;
    return (uint16_t)(((v<<1)&0x7f00u)|(v&0x007fu));
}
static inline uint16_t usb_time_to_midi14(uint16_t v) {
    if (v==0xffffu) return 0u;
    return (uint16_t)((v&0x7f00u)|((v&0x00ffu)>>1));
}
static inline uint16_t usb_s8_to_midi14(int8_t v) {
    int16_t t=v;
    if (t<0) t=(int16_t)(t+0x7f80);
    return (uint16_t)t;
}
static inline int8_t gain_coeff(int8_t v,uint8_t gain) {
    int32_t n=(int32_t)v*(int32_t)gain;
    if (n>=0) return (int8_t)udiv32((uint32_t)n,255u);
    return (int8_t)(-(int32_t)udiv32((uint32_t)(-n),255u));
}
static inline uint16_t convert_direction(uint8_t usbdir,uint8_t reciprocal) {
    uint16_t d=(uint16_t)usbdir*2u;
    if (reciprocal) { d=(uint16_t)(d+180u); if(d>=360u)d=(uint16_t)(d-360u); }
    return (uint16_t)((d&0x007fu)|((d&0x0180u)<<1));
}

static void reapply_condition_gain(uint8_t id) {
    if(id>=41u)return;
    uint32_t type=EFFECTS_ASSIGNED[id];
    if(type!=MIDI_SPRING&&type!=MIDI_DAMPER&&type!=MIDI_INERTIA&&type!=MIDI_FRICTION)return;
    modify_cached(id,MOD_COEFF_X,usb_s8_to_midi14(gain_coeff(ST->e[id].coeff_x,ST->e[id].usb_gain)));
    modify_cached(id,MOD_COEFF_Y,usb_s8_to_midi14(gain_coeff(ST->e[id].coeff_y,ST->e[id].usb_gain)));
}

static void set_effect(const uint8_t *b,uint16_t n) {
    if(n<13u)return;
    uint8_t id=b[0]; if(id>=41u)return;
    uint16_t duration=join16(&b[2]);
    uint16_t sample_period=join16(&b[6]);
    uint8_t gain=b[8], flags=b[10], dir=b[11];
    ST->e[id].usb_gain=gain; ST->e[id].usb_direction=dir; ST->e[id].sample_period=sample_period;
    modify_cached(id,MOD_DURATION,usb_time_to_midi14(duration));
    uint32_t type=EFFECTS_ASSIGNED[id];
    if(type==MIDI_SPRING||type==MIDI_DAMPER||type==MIDI_INERTIA||type==MIDI_FRICTION) {
        reapply_condition_gain(id); return;
    }
    if(type==MIDI_CONSTANT||type==MIDI_SINE||type==MIDI_SQUARE||type==MIDI_RAMP||type==MIDI_TRIANGLE||type==MIDI_SAW_DOWN||type==MIDI_SAW_UP) {
        modify_cached(id,MOD_GAIN,(uint16_t)((gain>>1)&0x7fu));
        if(flags&0x04u) modify_cached(id,MOD_DIRECTION,convert_direction(dir,0u));
        if(sample_period!=0u) {
            uint32_t sr=udiv32(1000u,sample_period); if(sr<1u)sr=1u; if(sr>127u)sr=127u;
            modify_cached(id,MOD_SAMPLE_RATE,usb_u16_to_midi14((uint16_t)sr));
        }
    }
}

static void set_envelope(const uint8_t*b,uint16_t n) {
    if(n<7u)return; uint8_t id=b[0];
    modify_cached(id,MOD_ATTACK,(uint16_t)(b[1]>>1));
    modify_cached(id,MOD_FADE,(uint16_t)(b[2]>>1));
    modify_cached(id,MOD_ATTACK_TIME,usb_time_to_midi14(join16(&b[3])));
    modify_cached(id,MOD_FADE_TIME,usb_time_to_midi14(join16(&b[5])));
}

static void set_condition(const uint8_t*b,uint16_t n) {
    if(n<8u)return; uint8_t id=b[0]; if(id>=41u)return;
    uint8_t axis=(uint8_t)(b[1]&0x0fu); int8_t cp=(int8_t)b[2];
    int8_t coeff_usb=(int8_t)b[3];
    int8_t coeff=gain_coeff(coeff_usb,ST->e[id].usb_gain);
    uint32_t type=EFFECTS_ASSIGNED[id];
    if(axis==0u) {
        ST->e[id].coeff_x=coeff_usb;
        modify_cached(id,MOD_COEFF_X,usb_s8_to_midi14(coeff));
        if(type!=MIDI_FRICTION) modify_cached(id,MOD_OFFSET_X,usb_s8_to_midi14(cp));
    } else if(axis==1u) {
        ST->e[id].coeff_y=coeff_usb;
        modify_cached(id,MOD_COEFF_Y,usb_s8_to_midi14(coeff));
        if(type!=MIDI_FRICTION) {
            uint16_t yoff=((uint8_t)cp==0x80u)?0x007fu:usb_s8_to_midi14((int8_t)(-cp));
            modify_cached(id,MOD_OFFSET_Y,yoff);
        }
    }
}

static uint8_t periodic_range_and_params(uint8_t id,int8_t off) {
    int16_t p1,p2;
    if(off>=0){p1=127;p2=(int16_t)-128+(int16_t)off*2;}
    else {p1=(int16_t)127+((int16_t)off+1)*2;p2=-128;}
    int16_t range=p1-p2; if(range<1)range=1;if(range>255)range=255;
    modify_cached(id,MOD_PARAM1,usb_s8_to_midi14((int8_t)p1));
    modify_cached(id,MOD_PARAM2,usb_s8_to_midi14((int8_t)p2));
    return (uint8_t)range;
}
static uint8_t scale_level_for_range(uint8_t range,uint8_t level) {
    uint32_t v=udiv32((uint32_t)level*255u,(uint32_t)(range?range:1u));
    if(v>255u)return 127u; return (uint8_t)((v>>1)&0x7fu);
}
static void set_periodic(const uint8_t*b,uint16_t n) {
    if(n<6u)return; uint8_t id=b[0],mag=b[1];int8_t off=(int8_t)b[2];uint16_t period=join16(&b[4]);
    uint16_t freq=1u; if(period<=13u)freq=77u;else if(period<1000u&&period)freq=(uint16_t)((udiv32(2000u,period)+1u)>>1);
    modify_cached(id,MOD_FREQUENCY,usb_u16_to_midi14(freq));
    uint16_t sr=100u;
    if(id<41u && ST->e[id].sample_period!=0u){uint32_t t=udiv32(1000u,ST->e[id].sample_period);if(t<1)t=1;if(t>127)t=127;sr=(uint16_t)t;}
    else if(freq>25u){sr=(uint16_t)(freq*4u);if(sr>127u)sr=127u;}
    modify_cached(id,MOD_SAMPLE_RATE,usb_u16_to_midi14(sr));
    uint8_t range=periodic_range_and_params(id,off);
    modify_cached(id,MOD_MAGNITUDE,(uint16_t)scale_level_for_range(range,mag));
}
static void set_constant(const uint8_t*b,uint16_t n) {
    if(n<3u)return;uint8_t id=b[0];if(id>=41u)return;int16_t m=(int16_t)join16(&b[1]);
    uint16_t am=(m>=0)?(uint16_t)m:(uint16_t)(-(m+1));
    modify_cached(id,MOD_MAGNITUDE,(uint16_t)((am>>1)&0x7fu));
    modify_cached(id,MOD_DIRECTION,convert_direction(ST->e[id].usb_direction,(uint8_t)(m<0)));
    modify_cached(id,MOD_PARAM1,0x007fu);modify_cached(id,MOD_PARAM2,0x0000u);
}
static void set_ramp(const uint8_t*b,uint16_t n) {
    if(n<3u)return;uint8_t id=b[0];int8_t s=(int8_t)b[1],e=(int8_t)b[2];
    int8_t off=(int8_t)(((int16_t)s+(int16_t)e)/2);uint8_t mag=(s>e)?(uint8_t)((int16_t)s-e):(uint8_t)((int16_t)e-s);
    uint8_t range=periodic_range_and_params(id,off);modify_cached(id,MOD_MAGNITUDE,(uint16_t)scale_level_for_range(range,mag));
}
static void effect_operation(const uint8_t*b,uint16_t n) {
    if(n<2u)return;uint8_t id=b[0],op=b[1];if(op==1u)op3(0x20u,id);else if(op==2u)op3(0x00u,id);else if(op==3u)op3(0x30u,id);
}
static void block_free(const uint8_t*b,uint16_t n) {
    if(n<1u)return;uint8_t id=b[0];op3(0x10u,id);if(id<41u){ST->e[id].valid_mask=0u;ST->e[id].usb_gain=255u;ST->e[id].coeff_x=ST->e[id].coeff_y=0;}
    /* Stock PicoWinder callback is bypassed for OUTPUT, so keep allocator state in sync. */
    if(id<41u)EFFECTS_ASSIGNED[id]=MIDI_NONE;
}
static void device_control(const uint8_t*b,uint16_t n) {
    if(n<1u)return;uint8_t c=b[0];
    /* Exact FFB Pro mapping from adapt-ffb-joy. */
    if(c==1u)ctrl2(0x02u);          /* enable actuators */
    else if(c==2u)ctrl2(0x03u);     /* disable */
    else if(c==3u)ctrl2(0x06u);     /* stop all incl autocentre */
    else if(c==4u){                 /* reset: real FFP reset + local cache invalidation */
        ctrl2(0x01u);
        ctrl2(0x06u); /* A2: Reset re-enables native autocentre; force it OFF again. */
        for(uint32_t i=0;i<41;i++){ST->e[i].valid_mask=0u;if(i>=2u)EFFECTS_ASSIGNED[i]=MIDI_NONE;}
        ST->global_valid=0u;
    }
    else if(c==5u)ctrl2(0x05u);     /* pause */
    else if(c==6u)ctrl2(0x04u);     /* continue */
}
static void device_gain(const uint8_t*b,uint16_t n){if(n<1u)return;modify_cached(0x7fu,MOD_DEVICE_GAIN,(uint16_t)((b[0]>>1)&0x7fu));}

__attribute__((noinline))
void base23_output_handler(uint8_t report_id,const uint8_t*buffer,uint16_t n){
    init_state();if(!buffer)return;
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

/* We enter here by BRANCH from inside stock tud_hid_set_report_cb(), after its
 * prologue. r1=report_id, r4=buffer, r5=bufsize. After processing, branch back
 * to stock callback epilogue at 0x10000332 (TEST10 intentionally skips echo). */
__attribute__((naked,used,section(".text.entry")))
void base23_output_entry(void){
    __asm volatile(
        "mov r0, r1\n"
        "mov r1, r4\n"
        "mov r2, r5\n"
        "bl base23_output_handler\n"
        "ldr r3, =0x10000333\n"
        "bx r3\n"
    );
}
