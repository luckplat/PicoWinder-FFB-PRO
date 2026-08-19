ENTRY(a11_guarded_drain)
SECTIONS {
  . = 0x10006700;
  .text : { *(.text*) *(.rodata*) }
  /DISCARD/ : { *(.comment*) *(.ARM.attributes*) *(.eh_frame*) }
}
