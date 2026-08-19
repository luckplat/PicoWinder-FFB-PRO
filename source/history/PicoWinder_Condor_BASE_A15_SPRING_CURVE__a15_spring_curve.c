#include <stdint.h>

typedef void (*handler_fn)(uint8_t,const uint8_t*,uint16_t);
typedef void (*qwrite_fn)(const uint8_t*,uint32_t);

/* Correct byte ABI confirmed in A3. */
#define EFFECTS_ASSIGNED ((volatile uint8_t*)0x200016d0u)
#define MIDI_SPRING 0x0du

/*
 * A15 spring transfer curve:
 * new_strength = round(old_strength^2 / 127)
 *
 * old_strength is already the stock PicoWinder value:
 * positiveCoefficient >> 1, range 0..127.
 *
 * LUT avoids any runtime integer division and is fully deterministic.
 */
static const uint8_t spring_curve[128] = {
    0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 2, 2,
    2, 2, 3, 3, 3, 3, 4, 4, 5, 5, 5, 6, 6, 7, 7, 8,
    8, 9, 9, 10, 10, 11, 11, 12, 13, 13, 14, 15, 15, 16, 17, 17,
    18, 19, 20, 20, 21, 22, 23, 24, 25, 26, 26, 27, 28, 29, 30, 31,
    32, 33, 34, 35, 36, 37, 39, 40, 41, 42, 43, 44, 45, 47, 48, 49,
    50, 52, 53, 54, 56, 57, 58, 60, 61, 62, 64, 65, 67, 68, 70, 71,
    73, 74, 76, 77, 79, 80, 82, 84, 85, 87, 88, 90, 92, 94, 95, 97,
    99, 101, 102, 104, 106, 108, 110, 112, 113, 115, 117, 119, 121, 123, 125, 127,
};

static void modify_raw(uint8_t id,uint8_t param,uint16_t value){
    uint8_t m[6]={
        0xb5u,param,(uint8_t)(id&0x7fu),0xa5u,
        (uint8_t)(value&0x7fu),(uint8_t)((value>>7)&0x7fu)
    };
    ((qwrite_fn)0x10005931u)(m,6u);
}

__attribute__((used,noinline))
void a15_handler(uint8_t report_id,const uint8_t* b,uint16_t n){
    if(!b) return;

    /*
     * Same SetCondition semantics as the tested A14/A10 handler:
     * - center offset unchanged
     * - base strength = positiveCoefficient >> 1
     *
     * A15 changes ONLY MIDI_SPRING slots by applying spring_curve[].
     * Damper/Inertia/Friction stay exactly at A14 strength.
     */
    if(report_id==4u){
        if(n<8u) return;

        uint8_t id=b[0];
        uint8_t axis=(uint8_t)(b[1]&0x0fu);
        int8_t center=(int8_t)b[2];
        uint8_t strength=(uint8_t)(b[3]>>1);

        if(id<41u && EFFECTS_ASSIGNED[id]==MIDI_SPRING){
            strength=spring_curve[strength];
        }

        uint16_t offset=(uint16_t)(int16_t)center;

        if(axis==0u){
            modify_raw(id,0x50u,offset);   /* OFFSET_X */
            modify_raw(id,0x48u,strength); /* STRENGTH_X */
        }else if(axis==1u){
            modify_raw(id,0x54u,offset);   /* OFFSET_Y */
            modify_raw(id,0x4cu,strength); /* STRENGTH_Y */
        }
        return;
    }

    /*
     * Same condition-effect SetEffect path as A14/A10.
     * Only duration is applied here.
     */
    if(report_id==2u && n>=4u){
        uint8_t usb_type=b[1];
        if(usb_type>=8u && usb_type<=11u){
            uint16_t duration=(uint16_t)b[2] | ((uint16_t)b[3]<<8);
            uint16_t midi_duration=
                (duration==0xffffu)?0u:(uint16_t)(duration>>1);
            if(midi_duration>0x3fffu) midi_duration=0x3fffu;
            modify_raw(b[0],0x40u,midi_duration);
            return;
        }
    }

    /*
     * Everything else goes to the exact underlying A14 engine.
     * This preserves A14's periodic gain fix at 0x10006018.
     */
    ((handler_fn)0x10005c15u)(report_id,b,n);
}
