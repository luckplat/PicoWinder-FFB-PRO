ENTRY(a16_hid_report)
SECTIONS {
  . = 0x10006D00;
  .text : { *(.text*) *(.rodata*) }
  /DISCARD/ : { *(.comment*) *(.ARM.attributes*) *(.eh_frame*) }
}
