ENTRY(a12_paced_drain)
SECTIONS {
  . = 0x10006900;
  .text : { *(.text*) *(.rodata*) }
  /DISCARD/ : { *(.comment*) *(.ARM.attributes*) *(.eh_frame*) }
}
