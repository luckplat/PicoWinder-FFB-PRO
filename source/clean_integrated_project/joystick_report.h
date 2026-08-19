#ifndef JOYSTICK_REPORT_H
#define JOYSTICK_REPORT_H
#include <stdint.h>

typedef struct {
    uint16_t buttons;
    uint16_t x;
    uint16_t y;
    uint8_t twist;
    uint8_t throttle;
    uint8_t hat;
} picowinder_report_t;

#define PICOWINDER_REPORT_SIZE 9u
#endif
