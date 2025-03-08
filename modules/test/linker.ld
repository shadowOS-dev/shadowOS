ENTRY(module_init)

SECTIONS
{
    . = 0x100000;

    .text : {
        *(.text)
    }

    .data : {
        *(.data)
    }

    .bss : {
        *(COMMON)
        *(.bss)
    }

    . = ALIGN(4);
    module_init = .;
}
