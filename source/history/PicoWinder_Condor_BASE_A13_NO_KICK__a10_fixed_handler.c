#include <stdint.h>

typedef void (*handler_fn)(uint8_t,const uint8_t*,uint16_t);
typedef void (*qwrite_fn)(const uint8_t*,uint32_t);

static void modify_raw(uint8_t id,uint8_t param,uint16_t value){
    uint8_t m[6];
    m[0]=0xb5u; m[1]=param; m[2]=(uint8_t)(id&0x7fu); m[3]=0xa5u;
    m[4]=(uint8_t)(value&0x7fu);
    m[5]=(uint8_t)((value>>7)&0x7fu);
    ((qwrite_fn)0x10005931u)(m,6u);
}

__attribute__((used,noinline))
void a10_handler(uint8_t report_id,const uint8_t* b,uint16_t n){
    if(!b) return;

    /*
     * Condition effects: deliberately use the public stock PicoWinder
     * semantics that A8 was intended to test.
     */
    if(report_id==4u){
        if(n<8u) return;
        uint8_t id=b[0];
        uint8_t axis=(uint8_t)(b[1]&0x0fu);
        int8_t center=(int8_t)b[2];
        uint8_t strength=(uint8_t)(b[3]>>1);
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
     * For USB condition effect types 8..11, stock PicoWinder SetEffect
     * applies duration but not condition gain/direction updates.
     */
    if(report_id==2u && n>=4u){
        uint8_t usb_type=b[1];
        if(usb_type>=8u && usb_type<=11u){
            uint16_t duration=(uint16_t)b[2] | ((uint16_t)b[3]<<8);
            uint16_t midi_duration=(duration==0xffffu)?0u:(uint16_t)(duration>>1);
            if(midi_duration>0x3fffu) midi_duration=0x3fffu;
            modify_raw(b[0],0x40u,midi_duration);
            return;
        }
    }

    /* All other OUTPUT reports: exact A7/A1 handler. */
    ((handler_fn)0x10005c15u)(report_id,b,n);
}
