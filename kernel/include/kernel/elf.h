#ifndef KERN_ELF_H
#define KERN_ELF_H

#include <efi.h>
#include <kernel/types.h>
#include <kernel/paging.h>

#define ELF_R_SYM(I)  ((I) >> 32)
#define ELF_R_TYPE(I) ((I) & 0xffffffff)

enum elf_type {
	ET_NONE = 0,
	ET_REL  = 1,
	ET_EXEC = 2
};

enum elf_shtypes {
	ESHT_NULL =     0,
	ESHT_PROGBITS = 1,
	ESHT_SYMTAB =   2,
	ESHT_STRTAB =   3,
	ESHT_RELA =     4,
	ESHT_NOBITS =   8,
	ESHT_REL =      9
};

enum elf_shattributes {
	ESHA_WRITE = 1,
	ESHA_ALLOC = 2
};

#define ELF_SYMBOL_BIND(Info) ((Info) >> 4)
#define ELF_SYMBOL_TYPE(Info) ((Info) & 0x0f)

#define ELF_ABSOLUTE_SYMBOL 0xFFF1

enum elf_symbindings {
	ESYMB_LOCAL  = 0,
	ESYMB_GLOBAL = 1,
	ESYMB_WEAK   = 2
};

enum elf_symtypes {
	ESYMT_NOTYPE = 0,
	ESYMT_OBJECT = 1,
	ESYMT_FUNC   = 2
};

enum elf_phtypes {
	EPHT_NULL    = 0,
	EPHT_LOAD    = 1,
	EPHT_DYNAMIC = 2,
	EPHT_INTERP  = 3,
	EPHT_NOTE    = 4
};

enum elf_reltypes {
	ERT_386_NONE = 0, // No relocation
	ERT_386_32   = 1, // Symbol + Offset
	ERT_386_PC32 = 2  // Symbol + Offset - Section Offset
};

#pragma push(pack, 1)
typedef struct elf_header {
	u8 Ident[16];
	u16 Type;
	u16 Machine;
	u32 Version;
	u64 Entry;
	u64 PHOffset;
	u64 SHOffset;
	u32 Flags;
	u16 EHSize;
	u16 PHEntrySize;
	u16 PHNum;
	u16 SHEntrySize;
	u16 SHNum;
	u16 SHStringIndex;
} elf_header;

typedef struct elf_sheader {
	u32 Name;
	u32 Type;
	u32 Flags;
	u64 Address;
	u64 Offset;
	u32 Size;
	u32 Link;
	u32 Info;
	u32 AddressAlign;
	u32 EntrySize;
} elf_sheader;

typedef struct elf_symbol {
	u32 Name;
	u32 Value;
	u32 Size;
	u8  Info;
	u8  Other;
	u16 Index;
} elf_symbol;

typedef struct elf_pheader {
	u32 Type;
	u32 Flags;
	u64 Offset;
	u64 VAddress;
	u64 PAddress;
	u64 FileSize;
	u64 MemSize;
	u64 Align;
} elf_pheader;

typedef struct elf_rel {
	u64 Offset;
	u64 Info;
} elf_rel;
#pragma pop()

static inline elf_sheader *ELFGetSHeader(elf_header *Header) {
	elf_sheader *Result = (elf_sheader*)((u8)Header + Header->SHOffset);
	return(Result);
}

static inline elf_pheader *ELFGetPHeader(elf_header *Header) {
	elf_pheader *Result = (elf_pheader*)((u64)Header + Header->PHOffset);
	return(Result);
}

static inline elf_sheader *ELFGetSection(elf_header *Header, u32 Index) {
	elf_sheader *SHeader = ELFGetSHeader(Header);
	elf_sheader *Result = &SHeader[Index];
	return(Result);
}

static u32 ELFGetSymbolVal(elf_header *Header, u32 Table, u32 Index) {
	if(Table == 0 || Index == 0)
		return(-1);
	
	elf_sheader *SymbolTable = ELFGetSection(Header, Table);
	u32 SymbolTableEntries = SymbolTable->Size / SymbolTable->EntrySize;
	if(Index >= SymbolTableEntries)
		return(-1);

	u32 SymbolAddress = (u32)Header + SymbolTable->Offset;
	elf_symbol *Symbol = &((elf_symbol*)SymbolAddress)[Index];
	
	if(Symbol->Index == 0) {
		// External symbol
		if(ELF_SYMBOL_BIND(Symbol->Info) & ESYMB_WEAK) {
			return(0);
		} else {
			return(-1);
		}
	} else if(Symbol->Index == ELF_ABSOLUTE_SYMBOL) {
		// Absolute symbol
		return(Symbol->Value);
	} else {
		// Internal symbol
		elf_sheader *Target = ELFGetSection(Header, Symbol->Index);
		u32 Result = (u32)Header + Symbol->Value + Target->Offset;
		return(Result);
	}
}

// Load relocatable elf
/*
elf_sheader *SHeader = ELFGetSHeader(Header);
		for(u16 SHeaderIndex = 0; SHeaderIndex < Header->SHNum; ++SHeaderIndex) {
			elf_sheader *Section = &SHeader[SHeaderIndex];

			if(Section->Type == ESHT_NOBITS) {
				if(Section->Size == 0) continue;

				if(Section->Flags & ESHA_ALLOC) {
					u32 SectionPages = EFI_SIZE_TO_PAGES(Section->Size);
					void *Memory;
					gBS->AllocatePages(AllocateAnyPages, EfiLoaderData, SectionPages, &Memory);
					MemSet(Memory, 0, Section->Size);
					Section->Offset = (u64)Memory - (u64)Header;
				}
			}
		}

		for(u16 SHeaderIndex = 0; SHeaderIndex < Header->SHNum; ++SHeaderIndex) {
			elf_sheader *Section = &SHeader[SHeaderIndex];

			if(Section->Type == ESHT_REL) {
				for(u32 Index = 0; Index < (Section->Size / Section->EntrySize); ++Index) {
					elf_rel *Rel = &((elf_rel*)((u64)Header + Section->Offset))[Index];
					elf_sheader *Target = ELFGetSection(Header, Section->Info);

					u64 Address = (u64)Header + Target->Offset;
					u64 *Ref = (u64*)(Address + Rel->Offset);

					u32 SymbolVal = 0;
					if(ELF_R_SYM(Rel->Info) != 0) {
						SymbolVal = ELFGetSymbolVal(Header, Section->Link, ELF_R_SYM(Rel->Info));
						if(SymbolVal == -1) {
							PRINT(L"Error getting kernel section relocation symbol.\n\r");
							asm("int $0x8");
						}
					}

					switch(ELF_R_TYPE(Rel->Info)) {
						case ERT_386_NONE: {
						} break;

						case ERT_386_32: {
							*Ref = SymbolVal + *Ref;
						} break;

						case ERT_386_PC32: {
							*Ref = SymbolVal + *Ref - (u64)Ref;
						} break;

						default: {
							PRINT(L"Unsupported kernel relocation type.\n\r");
							asm("int $0x8");
						}
					}
				}
			}
		}
*/

#endif // KERN_ELF_H
