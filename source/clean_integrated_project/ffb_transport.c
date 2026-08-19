#include "ffb_transport.h"

#include "hardware/uart.h"
#include "hardware/regs/uart.h"
#include "hardware/structs/uart.h"
#include "pico/stdlib.h"

/* Final transport behavior from A12:
 * - 4096-byte nonblocking FIFO
 * - complete logical-message framing
 * - 1 ms quiet time after normal SideWinder messages
 * - 75 ms quiet time after C5 01 Device Reset
 * - no blocking of TinyUSB/main loop while waiting
 */

#define QSIZE 4096u
#define QMASK (QSIZE - 1u)
#define NORMAL_GAP_US 1000u
#define RESET_GUARD_US 75000u
#define MAX_SYSEX_SCAN 256u

static uint8_t qdata[QSIZE];
static volatile uint16_t qhead;
static volatile uint16_t qtail;
static volatile uint32_t qdrop;

typedef enum {
    PH_IDLE = 0,
    PH_SENDING,
    PH_WAIT_UART_EMPTY,
    PH_WAIT_GAP
} pacing_phase_t;

static struct {
    pacing_phase_t phase;
    uint32_t deadline;
    uint16_t remaining;
    uint32_t post_gap_us;
} ps;

static inline uint16_t q_next(uint16_t p) { return (uint16_t)((p + 1u) & QMASK); }
static inline uint8_t q_at(uint16_t tail, uint16_t off) { return qdata[(tail + off) & QMASK]; }
static inline bool tx_full(void) { return (uart_get_hw(uart0)->fr & UART_UARTFR_TXFF_BITS) != 0u; }
static inline bool tx_empty_idle(void) {
    uint32_t fr = uart_get_hw(uart0)->fr;
    return (fr & (UART_UARTFR_TXFE_BITS | UART_UARTFR_BUSY_BITS)) == UART_UARTFR_TXFE_BITS;
}

void ffb_transport_init(void) {
    qhead = qtail = 0u;
    qdrop = 0u;
    ps.phase = PH_IDLE;
    ps.deadline = 0u;
    ps.remaining = 0u;
    ps.post_gap_us = NORMAL_GAP_US;
}

uint16_t ffb_transport_occupancy(void) {
    return (uint16_t)((qhead - qtail) & QMASK);
}

uint32_t ffb_transport_dropped(void) { return qdrop; }

bool ffb_transport_enqueue(const uint8_t *data, size_t len) {
    if (!data || len == 0u || len >= QSIZE) return false;
    uint16_t h = qhead;
    uint16_t t = qtail;
    uint16_t used = (uint16_t)((h - t) & QMASK);
    uint16_t free_bytes = (uint16_t)(QMASK - used);
    if (len > free_bytes) {
        qdrop++;
        return false;
    }

    /* Producer and consumer run from the same main-loop/TinyUSB context.
     * Publish the head only after the complete logical message is copied.
     */
    for (size_t i = 0; i < len; ++i) {
        qdata[h] = data[i];
        h = q_next(h);
    }
    qhead = h;
    return true;
}

static uint16_t message_length(uint16_t tail, uint16_t head, uint32_t *gap) {
    uint16_t occ = (uint16_t)((head - tail) & QMASK);
    if (!occ) return 0u;

    uint8_t b0 = q_at(tail, 0u);
    if (b0 == 0xc5u) {
        if (occ < 2u) return 0u;
        *gap = (q_at(tail, 1u) == 0x01u) ? RESET_GUARD_US : NORMAL_GAP_US;
        return 2u;
    }
    if (b0 == 0xb5u) {
        if (occ < 3u) return 0u;
        if (occ >= 4u && q_at(tail, 3u) == 0xa5u) {
            if (occ < 6u) return 0u;
            *gap = NORMAL_GAP_US;
            return 6u;
        }
        *gap = NORMAL_GAP_US;
        return 3u;
    }
    if (b0 == 0xf0u) {
        uint16_t limit = occ > MAX_SYSEX_SCAN ? MAX_SYSEX_SCAN : occ;
        for (uint16_t i = 1u; i < limit; ++i) {
            if (q_at(tail, i) == 0xf7u) {
                *gap = NORMAL_GAP_US;
                return (uint16_t)(i + 1u);
            }
        }
        return 0u;
    }

    /* Defensive framing recovery. */
    *gap = NORMAL_GAP_US;
    return 1u;
}

void ffb_transport_task(void) {
    if (ps.phase == PH_WAIT_UART_EMPTY) {
        if (!tx_empty_idle()) return;
        ps.deadline = time_us_32() + ps.post_gap_us;
        ps.phase = PH_WAIT_GAP;
        return;
    }

    if (ps.phase == PH_WAIT_GAP) {
        if ((int32_t)(time_us_32() - ps.deadline) < 0) return;
        ps.phase = PH_IDLE;
    }

    uint16_t tail = qtail;
    uint16_t head = qhead;

    if (ps.phase == PH_IDLE) {
        uint32_t gap = NORMAL_GAP_US;
        uint16_t len = message_length(tail, head, &gap);
        if (!len) return;
        ps.remaining = len;
        ps.post_gap_us = gap;
        ps.phase = PH_SENDING;
    }

    /* Retain the conservative historical 15-byte per-call ceiling. */
    uint32_t budget = 15u;
    tail = qtail;
    head = qhead;
    while (ps.phase == PH_SENDING && ps.remaining && budget) {
        if (tail == head || tx_full()) return;
        uart_get_hw(uart0)->dr = qdata[tail];
        tail = q_next(tail);
        qtail = tail;
        ps.remaining--;
        budget--;
        if (!ps.remaining) {
            ps.phase = PH_WAIT_UART_EMPTY;
            return;
        }
    }
}
