#include "input_filter.h"

#include <stdint.h>

#define XY_HYST 4u
#define SL_IDLE_BAND 3u
#define SL_FAST_DELTA 8u
#define SL_FAST_PERSIST 6u
#define SL_MED_PERSIST 250u
#define SL_TRACK_WINDOW 100u
#define SL_STABLE_SPAN 3u

static const uint8_t z_map_lut[64]={
    0,1,3,4,5,7,8,9,11,12,13,15,16,18,19,20,
    22,23,24,26,27,28,30,31,32,32,32,32,32,32,32,32,
    32,32,32,32,32,32,32,33,34,36,37,38,39,41,42,43,
    44,46,47,48,49,51,52,53,54,56,57,58,59,61,62,63
};

typedef struct {
    uint8_t initialized;
    uint16_t x,y;
    uint8_t slider,z;
    int8_t slider_dir;
    uint8_t slider_mode;
    uint16_t slider_count,track_count;
    uint8_t track_min,track_max;
} filter_state_t;
static filter_state_t s;

void input_filter_reset(void){s.initialized=0u;}
static uint16_t track_hyst16(uint16_t raw,uint16_t f,uint16_t h){if(raw>(uint16_t)(f+h))return(uint16_t)(raw-h);if((uint16_t)(raw+h)<f)return(uint16_t)(raw+h);return f;}
static void reset_candidate(void){s.slider_dir=0;s.slider_count=0u;}
static uint8_t filter_slider(uint8_t raw){
    if(s.slider_mode){
        s.slider=raw;
        if(!s.track_count){s.track_min=s.track_max=raw;s.track_count=1u;}
        else{if(raw<s.track_min)s.track_min=raw;if(raw>s.track_max)s.track_max=raw;if(s.track_count<SL_TRACK_WINDOW)s.track_count++;}
        if(s.track_count>=SL_TRACK_WINDOW){if((uint8_t)(s.track_max-s.track_min)<=SL_STABLE_SPAN)s.slider_mode=0u;s.track_count=0u;s.track_min=s.track_max=raw;}
        reset_candidate();return s.slider;
    }
    int16_t d=(int16_t)raw-(int16_t)s.slider;uint16_t mag=(uint16_t)(d<0?-d:d);
    if(mag<=SL_IDLE_BAND){reset_candidate();return s.slider;}
    int8_t dir=d>0?1:-1;uint16_t req=mag>=SL_FAST_DELTA?SL_FAST_PERSIST:SL_MED_PERSIST;
    if(dir!=s.slider_dir){s.slider_dir=dir;s.slider_count=1u;}else if(s.slider_count<req)s.slider_count++;
    if(s.slider_count>=req){s.slider=raw;reset_candidate();if(mag>=SL_FAST_DELTA){s.slider_mode=1u;s.track_count=1u;s.track_min=s.track_max=raw;}}
    return s.slider;
}
static uint8_t map_z(uint8_t raw){return z_map_lut[raw&0x3fu];}
static uint8_t slew1(uint8_t target,uint8_t cur){if(target>cur)return(uint8_t)(cur+1u);if(target<cur)return(uint8_t)(cur-1u);return cur;}

void input_filter_make_snapshot(volatile const picowinder_report_t *raw,uint8_t out[9]){
    uint16_t buttons=raw->buttons,rx=raw->x,ry=raw->y;uint8_t rz=raw->twist,rs=raw->throttle,hat=raw->hat;
    if(!s.initialized || s.x>1023u || s.y>1023u || s.slider>127u || s.z>63u){
        s.x=rx;s.y=ry;s.slider=rs;s.z=map_z(rz);s.slider_dir=0;s.slider_mode=0u;s.slider_count=0u;s.track_count=0u;s.track_min=s.track_max=rs;s.initialized=1u;
    }else{
        s.x=track_hyst16(rx,s.x,XY_HYST);s.y=track_hyst16(ry,s.y,XY_HYST);s.slider=filter_slider(rs);s.z=slew1(map_z(rz),s.z);
    }
    out[0]=(uint8_t)buttons;out[1]=(uint8_t)(buttons>>8);out[2]=(uint8_t)s.x;out[3]=(uint8_t)(s.x>>8);out[4]=(uint8_t)s.y;out[5]=(uint8_t)(s.y>>8);out[6]=s.z;out[7]=s.slider;out[8]=hat;
}
