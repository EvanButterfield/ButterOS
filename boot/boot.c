#include <efi.h>
#include <efilib.h>

#include <data.h>

#include <kernel/memory.h>
#include <kernel/vga.h>
#include <kernel/elf.h>
#include <kernel/functions.h>
#include <kernel/main.h>

static butter_memory_map MemoryMap;
static butter_frame_buffer FrameBuffer;

UINT64 NextAllocPage;

#define PRINT(String) ST->ConOut->OutputString(ST->ConOut, (String))

CHAR16 *EfiIntToStr(UINT64 Value, UINT64 Base, CHAR16 *Buffer) {
	UINT32 CharIndex = 30;
	for(; Value && CharIndex; --CharIndex, Value /= Base) {
		Buffer[CharIndex] = (L"0123456789abcdef")[Value % Base];
	}

	return(&Buffer[CharIndex + 1]);
}

void *EfiAllocPage() {
	void *Page = (void*)NextAllocPage;
	NextAllocPage += EFI_PAGE_SIZE;
	return(Page);
}

EFI_STATUS efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
	EFI_STATUS Status;
	EFI_INPUT_KEY Key;

	ST = SystemTable;
	gBS = ST->BootServices;

	// InitializeLib(ImageHandle, SystemTable);
	PRINT(L"Hello, world!.\r\n");

	UINTN ClearScreen = FALSE;
	if(ClearScreen) {
		Status = ST->ConOut->ClearScreen(ST->ConOut);
		if(EFI_ERROR(Status)) {
			return(Status);
		}
		
		Status = ST->ConOut->SetCursorPosition(ST->ConOut, 0, 0);
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
	
	// -----Load Kernel Image-----
	EFI_GUID LoadedImageProtocolGUID = EFI_LOADED_IMAGE_PROTOCOL_GUID;
	EFI_LOADED_IMAGE_PROTOCOL *LoadedImage;
	gBS->HandleProtocol(ImageHandle, &LoadedImageProtocolGUID, (VOID**)&LoadedImage);

	EFI_GUID SimpleFileSystemProtocolGUID = EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID;
	EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *FileSystem;
	gBS->HandleProtocol(LoadedImage->DeviceHandle, &SimpleFileSystemProtocolGUID, (VOID**)&FileSystem);

	EFI_FILE *Root;
	FileSystem->OpenVolume(FileSystem, &Root);

	EFI_FILE *KernelEXE;
	Status = Root->Open(Root, &KernelEXE, L"\\kernel\\kernel.exe", EFI_FILE_MODE_READ, 0);
	if(EFI_ERROR(Status)) {
		PRINT(L"Failed to open kernel/kernel.exe.\r\n");
		return(Status);
	}

	EFI_GUID FileInfoGUID = EFI_FILE_INFO_ID;
	UINTN KernelEXEInfoSize = 0;
	Status = KernelEXE->GetInfo(KernelEXE, &FileInfoGUID, &KernelEXEInfoSize, 0);
	if(Status != EFI_BUFFER_TOO_SMALL) {
		PRINT(L"Failed first GetInfo of kernel file.\r\n");
		return(Status);
	}

	EFI_FILE_INFO *KernelEXEInfo;
	Status = gBS->AllocatePool(EfiLoaderData, KernelEXEInfoSize, (VOID**)&KernelEXEInfo);
	if(EFI_ERROR(Status)) {
		PRINT(L"Failed to allocate KernelEXEInfo.\r\n");
		return(Status);
	}

	Status = KernelEXE->GetInfo(KernelEXE, &FileInfoGUID, &KernelEXEInfoSize, KernelEXEInfo);
	if(EFI_ERROR(Status)) {
		PRINT(L"Failed to get info of kernel file.\r\n");
		return(Status);
	}

	VOID *KernelBuffer;
	Status = gBS->AllocatePool(EfiLoaderData, KernelEXEInfo->FileSize, &KernelBuffer);
	if(EFI_ERROR(Status)) {
		PRINT(L"Failed to allocate memory for kernel file.\r\n");
		return(Status);
	}

	Status = KernelEXE->Read(KernelEXE, &KernelEXEInfo->FileSize, KernelBuffer);
	if(EFI_ERROR(Status)) {
		PRINT(L"Error reading KernelEXE.\r\n");
		return(Status);
	}

	gBS->FreePool(KernelEXEInfo);

	Status = KernelEXE->Close(KernelEXE);
	if(EFI_ERROR(Status)) {
		PRINT(L"Error closing KernelEXE.\r\n");
		return(Status);
	}

	Status = ST->ConOut->SetCursorPosition(ST->ConOut, 0, 0);
	if(EFI_ERROR(Status)) {
		return(Status);
	}

	EFI_PHYSICAL_ADDRESS KernelEntryAddress;
	butter_elf_header *KernelHeader;
	UINT64 LastKernelSegmentAddress;
	UINT64 LastKernelSegmentSize;
	{
		butter_elf_header *Header = (butter_elf_header*)KernelBuffer;
		if(
				Header->Ident[0] != 0x7f ||
				Header->Ident[1] != 'E'  ||
				Header->Ident[2] != 'L'  ||
				Header->Ident[3] != 'F'  ||
				Header->Ident[4] != 2    ||
				Header->Ident[5] != 1) {
			PRINT(L"Incorrect kernel header.\r\n");
			return(0);
		}

		if(Header->Type == ET_REL) {
			PRINT(L"REL kernel unsupported.\r\n");
			return(0);
		}

		if(Header->PHNum == 0) {
			PRINT(L"No program segments in kernel image.\r\n");
			return(0);
		}

		if(Header->Entry == 0) {
			PRINT(L"Kernel has no entry.\r\n");
			return(0);
		}

		KernelHeader = Header;

		KernelEntryAddress = (EFI_PHYSICAL_ADDRESS)((UINT64)KernelBuffer + Header->Entry);

		butter_elf_pheader *PHeaders = ELFGetPHeader(Header);
		UINT32 SegmentsLoaded = 0;
		for(UINT32 PHIndex = 0; PHIndex < Header->PHNum; ++PHIndex) {
			butter_elf_pheader PHeader = PHeaders[PHIndex];
			if(PHeader.Type == EPHT_LOAD) {
				UINT64 Offset = PHeader.Offset;
				UINT64 FileSize = PHeader.FileSize;
				UINT64 MemSize = PHeader.MemSize;
				UINT64 PAddress = PHeader.PAddress;
				LastKernelSegmentAddress = PAddress;
				LastKernelSegmentSize = FileSize;
				
				UINT64 PageCount = EFI_SIZE_TO_PAGES(FileSize);
				Status = gBS->AllocatePages(AllocateAddress, EfiLoaderData, PageCount, (EFI_PHYSICAL_ADDRESS*)&PAddress);
				if(EFI_ERROR(Status)) {
					PRINT(L"Error allocating memory for kernel program header.\r\n");
					return(0);
				}

				if(FileSize > 0) {
					VOID *ProgramData = (VOID*)((UINT64)Header + Offset);
					gBS->CopyMem((VOID*)PAddress, ProgramData, FileSize);
				}

				++SegmentsLoaded;
			}
		}
	}

	// gBS->FreePool(KernelBuffer);

	Status = ST->ConOut->SetCursorPosition(ST->ConOut, 0, 0);
	if(EFI_ERROR(Status)) {
		return(Status);
	}
	// ----/Load Kernel Image/----
	
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

#if BOOT_PRINT_MEMORY
	PRINT(L"Conventional memory descriptors:\n\r");
#endif

	UINT64 DescriptorBase = (UINT64)MemoryMap.Map;
	CHAR16 DescSizeBuffer[36] = {0};
	DescSizeBuffer[31] = 'K';
	DescSizeBuffer[32] = 'B';
	DescSizeBuffer[33] = '\n';
	DescSizeBuffer[34] = '\r';

	UINT64 BestDescStart = 0;
	UINT64 BestDescPages = 0;
	UINT64 AllKB = 0;
	for(UINTN MemoryIndex = 0; MemoryIndex < MemoryMap.Size; MemoryIndex += MemoryMap.DescriptorSize) {
		EFI_MEMORY_DESCRIPTOR *Descriptor = (EFI_MEMORY_DESCRIPTOR*)(DescriptorBase + MemoryIndex);
		if(Descriptor->Type != EfiConventionalMemory) {
			continue;
		}

		if(Descriptor->NumberOfPages > BestDescPages) {
			BestDescStart = Descriptor->PhysicalStart;
			BestDescPages = Descriptor->NumberOfPages;
		}

		UINT64 SizeKB = EFI_PAGES_TO_SIZE(Descriptor->NumberOfPages) / 1024;
		AllKB += SizeKB;

#if BOOT_PRINT_MEMORY
		CHAR16 *SizeKBStr = EfiIntToStr(SizeKB, 10, DescSizeBuffer);
		PRINT(SizeKBStr);
#endif
	}
	NextAllocPage = BestDescStart;

#if BOOT_PRINT_MEMORY
	{
		CHAR16 *SizeKBStr = EfiIntToStr(AllKB, 10, DescSizeBuffer);
		PRINT(L"\n\rAll conventional memory: ");
		PRINT(SizeKBStr);

		UINT64 BestKB = EFI_PAGES_TO_SIZE(BestDescPages) / 1024;
		CHAR16 *BestKBStr = EfiIntToStr(BestKB, 10, DescSizeBuffer);
		PRINT(L"\n\rBest allocation size: ");
		PRINT(BestKBStr);

		DescSizeBuffer[31] = 0;
		CHAR16 *StartStr = EfiIntToStr(BestDescStart, 16, DescSizeBuffer);
		PRINT(L"Best allocation start: 0x");
		PRINT(StartStr);
	}

	for(;;);
#endif

	Status = gBS->ExitBootServices(ImageHandle, MemoryMap.Key);
	if(EFI_ERROR(Status)) {
		PRINT(L"Failed to exit boot services.\r\n");
		return(Status);
	}
	// ----/Exit Boot Services/----

	butter_kernel_config KernelConfig = {
		&FrameBuffer,
		&MemoryMap,
		Root
	};

	kernel_main KernelMain = (kernel_main)KernelHeader->Entry;
	int KernelStatus = KernelMain(&KernelConfig);
	ST->RuntimeServices->ResetSystem(EfiResetShutdown, EFI_SUCCESS, 0, 0);
	return(KernelStatus);
}
