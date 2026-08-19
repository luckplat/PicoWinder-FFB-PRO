#ifndef FFB_ENGINE_H
#define FFB_ENGINE_H
#include <stdint.h>
void ffb_engine_init(void);
void ffb_engine_handle_output(uint8_t report_id, const uint8_t *buffer, uint16_t len);
#endif
