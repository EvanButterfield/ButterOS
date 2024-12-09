#include <kernel/main.h>
#include <kernel/types.h>

int __attribute__((aligned(0x1000), sysv_abi)) KernelMain(butter_kernel_config *Config) {
	u32 XOffset = 0;
	u32 YOffset = 0;
	for(;;) {
		++XOffset;
		++YOffset;
		u32 *Row = (u32*)Config->FrameBuffer->Base;
		for(u32 Y = 0; Y < Config->FrameBuffer->VerticalResolution; ++Y) {
			u32 *Pixel = Row;
			for(u32 X = 0; X < Config->FrameBuffer->HorizontalResolution; ++X) {
				*Pixel++ = (XOffset << 24) | (YOffset << 16) | (0x00 << 8) | (0xFF);
			}

			Row += Config->FrameBuffer->PixelsPerScanLine;
		}
	}

	return(0);
}