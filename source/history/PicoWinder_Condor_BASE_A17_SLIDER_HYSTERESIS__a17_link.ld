ENTRY(a17_hid_report)
SECTIONS {
  . = 0x10006E00;
  .text : { *(.text*) *(.rodata*) }
  /DISCARD/ : { *(.comment*) *(.ARM.attributes*) *(.eh_frame*) }
}
