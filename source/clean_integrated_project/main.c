#include <stdint.h>
#include <stdbool.h>

#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/uart.h"
#include "tusb.h"

#include "ffb_handshake.pio.h"
#include "read_joystick.pio.h"
#include "config.h"
#include "joystick_report.h"
#include "input_filter.h"
#include "ffb_transport.h"
#include "ffb_midi.h"
#include "ffb_engine.h"

static volatile picowinder_report_t report;

typedef struct {
    uint16_t buttons;
    uint16_t x:10;
    uint16_t y:10;
    uint8_t throttle:7;
    uint8_t twist:6;
    uint8_t hat:4;
} joystick_state_t;
static volatile joystick_state_t joystickState;

void joystickReadIRQ(void){
    const PIO pio=pio0;const uint sm=0;
    (void)pio_sm_get_rx_fifo_level(pio,sm);
    uint64_t raw0=pio_sm_get(pio,sm);uint64_t raw1=pio_sm_get(pio,sm);uint64_t raw=(raw1<<16)|(raw0>>8);
#ifdef FIRMWARE_SHIFT
    bool shift=((~raw)&0x100)!=0;uint16_t buttons=(uint16_t)((~raw)&0xffu);joystickState.buttons=shift?(uint16_t)(buttons<<8):buttons;
#else
    joystickState.buttons=(uint16_t)(~(raw&0x1ffu));
#endif
    joystickState.x=(uint16_t)((raw>>9)&0x3ffu);joystickState.y=(uint16_t)((raw>>19)&0x3ffu);
    joystickState.throttle=(uint8_t)((raw>>29)&0x7fu);joystickState.twist=(uint8_t)((raw>>36)&0x3fu);joystickState.hat=(uint8_t)((raw>>42)&0x0fu);
    report.buttons=joystickState.buttons;report.x=joystickState.x;report.y=joystickState.y;report.twist=joystickState.twist;report.throttle=joystickState.throttle;report.hat=joystickState.hat;
    pio_interrupt_clear(pio,0);
}

static void hid_task(void){
    if(tud_suspended()){tud_remote_wakeup();return;}
    if(!tud_hid_ready())return;
    uint8_t snapshot[PICOWINDER_REPORT_SIZE];
    input_filter_make_snapshot(&report,snapshot);
    tud_hid_n_report(0x00,0x01,snapshot,PICOWINDER_REPORT_SIZE);
}

int main(void){
    tud_init(0);
    uart_init(uart0,31250);
    gpio_set_function(PIN_MIDI_TX,UART_FUNCSEL_NUM(uart0,PIN_MIDI_TX));
    ffb_transport_init();ffb_engine_init();input_filter_reset();

    PIO pio=pio0;uint sm=0;
    uint offset_handshake=pio_add_program(pio,&ffb_handshake_program);
    ffb_handshake_program_init(pio,sm,offset_handshake,100000,PIN_TRIGGER);
    uint delays[7]={1000,70,300,150,780,40,590};uint pulses[7]={1,4,3,2,2,3,2};
    for(int i=0;i<7;i++){uint32_t word=(pulses[i]-1u)<<16|(delays[i]-1u);pio_sm_put(pio,sm,word);}
    pio_sm_set_enabled(pio,sm,true);sleep_ms(400);pio_sm_set_enabled(pio,sm,false);
#ifdef DISABLE_AUTO_CENTER
    ffb_midi_set_autocenter(false);
#endif
    uint offset_readjoy=pio_add_program(pio,&read_joystick_program);
    read_joystick_program_init(pio,sm,offset_readjoy,1000000,PIN_TRIGGER,PIN_CLK,PIN_D0,PIN_D1,PIN_D2);
    uint pio_irq=PIO0_IRQ_0;pio_set_irq0_source_enabled(pio0,pis_interrupt0,true);irq_set_exclusive_handler(pio_irq,joystickReadIRQ);irq_set_enabled(pio_irq,true);pio_sm_set_enabled(pio,sm,true);

    /* A13: no local example Spring and no trigger kick. */
    while(1){
        tud_task();
        ffb_transport_task();
        hid_task();
        ffb_transport_task();
    }
}
