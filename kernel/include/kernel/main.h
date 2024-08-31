#include <kernel/vga.h>

#define KERNEL_MAIN(Name) int Name(                               \
				butter_memory_map *MemoryMap,     \
				butter_frame_buffer *FrameBuffer, \
				EFI_FILE_HANDLE *CurrentVolume)
typedef KERNEL_MAIN(kernel_main);
