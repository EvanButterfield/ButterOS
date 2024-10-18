#ifndef BUTTER_KERN_ELF
#define BUTTER_KERN_ELF

enum butter_elf_type {
	ET_NONE = 0,
	ET_REL  = 1,
	ET_EXEC = 2
};

enum butter_elf_shtypes {
	ESHT_NULL =     0,
	ESHT_PROGBITS = 1,
	ESHT_SYMTAB =   2,
	ESHT_STRTAB =   3,
	ESHT_RELA =     4,
	ESHT_NOBITS =   8,
	ESHT_REL =      9
};

enum butter_elf_shattributes {
	ESHA_WRITE = 1,
	ESHA_ALLOC = 2
};

#define ELF_SYMBOL_BIND(Info) ((Info) >> 4)
#define ELF_SYMBOL_TYPE(Info) ((Info) & 0x0f)

#define ELF_ABSOLUTE_SYMBOL 0xFFF1

enum butter_elf_symbindings {
	ESYMB_LOCAL  = 0,
	ESYMB_GLOBAL = 1,
	ESYMB_WEAK   = 2
};

enum butter_elf_symtypes {
	ESYMT_NOTYPE = 0,
	ESYMT_OBJECT = 1,
	ESYMT_FUNC   = 2
};

enum butter_elf_phtypes {
	EPHT_NULL    = 0,
	EPHT_LOAD    = 1,
	EPHT_DYNAMIC = 2,
	EPHT_INTERP  = 3,
	EPHT_NOTE    = 4
};

#pragma push(pack, 1)
typedef struct butter_elf_header {
	UINT8 Ident[16];
	UINT16 Type;
	UINT16 Machine;
	UINT32 Version;
	UINT64 Entry;
	UINT64 PHOffset;
	UINT64 SHOffset;
	UINT32 Flags;
	UINT16 EHSize;
	UINT16 PHEntrySize;
	UINT16 PHNum;
	UINT16 SHEntrySize;
	UINT16 SHNum;
	UINT16 SHStringIndex;
} butter_elf_header;

typedef struct butter_elf_sheader {
	UINT32 Name;
	UINT32 Type;
	UINT32 Flags;
	UINT64 Address;
	UINT64 Offset;
	UINT32 Size;
	UINT32 Link;
	UINT32 Info;
	UINT32 AddressAlign;
	UINT32 EntrySize;
} butter_elf_sheader;

typedef struct butter_elf_symbol {
	UINT32 Name;
	UINT32 Value;
	UINT32 Size;
	UINT8  Info;
	UINT8  Other;
	UINT16 Index;
} butter_elf_symbol;

typedef struct butter_elf_pheader {
	UINT32 Type;
	UINT32 Flags;
	UINT64 Offset;
	UINT64 VAddress;
	UINT64 PAddress;
	UINT64 FileSize;
	UINT64 MemSize;
	UINT64 Align;
} butter_elf_pheader;
#pragma pop()

static inline butter_elf_sheader *ELFGetSHeader(butter_elf_header *Header) {
	butter_elf_sheader *Result = (butter_elf_sheader*)((UINT8)Header + Header->SHOffset);
	return(Result);
}

static inline butter_elf_pheader *ELFGetPHeader(butter_elf_header *Header) {
	butter_elf_pheader *Result = (butter_elf_pheader*)((UINT64)Header + Header->PHOffset);
	return(Result);
}

static inline butter_elf_sheader *ELFGetSection(butter_elf_header *Header, UINT32 Index) {
	butter_elf_sheader *SHeader = ELFGetSHeader(Header);
	butter_elf_sheader *Result = &SHeader[Index];
	return(Result);
}

static UINT32 ELFGetSymbolVal(butter_elf_header *Header, UINT32 Table, UINT32 Index) {
	if(Table == 0 || Index == 0)
		return(-1);
	
	butter_elf_sheader *SymbolTable = ELFGetSection(Header, Table);
	UINT32 SymbolTableEntries = SymbolTable->Size / SymbolTable->EntrySize;
	if(Index >= SymbolTableEntries)
		return(-1);

	UINT32 SymbolAddress = (UINT32)Header + SymbolTable->Offset;
	butter_elf_symbol *Symbol = &((butter_elf_symbol*)SymbolAddress)[Index];
	
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
		butter_elf_sheader *Target = ELFGetSection(Header, Symbol->Index);
		UINT32 Result = (UINT32)Header + Symbol->Value + Target->Offset;
		return(Result);
	}
}

#endif // BUTTER_KERN_ELF
