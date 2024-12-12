#include <kernel/main.h>
#include <kernel/types.h>
#include <kernel/paging.h>

#include <kernel/font.h>

#define COLOR_VAL(A, B, C, D) ((A) << 24 | (B) << 16 | (C) << 8 | (D))
#define TTF_FLAG(A, B, C, D) COLOR_VAL((A), (B), (C), (D))

u32 strlen(u8 *Str) {
	u32 Count = 0;
	while(Str[Count] != 0) {
		++Count;
	}
	return(Count);
}

void DrawText(u8 *Text, butter_frame_buffer *FrameBuffer, u32 Spacing) {
	u32 Length = strlen(Text);

	for(u32 TextIndex = 0; TextIndex < Length; ++TextIndex) {
		u32 Char = Text[TextIndex] - ' ';

		u32 Row;
		if(Text[TextIndex] > 'G') {
			Row = 1;
		}
		else {
			Row = 0;
		}

		u32 *SourceRow = (u32*)Font.pixel_data + Char*8 + Row*7*Font.width;
		u32 *DestRow = (u32*)FrameBuffer->Base + TextIndex*(8 + Spacing) + FrameBuffer->PixelsPerScanLine*3;
		for(u32 Y = 0; Y < 8; ++Y) {
			u32 *Source = SourceRow;
			u32 *Dest = DestRow;
			for(u32 X = 0; X < 8; ++X) {
				u8 B = (u8)(*Source >> 16);
				u8 G = (u8)(*Source >> 8);
				u8 R = (u8)(*Source);

				if((R + G + B) != 0) {
					*Dest = COLOR_VAL(0xFF, R, G, B);
				}
				++Dest;
				++Source;
			}
			
			SourceRow += Font.width;
			DestRow += FrameBuffer->PixelsPerScanLine;
		}
	}
}

int __attribute__((aligned(0x1000), sysv_abi)) KernelMain(butter_kernel_config *Config) {
	EFI_STATUS Status;

	EFI_FILE *Root = Config->Root;

	NextAllocPage = Config->NextAllocPage;
	PagesLeft = Config->PagesLeft;

	u32 Color = COLOR_VAL(0xFF, 0, 0, 0);

	{
		u32 *Row = (u32*)Config->FrameBuffer->Base;
		for(u32 Y = 0; Y < Config->FrameBuffer->VerticalResolution; ++Y) {
			u32 *Pixel = Row;
			for(u32 X = 0; X < Config->FrameBuffer->HorizontalResolution; ++X) {
				*Pixel++ = Color;
			}

			Row += Config->FrameBuffer->PixelsPerScanLine;
		}

		DrawText("HELLO, MRS. NITSCHKE! THIS IS BUTTEROS!", Config->FrameBuffer, 2);
	}
	for(;;);

	return(0);
}