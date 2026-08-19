#include <stdint.h>
#include <stdbool.h>

typedef bool (*hid_report_fn)(uint8_t,uint8_t,void const*,uint16_t);

#define HSTATE_ADDR 0x20012e00u
#define H_MAGIC      0xA170A517u
#define HYST_XY      4u
#define HYST_SLIDER  1u

struct HState {
    uint32_t magic;
    uint16_t x;
    uint16_t y;
    uint8_t slider;
    uint8_t pad;
};
#define HS ((volatile struct HState*)HSTATE_ADDR)

static inline uint16_t rd16(const uint8_t *p) {
    return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}
static inline void wr16(uint8_t *p,uint16_t v) {
    p[0]=(uint8_t)(v & 0xffu);
    p[1]=(uint8_t)(v >> 8);
}

static inline uint16_t track_hyst16(uint16_t raw,uint16_t filtered,uint16_t h) {
    if (raw > (uint16_t)(filtered + h)) return (uint16_t)(raw - h);
    if ((uint16_t)(raw + h) < filtered) return (uint16_t)(raw + h);
    return filtered;
}

static inline uint8_t track_hyst8(uint8_t raw,uint8_t filtered,uint8_t h) {
    if ((uint16_t)raw > (uint16_t)filtered + h) return (uint8_t)(raw - h);
    if ((uint16_t)raw + h < (uint16_t)filtered) return (uint8_t)(raw + h);
    return filtered;
}

__attribute__((used,noinline))
bool a17_hid_report(uint8_t instance,uint8_t report_id,
                    void const *report_ptr,uint16_t len) {
    if (report_id==1u && report_ptr && len>=9u) {
        uint8_t *p=(uint8_t*)(uintptr_t)report_ptr;

        uint16_t raw_x=rd16(p+2u);
        uint16_t raw_y=rd16(p+4u);
        uint8_t raw_slider=p[7u];

        if (HS->magic!=H_MAGIC || HS->x>1023u || HS->y>1023u ||
            HS->slider>127u) {
            HS->x=raw_x;
            HS->y=raw_y;
            HS->slider=raw_slider;
            HS->magic=H_MAGIC;
        } else {
            HS->x=track_hyst16(raw_x,HS->x,HYST_XY);
            HS->y=track_hyst16(raw_y,HS->y,HYST_XY);
            HS->slider=track_hyst8(raw_slider,HS->slider,HYST_SLIDER);
        }

        wr16(p+2u,HS->x);
        wr16(p+4u,HS->y);
        p[7u]=HS->slider;
    }

    return ((hid_report_fn)0x10003a7du)(instance,report_id,report_ptr,len);
}
