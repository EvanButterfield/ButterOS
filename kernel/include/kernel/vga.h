#ifndef BUTTER_KERN_INC_VGA_H
#define BUTTER_KERN_INC_VGA_H

#include <efi.h>

typedef struct butter_memory_map {
	UINTN Size;
	EFI_MEMORY_DESCRIPTOR *Map;
	UINTN Key;
	UINTN DescriptorSize;
	UINT32 DescriptorVersion;
} butter_memory_map;

typedef struct butter_frame_buffer {
	UINT32 BytesPerPixel;
	UINTN Size;
	EFI_PHYSICAL_ADDRESS Base;
	UINT32 HorizontalResolution;
	UINT32 VerticalResolution;
	UINT32 PixelsPerScanLine;
} butter_frame_buffer;

#endif // BUTTER_KERN_INC_VGA_H
