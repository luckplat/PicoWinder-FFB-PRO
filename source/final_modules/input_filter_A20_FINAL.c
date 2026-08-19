/*
 * PicoWinder Condor FINAL A20 — axis refinement
 *
 * FFB is NOT modified by this module.
 *
 * X/Y:
 *   Keep the validated +/-4 tracking hysteresis.
 *
 * Slider/Throttle (0..127):
 *   Idle-lock + movement tracking:
 *     - idle band +/-3: completely held
 *     - medium displacement 4..7: must persist 250 reports
 *     - large displacement >=8: must persist 6 reports, then enter tracking
 *     - during tracking output follows RAW exactly
 *     - tracking exits after a 100-report window whose raw span <=3
 *
 * Twist / Rotation Z (0..63):
 *   - large center-zero dead zone: raw 24..38 -> output center 32
 *   - LUT remaps both sides so 0 -> 0 and 63 -> 63
 *   - slew limit output to one count per HID report
 *
 * No filtered value is ever written into the PIO IRQ-owned global report.
 */

#include <stdint.h>
#include <stdbool.h>

typedef bool (*hid_report_fn)(uint8_t,uint8_t,void const*,uint16_t);

#define HSTATE_ADDR 0x20012e00u
#define H_MAGIC     0xA200C020u

#define XY_HYST 4u

#define SL_IDLE_BAND      3u
#define SL_FAST_DELTA     8u
#define SL_FAST_PERSIST   6u
#define SL_MED_PERSIST    250u
#define SL_TRACK_WINDOW   100u
#define SL_STABLE_SPAN    3u

struct HState {
    uint32_t magic;
    uint16_t x;
    uint16_t y;
    uint8_t slider;
    uint8_t z;
    int8_t slider_dir;
    uint8_t slider_mode;
    uint16_t slider_count;
    uint16_t track_count;
    uint8_t track_min;
    uint8_t track_max;
};

#define HS ((volatile struct HState*)(uintptr_t)HSTATE_ADDR)

static const uint8_t z_map_lut[64] = {
    0, 1, 3, 4, 5, 7, 8, 9, 11, 12, 13, 15, 16, 18, 19, 20, 22, 23, 24, 26, 27, 28, 30, 31, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 33, 34, 36, 37, 38, 39, 41, 42, 43, 44, 46, 47, 48, 49, 51, 52, 53, 54, 56, 57, 58, 59, 61, 62, 63
};

static inline uint16_t load16(uintptr_t address) {
    return *(volatile uint16_t const*)address;
}

static inline uint16_t track_hyst16(uint16_t raw,uint16_t filtered,uint16_t h) {
    if (raw > (uint16_t)(filtered + h)) return (uint16_t)(raw - h);
    if ((uint16_t)(raw + h) < filtered) return (uint16_t)(raw + h);
    return filtered;
}

static inline void reset_slider_candidate(void) {
    HS->slider_dir=0;
    HS->slider_count=0;
}

static uint8_t filter_slider(uint8_t raw) {
    if (HS->slider_mode!=0u) {
        HS->slider=raw;

        if (HS->track_count==0u) {
            HS->track_min=raw;
            HS->track_max=raw;
            HS->track_count=1u;
        } else {
            if (raw<HS->track_min) HS->track_min=raw;
            if (raw>HS->track_max) HS->track_max=raw;
            if (HS->track_count<SL_TRACK_WINDOW) {
                HS->track_count=(uint16_t)(HS->track_count+1u);
            }
        }

        if (HS->track_count>=SL_TRACK_WINDOW) {
            uint8_t span=(uint8_t)(HS->track_max-HS->track_min);
            if (span<=SL_STABLE_SPAN) {
                HS->slider_mode=0u;
            }
            HS->track_count=0u;
            HS->track_min=raw;
            HS->track_max=raw;
        }

        reset_slider_candidate();
        return HS->slider;
    }

    int16_t delta=(int16_t)raw-(int16_t)HS->slider;
    uint16_t mag=(uint16_t)(delta<0 ? -delta : delta);

    if (mag<=SL_IDLE_BAND) {
        reset_slider_candidate();
        return HS->slider;
    }

    int8_t dir=(delta>0) ? 1 : -1;
    uint16_t required=(mag>=SL_FAST_DELTA) ?
        SL_FAST_PERSIST : SL_MED_PERSIST;

    if (dir!=HS->slider_dir) {
        HS->slider_dir=dir;
        HS->slider_count=1u;
    } else if (HS->slider_count<required) {
        HS->slider_count=(uint16_t)(HS->slider_count+1u);
    }

    if (HS->slider_count>=required) {
        HS->slider=raw;
        reset_slider_candidate();

        if (mag>=SL_FAST_DELTA) {
            HS->slider_mode=1u;
            HS->track_count=1u;
            HS->track_min=raw;
            HS->track_max=raw;
        }
    }

    return HS->slider;
}

static inline uint8_t map_z(uint8_t raw) {
    return z_map_lut[raw & 0x3fu];
}

static inline uint8_t slew1(uint8_t target,uint8_t current) {
    if (target>current) return (uint8_t)(current+1u);
    if (target<current) return (uint8_t)(current-1u);
    return current;
}

__attribute__((used,noinline))
bool final_a20_hid_report(uint8_t instance,uint8_t report_id,
                          void const *report_ptr,uint16_t len) {
    if (report_id==1u && report_ptr && len==9u) {
        uintptr_t a=(uintptr_t)report_ptr;
        volatile uint8_t const *p=(volatile uint8_t const*)a;

        uint16_t buttons=load16(a+0u);
        uint16_t raw_x=load16(a+2u);
        uint16_t raw_y=load16(a+4u);
        uint8_t raw_z=p[6u];
        uint8_t raw_slider=p[7u];
        uint8_t raw_hat=p[8u];

        if (HS->magic!=H_MAGIC ||
            HS->x>1023u || HS->y>1023u ||
            HS->slider>127u || HS->z>63u) {
            HS->x=raw_x;
            HS->y=raw_y;
            HS->slider=raw_slider;
            HS->z=map_z(raw_z);

            HS->slider_dir=0;
            HS->slider_mode=0u;
            HS->slider_count=0u;
            HS->track_count=0u;
            HS->track_min=raw_slider;
            HS->track_max=raw_slider;

            HS->magic=H_MAGIC;
        } else {
            HS->x=track_hyst16(raw_x,HS->x,XY_HYST);
            HS->y=track_hyst16(raw_y,HS->y,XY_HYST);
            HS->slider=filter_slider(raw_slider);
            HS->z=slew1(map_z(raw_z),HS->z);
        }

        uint8_t snapshot[9]={
            (uint8_t)buttons,
            (uint8_t)(buttons>>8),
            (uint8_t)HS->x,
            (uint8_t)(HS->x>>8),
            (uint8_t)HS->y,
            (uint8_t)(HS->y>>8),
            HS->z,
            HS->slider,
            raw_hat
        };

        return ((hid_report_fn)0x10003a7du)
            (instance,report_id,snapshot,len);
    }

    return ((hid_report_fn)0x10003a7du)
        (instance,report_id,report_ptr,len);
}
