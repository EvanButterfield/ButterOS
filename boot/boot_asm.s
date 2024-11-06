.section .text
.global LoadGDT
LoadGDT:
    cli

    lgdt (%rcx)
    movw $0x28, %ax
    ltr  %ax

    movw  $0x10, %ax
    mov   %ax,   %ds
    mov   %ax,   %es
    mov   %ax,   %fs
    mov   %ax,   %gs
    mov   %ax,   %ss
    popq  %rdi
    movq  $0x08,  %rax
    pushq %rax
    pushq %rdi
    lretq
