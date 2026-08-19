#ifndef INPUT_FILTER_H
#define INPUT_FILTER_H
#include <stdint.h>
#include "joystick_report.h"
void input_filter_reset(void);
void input_filter_make_snapshot(volatile const picowinder_report_t *raw, uint8_t out[9]);
#endif
