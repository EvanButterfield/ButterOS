.section .text
.global LoadGDTAndPML4
LoadGDTAndPML4:
    cli

    lgdt (%rdx)
    mov  $0x28, %ax
    ltr  %ax
    ret

    mov %rcx, %cr3

    mov  $0x10, %ax
    mov   %ax,   %ds
    mov   %ax,   %es
    mov   %ax,   %fs
    mov   %ax,   %gs
    mov   %ax,   %ss

    pop  %rdi
    mov  $0x08,  %rax
    push %rax
    push %rdi
    lretq

.global EnableSCE
EnableSCE:
    mov $0xc0000080, %rcx
    rdmsr
    or  $0x1, %eax
    wrmsr
    mov $0xc0000081, %rcx
    rdmsr
    mov $0x00180008, %edx
    wrmsr
    ret

.global SetStack
SetStack:
    mov %rcx, %rbp
    mov %rcx, %rsp
    mov %rdx, %rdi
    jmp *%r8
    ret
