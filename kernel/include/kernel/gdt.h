#ifndef KERN_GDT_H
#define KERN_GDT_H

#include <kernel/types.h>

#define STACK_SIZE 0x4000

typedef struct gdt_entry {
	u16 Limit15_0;
	u16 Base15_0;
	u8 Base23_16;
	u8 Access;
	u8 LimitAndFlags;
	u8 Base31_24;
} gdt_entry;

typedef struct gdt_entry_64 {
	u16 Limit15_0;
	u16 Base15_0;
	u8 Base23_16;
	u8 Access;
	u8 LimitAndFlags;
	u8 Base31_24;
	u32 Base63_32;
	u32 Reserved;
} gdt_entry_64;

typedef struct tss {
	u32 Reserved0;
	u64 RSP0;
	u64 RSP1;
	u64 RSP2;
	u64 Reserved1;
	u64 IST1;
	u64 IST2;
	u64 IST3;
	u64 IST4;
	u64 IST5;
	u64 IST6;
	u64 IST7;
	u64 Reserved2;
	u16 Reserved3;
	u16 IOPB;
} __attribute__((aligned(0x10))) tss;

#define GDT_KERNEL_CODE 0x8
#define GDT_KERNEL_DATA 0x10
#define GDT_USER_CODE   0x18
#define GDT_USER_DATA   0x20
#define GDT_TSS         0x28

typedef struct gdt {
	gdt_entry Null;
	gdt_entry KernelCode;
	gdt_entry KernelData;
	gdt_entry UserCode;
	gdt_entry UserData;
	gdt_entry_64 TSS;
} __attribute__((aligned(0x10))) gdt;

typedef struct table_ptr {
	u16 Limit;
	u64 Base;
} __attribute__((packed)) table_ptr;

#endif // KERN_GDT_H