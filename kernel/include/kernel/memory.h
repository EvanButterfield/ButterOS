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

#endif // BUTTER_KERN_INC_MEMORY_H
