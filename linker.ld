ENTRY(_start)

SECTIONS
{
    /* Begin loading at the 1 megabyte mark */
    . = 1M;

    .text : {
        /* Ensure the Multiboot header is at the very beginning */
        *(.multiboot)
        *(.text)
    }

    .rodata : {
        *(.rodata*)
    }

    .data : {
        *(.data)
    }

    .bss : {
        *(.bss)
    }
}
