OUTPUT_ARCH(riscv)
ENTRY(kernel_boot)

KERNEL_PHYS_BASE = 0x80200000;

SECTIONS {
    . = KERNEL_PHYS_BASE;
    
    .text : {
        KEEP(*(.text.boot))
        *(.text)
        *(.text.*)
    }
    
    .rodata : {
        *(.rodata)
        *(.rodata.*)
    }
    
    .data : {
        *(.data)
        *(.data.*)
    }
    
    .bss : {
        __bss_start = .;
        
        . = ALIGN(16);
        *(.bss.stack)
        
        *(.bss)
        *(.bss.*)
        
        __bss_end = .;
    }
    
    __kernel_end = .;
}