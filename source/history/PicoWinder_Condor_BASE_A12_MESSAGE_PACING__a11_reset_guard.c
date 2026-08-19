#include <stdint.h>

/*
 * A11 RESET_GUARD: asynchronous physical-UART barrier after SideWinder RESET.
 *
 * A10/A2 already queues the exact byte stream:
 *     C5 01   SideWinder FFB Pro Reset
 *     C5 06   Stop All / native autocentre off
 *
 * Condor then immediately queues its own STOP ALL, GAIN, CREATE, ...
 *
 * A11 changes ONLY the FIFO drain policy. When the raw FIFO reaches C5 01:
 *   1. transmit exactly C5 01;
 *   2. wait until UART0 is TX-empty and not BUSY (RESET has left the wire);
 *   3. hold the physical FFB UART for 75,000 us without blocking USB/main loop;
 *   4. resume the FIFO. The next queued bytes are A2's existing C5 06,
 *      followed by the host commands already accumulated behind it.
 *
 * No USB callback delay. No PIO/input change. No effect translation change.
 */

#define QBASE       ((volatile uint8_t*)0x20010000u)
#define QMASK       0x0fffu
#define QDATA_OFF   8u

/* Deliberately outside A10 engine state (0x20012000..~0x200126xx). */
#define GSTATE_ADDR 0x20012f00u
#define G_MAGIC     0xA1175AA1u

#define UART0_DR    (*(volatile uint32_t*)0x40034000u)
#define UART0_FR    (*(volatile uint32_t*)0x40034018u)
#define UART_FR_BUSY 0x08u
#define UART_FR_TXFF 0x20u
#define UART_FR_TXFE 0x80u

#define TIMERAWL    (*(volatile uint32_t*)0x40054028u)
#define RESET_GUARD_US 75000u

struct GuardState {
    uint32_t magic;
    uint32_t state;
    uint32_t deadline;
};
#define GS ((volatile struct GuardState*)GSTATE_ADDR)

enum {
    GS_NORMAL = 0u,
    GS_SEND_RESET_SECOND = 1u,
    GS_WAIT_UART_EMPTY = 2u,
    GS_WAIT_75MS = 3u
};

static inline uint16_t q_head(void) { return *(volatile uint16_t*)(QBASE+0u); }
static inline uint16_t q_tail(void) { return *(volatile uint16_t*)(QBASE+2u); }
static inline void q_set_tail(uint16_t v) { *(volatile uint16_t*)(QBASE+2u)=v; }
static inline uint8_t q_byte(uint16_t p) { return QBASE[QDATA_OFF + (p & QMASK)]; }
static inline uint16_t q_next(uint16_t p) { return (uint16_t)((p+1u)&QMASK); }

static inline void ensure_state(void) {
    if (GS->magic != G_MAGIC) {
        GS->state=GS_NORMAL;
        GS->deadline=0u;
        GS->magic=G_MAGIC;
    } else if (GS->state > GS_WAIT_75MS) {
        GS->state=GS_NORMAL;
        GS->deadline=0u;
    }
}

__attribute__((used,noinline))
void a11_guarded_drain(void) {
    ensure_state();

    uint16_t tail=q_tail();
    uint16_t head=q_head();

    /* If C5 was sent but the FIFO/UART had no room for 01 in that same pass,
       finish only the reset command; never release later bytes here. */
    if (GS->state==GS_SEND_RESET_SECOND) {
        if (tail==head) return; /* defensive; qwrite normally made C5 01 atomic */
        if (UART0_FR & UART_FR_TXFF) return;
        if (q_byte(tail)!=0x01u) {
            /* Defensive recovery: do not fabricate a reset byte. */
            GS->state=GS_NORMAL;
        } else {
            UART0_DR=0x01u;
            tail=q_next(tail);
            q_set_tail(tail);
            GS->state=GS_WAIT_UART_EMPTY;
            return;
        }
    }

    /* Start the 75 ms timer only after C5 01 has physically left UART0. */
    if (GS->state==GS_WAIT_UART_EMPTY) {
        uint32_t fr=UART0_FR;
        if ((fr & (UART_FR_TXFE|UART_FR_BUSY)) != UART_FR_TXFE) return;
        GS->deadline=TIMERAWL + RESET_GUARD_US;
        GS->state=GS_WAIT_75MS;
        return;
    }

    /* Wrap-safe deadline test. USB/main-loop keep running while we return. */
    if (GS->state==GS_WAIT_75MS) {
        uint32_t now=TIMERAWL;
        if ((int32_t)(now-GS->deadline) < 0) return;
        GS->state=GS_NORMAL;
    }

    /* Preserve A10's old drain budget: at most 15 raw UART bytes per call. */
    uint32_t budget=15u;
    tail=q_tail();
    head=q_head();

    while (tail!=head && budget!=0u) {
        if (UART0_FR & UART_FR_TXFF) break;

        uint8_t b=q_byte(tail);
        uint16_t n=q_next(tail);

        /* C5 is a MIDI status byte; SideWinder data bytes are 7-bit, so
           C5 01 cannot occur inside parameter payload data. Detect RESET at
           the raw FIFO boundary, including a ring wrap between C5 and 01. */
        if (b==0xc5u && n!=head && q_byte(n)==0x01u) {
            UART0_DR=0xc5u;
            tail=n;
            q_set_tail(tail);

            /* Send 01 now if UART still has room; otherwise finish next pass. */
            if (UART0_FR & UART_FR_TXFF) {
                GS->state=GS_SEND_RESET_SECOND;
                return;
            }
            UART0_DR=0x01u;
            tail=q_next(tail);
            q_set_tail(tail);
            GS->state=GS_WAIT_UART_EMPTY;
            return;
        }

        UART0_DR=(uint32_t)b;
        tail=n;
        q_set_tail(tail);
        --budget;
    }
}
