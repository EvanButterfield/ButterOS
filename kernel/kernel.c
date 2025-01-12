#include <kernel/main.h>
#include <kernel/types.h>
#include <kernel/paging.h>
#include <kernel/string.h>

#include <kernel/font.h>

#define COLOR_VAL(A, B, C, D) ((A) << 24 | (B) << 16 | (C) << 8 | (D))
#define TTF_FLAG(A, B, C, D) COLOR_VAL((A), (B), (C), (D))

static EFI_FILE *Root;
static memory_map MemoryMap;
static frame_buffer FrameBuffer;

typedef struct memory_segment {
	u64 Address;
	u64 Length;
	b32 Reserved;
} memory_segment;

#define MAX_MEM_SEGMENTS 2048
memory_segment MemSegments[MAX_MEM_SEGMENTS];
static u64 NumMemSegments = 1;

// TODO(evan): Make this put segments in order.
// We wouldn't even need addresses
void *KMalloc(u64 Length) {
	for(u64 SegIndex = 0; SegIndex < NumMemSegments; ++SegIndex) {
		if(!MemSegments[SegIndex].Reserved &&
		   MemSegments[SegIndex].Length > Length) {
			u64 LengthDifference = MemSegments[SegIndex].Length - Length;
			
			MemSegments[SegIndex].Length = Length;
			MemSegments[SegIndex].Reserved = TRUE;

			if(LengthDifference != 0) {
				MemSegments[NumMemSegments].Address = MemSegments[SegIndex].Address + Length;
				MemSegments[NumMemSegments].Length = LengthDifference;
				MemSegments[NumMemSegments].Reserved = FALSE;
				++NumMemSegments;
			}

			return((void*)MemSegments[SegIndex].Address);
		}
	}
}

// TODO(evan): This won't combine freed sections.
// Fixing the above problem will solve this, however
void KFree(u64 Address) {
	for(u64 SegIndex = 0; SegIndex < NumMemSegments; ++SegIndex) {
		if(MemSegments[SegIndex].Address == Address) {
			MemSegments[SegIndex].Reserved = FALSE;
		}
	}
}

void DrawText(u8 *Text, u64 Spacing, u64 XPos, u64 YPos) {
	u64 Length = StringUpperCase(Text);

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
		u32 *DestRow = (u32*)FrameBuffer.Base + XPos + TextIndex*(8 + Spacing) + FrameBuffer.PixelsPerScanLine*YPos;
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
			DestRow += FrameBuffer.PixelsPerScanLine;
		}
	}
}

int __attribute__((aligned(0x1000), sysv_abi)) KernelMain(butter_kernel_config *Config) {
	EFI_STATUS Status;

	Root = Config->Root;

	NextAllocPage = Config->NextAllocPage;
	PagesLeft = Config->PagesLeft;

	for(u64 SegIndex = 0; SegIndex < MAX_MEM_SEGMENTS; ++SegIndex) {
		MemSegments[SegIndex] = (memory_segment){0};
	}
	MemSegments[0].Address = Config->NextAllocPage;
	MemSegments[0].Length = Config->PagesLeft*PAGE_SIZE;

	u32 Color = COLOR_VAL(0xFF, 0, 0, 0);

	MemoryMap = *Config->MemoryMap;
	FrameBuffer = *Config->FrameBuffer;

	{
		u32 *Row = (u32*)FrameBuffer.Base;
		for(u32 Y = 0; Y < FrameBuffer.VerticalResolution; ++Y) {
			u32 *Pixel = Row;
			for(u32 X = 0; X < FrameBuffer.HorizontalResolution; ++X) {
				*Pixel++ = Color;
			}

			Row += FrameBuffer.PixelsPerScanLine;
		}

		DrawText("Hello, welcome to ButterOS!", 2, 3, 3);
	}
	for(;;);

	return(0);
}