OUTPUT_FORMAT("elf32-i386");
ENTRY(_start);
SECTIONS
{
    . = 0x7C00;
    /* Code section, .text.bootentry code before other code */
    .text : SUBALIGN(0) {
        *(.text.bootentry);
        *(.text)
    }

    /* Read only data section with no alignment */
    .rodata : SUBALIGN(0) {
        *(.rodata)
    }

    /* Data section with no alignment */
    .data : SUBALIGN(0) {
        *(.data)
    }

    /* Boot signature at 510th byte from 0x7c00 */
    .sig : AT(0x7DFE) {
        SHORT(0xaa55);
    }

    /DISCARD/ : {
        *(.eh_frame);
        *(.comment);
        *(.note*);
    }
}
