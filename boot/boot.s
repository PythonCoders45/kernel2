.set ALIGN,    1<<0
.set MEMINFO,  1<<1
.set FLAGS,    ALIGN | MEMINFO
.set MAGIC,    0x1BADB002
.set CHECKSUM, -(MAGIC + FLAGS)

/* Multiboot Header */
.section .multiboot
.align 4
.long MAGIC
.long FLAGS
.long CHECKSUM

/* Set up a small stack space in uninitialized memory */
.section .bss
.align 16
stack_bottom:
.skip 16384 /* 16 Kilobytes of stack space */
stack_top:

.section .text
.global _start
.type _start, @function
_start:
    /* Point the stack pointer to our allocated stack */
    mov $stack_top, %esp

    /* Jump into your C kernel entry point */
    call kernel_main

    /* If the kernel returns, halt the CPU safely */
    cli
1:  hlt
    jmp 1b
