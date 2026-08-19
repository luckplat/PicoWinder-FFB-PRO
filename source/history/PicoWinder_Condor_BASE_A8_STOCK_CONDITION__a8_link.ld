ENTRY(a8_handler)
SECTIONS {
  . = 0x10006500;
  .text : { *(.text*) *(.rodata*) }
  /DISCARD/ : { *(.comment*) *(.ARM.attributes*) *(.eh_frame*) }
}
