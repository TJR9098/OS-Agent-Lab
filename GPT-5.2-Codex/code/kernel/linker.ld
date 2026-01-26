OUTPUT_ARCH(riscv)
ENTRY(kernel_boot)

KERNEL_PHYS_BASE = 0x80200000;
KERNEL_VIRT_BASE = 0xffffffff80000000;
KERNEL_VIRT_OFFSET = KERNEL_VIRT_BASE - KERNEL_PHYS_BASE;

SECTIONS {
  . = KERNEL_PHYS_BASE;
  .boot : ALIGN(16) {
    KEEP(*(.text.boot))
    KEEP(*(.boot))
    KEEP(*(.rodata.boot))
    KEEP(*(.data.boot))
    KEEP(*(.bss.boot))
  }
  __boot_end = .;

  . = KERNEL_VIRT_BASE + (. - KERNEL_PHYS_BASE);
  . = ALIGN(4096);

  .text : AT(ADDR(.text) - KERNEL_VIRT_OFFSET) {
    __text_start = .;
    *(.text .text.*)
    __text_end = .;
  }
  .rodata : AT(ADDR(.rodata) - KERNEL_VIRT_OFFSET) {
    *(.rodata .rodata.*)
  }
  .data : AT(ADDR(.data) - KERNEL_VIRT_OFFSET) {
    *(.data .data.*)
  }
  .bss : AT(ADDR(.bss) - KERNEL_VIRT_OFFSET) {
    __bss_start = .;
    *(.bss .bss.*)
    *(COMMON)
    __bss_end = .;
  }
  __kernel_end = .;
}