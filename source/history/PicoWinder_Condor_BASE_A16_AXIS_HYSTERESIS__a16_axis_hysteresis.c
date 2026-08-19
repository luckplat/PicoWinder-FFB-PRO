#include <stdint.h>
#include <stdbool.h>

typedef bool (*hid_report_fn)(uint8_t,uint8_t,void const*,uint16_t);

#define HSTATE_ADDR 0x20012e00u
#define H_MAGIC      0xA160A516u
#define HYST         4u

struct HState {
    uint32_t magic;
    uint16_t x;
    uint16_t y;
};
#define HS ((volatile struct HState*)HSTATE_ADDR)

static inline uint16_t rd16(const uint8_t *p) {
    return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}
static inline void wr16(uint8_t *p,uint16_t v) {
    p[0]=(uint8_t)(v & 0xffu);
    p[1]=(uint8_t)(v >> 8);
}

static inline uint16_t track_hysteresis(uint16_t raw,uint16_t filtered) {
    if (raw > (uint16_t)(filtered + HYST)) {
        return (uint16_t)(raw - HYST);
    }
    if ((uint16_t)(raw + HYST) < filtered) {
        return (uint16_t)(raw + HYST);
    }
    return filtered;
}

__attribute__((used,noinline))
bool a16_hid_report(uint8_t instance,uint8_t report_id,
                    void const *report_ptr,uint16_t len) {
    if (report_id==1u && report_ptr && len>=6u) {
        uint8_t *p=(uint8_t*)(uintptr_t)report_ptr;
        uint16_t raw_x=rd16(p+2u);
        uint16_t raw_y=rd16(p+4u);

        if (HS->magic!=H_MAGIC || HS->x>1023u || HS->y>1023u) {
            HS->x=raw_x;
            HS->y=raw_y;
            HS->magic=H_MAGIC;
        } else {
            HS->x=track_hysteresis(raw_x,HS->x);
            HS->y=track_hysteresis(raw_y,HS->y);
        }

        wr16(p+2u,HS->x);
        wr16(p+4u,HS->y);
    }

    return ((hid_report_fn)0x10003a7du)(instance,report_id,report_ptr,len);
}
