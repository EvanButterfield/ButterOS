#include <kernel/main.h>

int __attribute__((sysv_abi)) KernelMain(butter_kernel_config *Config) {
	UINT32 XOffset = 0;
	UINT32 YOffset = 0;
	for(;;) {
		++XOffset;
		++YOffset;
		UINT32 *Row = (UINT32*)Config->FrameBuffer->Base;
		for(UINT32 Y = 0; Y < Config->FrameBuffer->VerticalResolution; ++Y) {
			UINT32 *Pixel = Row;
			for(UINT32 X = 0; X < Config->FrameBuffer->HorizontalResolution; ++X) {
				*Pixel++ = (XOffset << 24) | (YOffset << 16) | (0x00 << 8) | (0xFF);
			}

			Row += Config->FrameBuffer->PixelsPerScanLine;
		}
	}

	return(0);
}