#ifndef BUTTER_KERN_INC_MEMORY_H
#define BUTTER_KERN_INC_MEMORY_H

#include <efi.h>

#define EFI_PAGES_TO_SIZE(Pages) ((Pages) << EFI_PAGE_SHIFT)

typedef struct butter_memory_map {
	UINTN Size;
	EFI_MEMORY_DESCRIPTOR *Map;
	UINTN Key;
	UINTN DescriptorSize;
	UINT32 DescriptorVersion;
} butter_memory_map;

static void MemCopy(void *_Destination, void *_Source, UINTN Size) {
	CHAR8 *Destination = (CHAR8*)_Destination;
	CHAR8 *Source = (CHAR8*)_Source;

	for(UINTN ByteIndex = 0; ByteIndex < Size; ++ByteIndex) {
		Destination[ByteIndex] = Source[ByteIndex];
	}
}

static void MemSet(void *_Mem, UINT32 Value, UINTN Size) {
	CHAR8 *Mem = (CHAR8*)_Mem;
	for(UINTN ByteIndex = 0; ByteIndex < Size; ++ByteIndex) {
		Mem[ByteIndex] = Value;
	}
}

#endif // BUTTER_KERN_INC_MEMORY_H
