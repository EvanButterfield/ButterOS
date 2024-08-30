#include <efi.h>
#include <efilib.h>

#include <kernel/vga.h>

static butter_memory_map MemoryMap;
static butter_frame_buffer FrameBuffer;

#define PRINT(String) ST->ConOut->OutputString(ST->ConOut, (String))

EFI_STATUS EfiMain(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
	EFI_STATUS Status;
	EFI_INPUT_KEY Key;

	ST = SystemTable;
	gBS = ST->BootServices;

	UINTN ClearScreen = FALSE;
	if(ClearScreen) {
		Status = ST->ConOut->ClearScreen(ST->ConOut);
		if(EFI_ERROR(Status)) {
			return(Status);
		}
	}

	Status = ST->ConIn->Reset(ST->ConIn, FALSE);
	if(EFI_ERROR(Status)) {
		return(Status);
	}

	// -----Get FrameBuffer-----
	EFI_GUID GOutGUID = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
	EFI_GRAPHICS_OUTPUT_PROTOCOL *GOutProtocol;
	Status = gBS->LocateProtocol(&GOutGUID, 0, (VOID**)&GOutProtocol);
	if(EFI_ERROR(Status)) {
		PRINT(L"Failed to get graphics output protocol.\r\n");
		return(Status);
	}

	GOutProtocol->SetMode(GOutProtocol, 0);
	FrameBuffer.BytesPerPixel = 8;

	FrameBuffer.Base = GOutProtocol->Mode->FrameBufferBase;
	FrameBuffer.Size = GOutProtocol->Mode->FrameBufferSize;
	FrameBuffer.HorizontalResolution = GOutProtocol->Mode->Info->HorizontalResolution;
	FrameBuffer.VerticalResolution = GOutProtocol->Mode->Info->VerticalResolution;
	FrameBuffer.PixelsPerScanLine = GOutProtocol->Mode->Info->PixelsPerScanLine;

	// ----/Get FrameBuffer/----
	
	// -----Exit Boot Services-----
	Status = gBS->GetMemoryMap(
			&MemoryMap.Size,
			MemoryMap.Map, &MemoryMap.Key,
			&MemoryMap.DescriptorSize, &MemoryMap.DescriptorVersion);
	if(Status != EFI_BUFFER_TOO_SMALL) {
		PRINT(L"Memory map invalid parameter(s).\r\n");
		return(Status);
	}

	MemoryMap.Size += 2 * MemoryMap.DescriptorSize;
	Status = gBS->AllocatePool(EfiLoaderData, MemoryMap.Size, (VOID**)&MemoryMap.Map);
	if(EFI_ERROR(Status)) {
		PRINT(L"Failed to allocate memory map.\r\n");
		return(Status);
	}

	Status = gBS->GetMemoryMap(
			&MemoryMap.Size,
			MemoryMap.Map, &MemoryMap.Key,
			&MemoryMap.DescriptorSize, &MemoryMap.DescriptorVersion);
	if(EFI_ERROR(Status)) {
		PRINT(L"Failed to get memory map.\r\n");
		return(Status);
	}

	Status = gBS->ExitBootServices(ImageHandle, MemoryMap.Key);
	if(EFI_ERROR(Status)) {
		PRINT(L"Failed to exit boot services.\r\n");
		return(Status);
	}
	// ----/Exit Boot Services/----

	UINT32 XOffset = 0;
	UINT32 YOffset = 0;
	for(;;) {
		++XOffset;
		++YOffset;
		UINT32 *Row = (UINT32*)FrameBuffer.Base;
		for(UINT32 Y = 0; Y < FrameBuffer.VerticalResolution; ++Y) {
			UINT32 *Pixel = Row;
			for(UINT32 X = 0; X < FrameBuffer.HorizontalResolution; ++X) {
				*Pixel++ = (XOffset << 24) | (YOffset << 16) | (0 << 8) | (0xFF);
			}

			Row += FrameBuffer.PixelsPerScanLine;
		}
	}

	ST->RuntimeServices->ResetSystem(EfiResetShutdown, EFI_SUCCESS, 0, 0);
	return(Status);
}
