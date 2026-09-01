ENTRY(_start)

MEMORY
{
    RAM (rwx) : ORIGIN = 0x80000000, LENGTH = 128K
}

SECTIONS
{
    . = ORIGIN(RAM);

    .text : {
        *(.text.init)    /* _start sits at ORIGIN */
        *(.text .text.*)
    } > RAM

    .rodata : {
        *(.rodata .rodata.*)
    } > RAM

    .data : {
        *(.data .data.*)
    } > RAM

    .bss (NOLOAD) : {
        *(.bss .bss.*)
        *(COMMON)
    } > RAM

    /* stack_top is at end of 128K of RAM, Stack grows down from the top of RAM */
    _stack_top = ORIGIN(RAM) + LENGTH(RAM);
}
