#include <kernel/vga.h>
#include <kernel/memory.h>

typedef struct butter_kernel_config {
	butter_frame_buffer *FrameBuffer;
	butter_memory_map *MemoryMap;
	EFI_FILE *Root;
} butter_kernel_config;

typedef int (__attribute__((sysv_abi)) *kernel_main) (butter_kernel_config *Config);
