ENTRY(a15_handler)
SECTIONS {
  . = 0x10006B00;
  .text : { *(.text*) *(.rodata*) }
  /DISCARD/ : { *(.comment*) *(.ARM.attributes*) *(.eh_frame*) }
}
