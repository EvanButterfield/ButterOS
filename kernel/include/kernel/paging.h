#ifndef KERN_PAGING_H
#define KERN_PAGING_H

#include <kernel/types.h>
#include <kernel/memory.h>

#define PAGE_SIZE 0x10000

#define PAGE_BIT_PRESENT         (1 << 0)
#define PAGE_BIT_READ_WRITE      (1 << 1)
#define PAGE_BIT_USER_MODE       (1 << 2)
#define PAGE_BIT_ACCESSED        (1 << 5)
#define PAGE_BIT_DIRTY           (1 << 6)
#define PAGE_BIT_EXECUTE_DISABLE (1 << 63)
#define PAGE_IDENTITY (PAGE_BIT_PRESENT | PAGE_BIT_READ_WRITE | PAGE_BIT_USER_MODE)

#define PAGE_ADDRESS_MASK 0x000ffffffffff000

#define KERNEL_VIRTUAL_ADDRESS 0xdeadbeefcafebabe

// 63-48: Canonical
// 47-39: PML4
// 38-30: Directory Pointer
// 29-21: Directory
// 20-12: Table
#define VIRTUAL_PML4_INDEX(Virtual) (((Virtual) >> 39) & 0x1ff)
#define VIRTUAL_PDPT_INDEX(Virtual) (((Virtual) >> 30) & 0x1ff)
#define VIRTUAL_PDT_INDEX(Virtual)  (((Virtual) >> 21) & 0x1ff)
#define VIRTUAL_PT_INDEX(Virtual)   (((Virtual) >> 12) & 0x1ff)

#define MAPPING_TABLE_ENTRIES 512
typedef struct mapping_table {
	u64 Entries[MAPPING_TABLE_ENTRIES];
} __attribute__((aligned(0x1000))) mapping_table;

mapping_table PML4 = {0};

u64 NextAllocPage;
u64 PagesLeft;

static void *KAllocPages(u64 Pages) {
	void *Page = (void*)NextAllocPage;
	NextAllocPage += PAGE_SIZE * Pages;
	PagesLeft -= Pages;
	return(Page);
}

static void KMapPages(u64 _Physical, u64 _Virtual, u64 NumPages, u64 Flags) {
    for(u64 Page = 0; Page < NumPages; ++Page) {
        u64 Addition = PAGE_SIZE*Page;
        u64 Physical = _Physical + Addition;
        u64 Virtual = _Virtual + Addition;

        u32 PML4Index = VIRTUAL_PML4_INDEX(Virtual);
        u32 PDPTIndex = VIRTUAL_PDPT_INDEX(Virtual);
        u32 PDTIndex = VIRTUAL_PDT_INDEX(Virtual);
        u32 PTIndex = VIRTUAL_PT_INDEX(Virtual);

        if(!(PML4.Entries[PML4Index] & PAGE_BIT_PRESENT)) {
            u64 PDPTAlloc = (u64)KAllocPages(1);
            MemSet((void*)PDPTAlloc, 0, PAGE_SIZE);
            PML4.Entries[PML4Index] = (PDPTAlloc & PAGE_ADDRESS_MASK) | PAGE_IDENTITY;
        }

        mapping_table *PDPT = (mapping_table*)(PML4.Entries[PML4Index] & PAGE_ADDRESS_MASK);
        if(!(PDPT->Entries[PDPTIndex] & PAGE_BIT_PRESENT)) {
            u64 PDTAlloc = (u64)KAllocPages(1);
            MemSet((void*)PDTAlloc, 0, PAGE_SIZE);
            PDPT->Entries[PDPTIndex] = (PDTAlloc & PAGE_ADDRESS_MASK) | PAGE_IDENTITY;
        }

        mapping_table *PDT = (mapping_table*)(PDPT->Entries[PDPTIndex] & PAGE_ADDRESS_MASK);
        if(!(PDT->Entries[PDTIndex] & PAGE_BIT_PRESENT)) {
            u64 PTAlloc = (u64)KAllocPages(1);
            MemSet((void*)PTAlloc, 0, PAGE_SIZE);
            PDT->Entries[PDTIndex] = (PTAlloc & PAGE_ADDRESS_MASK) | PAGE_IDENTITY;
        }

        mapping_table *PT = (mapping_table*)(PDT->Entries[PDTIndex] & PAGE_ADDRESS_MASK);
        PT->Entries[PTIndex] = (Physical & PAGE_ADDRESS_MASK) | Flags;
    }
}

#endif // KERN_PAGING_H