/* Based on upstream PicoWinder/TinyUSB descriptor source. GPL-3.0 project;
 * TinyUSB-derived portions retain their original MIT notice in upstream.
 * A22 change: USB Product string is "Picowinder FFB PRO".
 */
#include "tusb.h"
#include "usb_descriptors.h"
#include "bsp/board_api.h"
#include "usb_report_ids.h"
#include <string.h>

#define _PID_MAP(itf,n) ((CFG_TUD_##itf)<<(n))
#define USB_PID (0x4000|_PID_MAP(CDC,0)|_PID_MAP(MSC,1)|_PID_MAP(HID,2)|_PID_MAP(MIDI,3)|_PID_MAP(VENDOR,4))

tusb_desc_device_t const desc_device={
    .bLength=sizeof(tusb_desc_device_t),.bDescriptorType=TUSB_DESC_DEVICE,.bcdUSB=0x0200,
    .bDeviceClass=0x00,.bDeviceSubClass=0x00,.bDeviceProtocol=0x00,.bMaxPacketSize0=CFG_TUD_ENDPOINT0_SIZE,
    .idVendor=0xcafe,.idProduct=USB_PID,.bcdDevice=0x0100,.iManufacturer=0x01,.iProduct=0x02,.iSerialNumber=0x03,.bNumConfigurations=0x01
};
uint8_t const*tud_descriptor_device_cb(void){return(uint8_t const*)&desc_device;}

uint8_t const desc_hid_report[]={
    HID_USAGE_PAGE(HID_USAGE_PAGE_DESKTOP),HID_USAGE(HID_USAGE_DESKTOP_JOYSTICK),HID_COLLECTION(HID_COLLECTION_APPLICATION),
    SIDEWINDER_REPORT_DESC_INPUT_JOYSTICK(HID_REPORT_ID(REPORT_ID_INPUT_JOYSTICK)),
    SIDEWINDER_REPORT_DESC_OUTPUT_SET_EFFECT(HID_REPORT_ID(REPORT_ID_OUTPUT_SET_EFFECT)),
    SIDEWINDER_REPORT_DESC_OUTPUT_SET_ENVELOPE(HID_REPORT_ID(REPORT_ID_OUTPUT_SET_ENVELOPE)),
    SIDEWINDER_REPORT_DESC_OUTPUT_SET_CONDITION(HID_REPORT_ID(REPORT_ID_OUTPUT_SET_CONDITION)),
    SIDEWINDER_REPORT_DESC_OUTPUT_SET_PERIODIC(HID_REPORT_ID(REPORT_ID_OUTPUT_SET_PERIODIC)),
    SIDEWINDER_REPORT_DESC_OUTPUT_SET_CONSTANT(HID_REPORT_ID(REPORT_ID_OUTPUT_SET_CONSTANT)),
    SIDEWINDER_REPORT_DESC_OUTPUT_SET_RAMP(HID_REPORT_ID(REPORT_ID_OUTPUT_SET_RAMP)),
    SIDEWINDER_REPORT_DESC_OUTPUT_EFFECT_OPERATION(HID_REPORT_ID(REPORT_ID_OUTPUT_EFFECT_OPERATION)),
    SIDEWINDER_REPORT_DESC_OUTPUT_BLOCK_FREE(HID_REPORT_ID(REPORT_ID_OUTPUT_BLOCK_FREE)),
    SIDEWINDER_REPORT_DESC_OUTPUT_DEVICE_CONTROL(HID_REPORT_ID(REPORT_ID_OUTPUT_DEVICE_CONTROL)),
    SIDEWINDER_REPORT_DESC_OUTPUT_DEVICE_GAIN(HID_REPORT_ID(REPORT_ID_OUTPUT_DEVICE_GAIN)),
    SIDEWINDER_REPORT_DESC_FEATURE_CREATE_NEW_EFFECT(HID_REPORT_ID(REPORT_ID_FEATURE_CREATE_NEW_EFFECT)),
    SIDEWINDER_REPORT_DESC_FEATURE_BLOCK_LOAD(HID_REPORT_ID(REPORT_ID_FEATURE_BLOCK_LOAD)),
    SIDEWINDER_REPORT_DESC_FEATURE_POOL_REPORT(HID_REPORT_ID(REPORT_ID_FEATURE_POOL_REPORT)),
    HID_COLLECTION_END
};
uint8_t const*tud_hid_descriptor_report_cb(uint8_t instance){(void)instance;return desc_hid_report;}

enum{ITF_NUM_HID,ITF_NUM_TOTAL};
#define CONFIG_TOTAL_LEN (TUD_CONFIG_DESC_LEN+TUD_HID_DESC_LEN)
#define EPNUM_HID 0x81
uint8_t const desc_configuration[]={
    TUD_CONFIG_DESCRIPTOR(1,ITF_NUM_TOTAL,0,CONFIG_TOTAL_LEN,TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP,100),
    TUD_HID_DESCRIPTOR(ITF_NUM_HID,0,HID_ITF_PROTOCOL_NONE,sizeof(desc_hid_report),EPNUM_HID,CFG_TUD_HID_EP_BUFSIZE,1)
};
uint8_t const*tud_descriptor_configuration_cb(uint8_t index){(void)index;return desc_configuration;}

enum{STRID_LANGID=0,STRID_MANUFACTURER,STRID_PRODUCT,STRID_SERIAL};
char const*string_desc_arr[]={
    (const char[]){0x09,0x04},
    "Nolbinsoft",
    "Picowinder FFB PRO",
    NULL
};
uint16_t const*tud_descriptor_string_cb(uint8_t index,uint16_t langid){
    static uint16_t d[32];const size_t max_count=sizeof(d)/sizeof(d[0])-1u;(void)langid;size_t n;
    switch(index){
        case STRID_LANGID:memcpy(&d[1],string_desc_arr[0],2);n=1;break;
        case STRID_SERIAL:n=board_usb_get_serial(d+1,32);break;
        default:
            if(index>=sizeof(string_desc_arr)/sizeof(string_desc_arr[0]))return NULL;
            {const char*str=string_desc_arr[index];n=strlen(str);if(n>max_count)n=max_count;for(size_t i=0;i<n;i++)d[1+i]=(uint16_t)str[i];}
            break;
    }
    d[0]=(uint16_t)((TUSB_DESC_STRING<<8)|(2u*n+2u));return d;
}
