ENTRY(_start)

SECTIONS
{
  . = 0x400000;
  .text : {
    *(.text .text.*)
  }
  .rodata : {
    *(.rodata .rodata.*)
  }
  .data : {
    *(.data .data.*)
  }
  .sdata : ALIGN(16) {
    __global_pointer$ = . + 0x800;
    *(.sdata .sdata.*)
  }
  .bss : {
    *(.bss .bss.*)
    *(COMMON)
  }
  .sbss : {
    *(.sbss .sbss.*)
  }
  /DISCARD/ : {
    *(.eh_frame .note .comment)
  }
}
