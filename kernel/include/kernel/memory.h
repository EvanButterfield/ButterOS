#ifndef BUTTER_KERN_INC_MEMORY_H
#define BUTTER_KERN_INC_MEMORY_H

#include <efi.h>

typedef struct butter_memory_map {
	UINTN Size;
	EFI_MEMORY_DESCRIPTOR *Map;
	UINTN Key;
	UINTN DescriptorSize;
	UINT32 DescriptorVersion;
} butter_memory_map;

#endif // BUTTER_KERN_INC_MEMORY_H
