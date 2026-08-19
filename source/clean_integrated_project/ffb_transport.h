#ifndef FFB_TRANSPORT_H
#define FFB_TRANSPORT_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

void ffb_transport_init(void);
bool ffb_transport_enqueue(const uint8_t *data, size_t len);
void ffb_transport_task(void);
uint16_t ffb_transport_occupancy(void);
uint32_t ffb_transport_dropped(void);

#endif
