#include <efi.h>
#include <efilib.h>

#include <data.h>

#include <kernel/memory.h>
#include <kernel/vga.h>
#include <kernel/elf.h>
#include <kernel/main.h>
#include <kernel/types.h>
#include <kernel/gdt.h>
#include <kernel/paging.h>

#define INT_08 asm("int $0x8")

static butter_memory_map MemoryMap;
static butter_frame_buffer FrameBuffer;

tss TSS = {0};

gdt GDT = {
	{0x0000, 0, 0, 0x00, 0x00, 0}, // Null
	{0xffff, 0, 0, 0x9a, 0xaf, 0}, // Kernel Code
	{0xffff, 0, 0, 0x92, 0xcf, 0}, // Kernel Data
	{0xffff, 0, 0, 0xfa, 0xaf, 0}, // User Code
	{0xffff, 0, 0, 0xf2, 0xcf, 0}, // User Data
	{0x0000, 0, 0, 0x89, 0x00, 0, 0, 0} // TSS
};

butter_kernel_config KernelConfig;

extern void LoadGDTAndPML4(mapping_table *MappingTable, table_ptr *GDTPtr);
extern void EnableSCE(void);
extern void SetStack(u64 StackTop, butter_kernel_config *Config, u64 KernelEntry);

u64 KernelStack;
u64 KernelStackTop;

#define PRINT(String) ST->ConOut->OutputString(ST->ConOut, (String))

CHAR16 *EfiIntToStr(u64 Value, u64 Base, CHAR16 *Buffer) {
	UINT32 CharIndex = 30;
	for(; Value && CharIndex; --CharIndex, Value /= Base) {
		Buffer[CharIndex] = (L"0123456789abcdef")[Value % Base];
	}

	return(&Buffer[CharIndex + 1]);
}

EFI_STATUS efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
	EFI_STATUS Status;
	EFI_INPUT_KEY Key;

	ST = SystemTable;
	gBS = ST->BootServices;

	PRINT(L"Hello, world!.\n\r");

	UINTN ClearScreen = TRUE;
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

	KernelStackTop = (u64)(KernelStack + sizeof(KernelStack));

	// -----Get FrameBuffer-----
	EFI_GUID GOutGUID = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
	EFI_GRAPHICS_OUTPUT_PROTOCOL *GOutProtocol;
	Status = gBS->LocateProtocol(&GOutGUID, 0, (VOID**)&GOutProtocol);
	if(EFI_ERROR(Status)) {
		PRINT(L"Failed to get graphics output protocol.\n\r");
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

	EFI_LOADED_IMAGE *LoadedImage;
	EFI_GUID LoadedImageProtocolGUID = EFI_LOADED_IMAGE_PROTOCOL_GUID;
	gBS->HandleProtocol(ImageHandle, &LoadedImageProtocol, (void**)&LoadedImage);

	table_ptr GDTPtr = {sizeof(GDT) - 1, (u64)&GDT};

	PRINT(L"Welcome to ButterOS!\r\n");

#if !BOOT_PRINT_MEMORY
	{
		CHAR16 ImageBaseBuffer[33] = {0};
		ImageBaseBuffer[31] = '\n';
		ImageBaseBuffer[32] = '\r';
		CHAR16 *ImageBaseStr = EfiIntToStr((u64)LoadedImage->ImageBase, 16, ImageBaseBuffer);
		ImageBaseStr -= 2;
		ImageBaseStr[0] = '0';
		ImageBaseStr[1] = 'x';
		PRINT(L"Image Base: ");
		PRINT(ImageBaseStr);

		ImageBaseStr = EfiIntToStr((u64)efi_main, 16, ImageBaseBuffer);
		ImageBaseStr -= 2;
		ImageBaseStr[0] = '0';
		ImageBaseStr[1] = 'x';
		PRINT(L"Entry: ");
		PRINT(ImageBaseStr);
	}

	{
		CHAR16 PtrBuffer[33] = {0};
		PtrBuffer[31] = '\n';
		PtrBuffer[32] = '\r';
		CHAR16 *PtrBufferStr = EfiIntToStr((u64)&GDTPtr, 16, PtrBuffer);
		PtrBufferStr -= 2;
		PtrBufferStr[0] = '0';
		PtrBufferStr[1] = 'x';
		PRINT(L"GDT Table Base: ");
		PRINT(PtrBufferStr);

		PtrBufferStr = EfiIntToStr((u64)GDTPtr.Base, 16, PtrBuffer);
		PtrBufferStr -= 2;
		PtrBufferStr[0] = '0';
		PtrBufferStr[1] = 'x';
		PRINT(L"GDT Base: ");
		PRINT(PtrBufferStr);

		PtrBufferStr = EfiIntToStr((u64)&PML4, 16, PtrBuffer);
		PtrBufferStr -= 2;
		PtrBufferStr[0] = '0';
		PtrBufferStr[1] = 'x';
		PRINT(L"PML4 Base: ");
		PRINT(PtrBufferStr);
	}
#endif

	// -----Allocate Kernel Stack-----
	Status = gBS->AllocatePages(AllocateAnyPages, EfiLoaderData, EFI_SIZE_TO_PAGES(STACK_SIZE), &KernelStack);
	if(EFI_ERROR(Status)) {
		PRINT(L"Error allocating kernel stack.\n\r");
		return(Status);
	}

	KernelStackTop = KernelStack + STACK_SIZE;
	// ----/Allocate Kernel Stack/----
	
	// -----Load Kernel Image-----
	EFI_GUID SimpleFileSystemProtocolGUID = EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID;
	EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *FileSystem;
	gBS->HandleProtocol(LoadedImage->DeviceHandle, &SimpleFileSystemProtocolGUID, (VOID**)&FileSystem);

	EFI_FILE *Root;
	FileSystem->OpenVolume(FileSystem, &Root);

	EFI_FILE *KernelEXE;
	Status = Root->Open(Root, &KernelEXE, L"\\kernel\\kernel.exe", EFI_FILE_MODE_READ, 0);
	if(EFI_ERROR(Status)) {
		PRINT(L"Failed to open kernel/kernel.exe.\n\r");
		return(Status);
	}

	EFI_GUID FileInfoGUID = EFI_FILE_INFO_ID;
	UINTN KernelEXEInfoSize = 0;
	Status = KernelEXE->GetInfo(KernelEXE, &FileInfoGUID, &KernelEXEInfoSize, 0);
	if(Status != EFI_BUFFER_TOO_SMALL) {
		PRINT(L"Failed first GetInfo of kernel file.\n\r");
		return(Status);
	}

	EFI_FILE_INFO *KernelEXEInfo;
	Status = gBS->AllocatePool(EfiLoaderData, KernelEXEInfoSize, (VOID**)&KernelEXEInfo);
	if(EFI_ERROR(Status)) {
		PRINT(L"Failed to allocate KernelEXEInfo.\n\r");
		return(Status);
	}

	Status = KernelEXE->GetInfo(KernelEXE, &FileInfoGUID, &KernelEXEInfoSize, KernelEXEInfo);
	if(EFI_ERROR(Status)) {
		PRINT(L"Failed to get info of kernel file.\n\r");
		return(Status);
	}

	VOID *KernelBuffer;
	UINTN KernelBufferPages = (KernelEXEInfo->FileSize + (EFI_PAGE_SIZE - 1)) / EFI_PAGE_SIZE;
	Status = gBS->AllocatePages(AllocateAnyPages, EfiLoaderData, KernelBufferPages, &KernelBuffer);
	if(EFI_ERROR(Status)) {
		PRINT(L"Failed to allocate memory for kernel file.\n\r");
		return(Status);
	}

	Status = KernelEXE->Read(KernelEXE, &KernelEXEInfo->FileSize, KernelBuffer);
	if(EFI_ERROR(Status)) {
		PRINT(L"Error reading KernelEXE.\n\r");
		return(Status);
	}

	gBS->FreePool(KernelEXEInfo);

	Status = KernelEXE->Close(KernelEXE);
	if(EFI_ERROR(Status)) {
		PRINT(L"Error closing KernelEXE.\n\r");
		return(Status);
	}

	elf_header *KernelHeader;
	{
		elf_header *Header = (elf_header*)KernelBuffer;
		KernelHeader = Header;
		if(
				Header->Ident[0] != 0x7f ||
				Header->Ident[1] != 'E'  ||
				Header->Ident[2] != 'L'  ||
				Header->Ident[3] != 'F'  ||
				Header->Ident[4] != 2    ||
				Header->Ident[5] != 1) {
			PRINT(L"Incorrect kernel header.\n\r");
			INT_08;
		}

		if(Header->Type != ET_EXEC) {
			PRINT(L"Kernel isn't an EXEC ELF!\n\r");
			INT_08;
		}

		if(Header->PHNum == 0) {
			PRINT(L"No program segments in kernel image.\n\r");
			INT_08;
		}

		if(Header->Entry == 0) {
			PRINT(L"Kernel has no entry.\n\r");
			INT_08;
		}

		u64 Base = UINTPTR_MAX;

		elf_pheader *PHeader = ELFGetPHeader(Header);
#define MIN(A, B) ((A) < (B) ? (A) : (B))
#define MAX(A, B) ((A) > (B) ? (A) : (B))
		for(u16 PHIndex = 0; PHIndex < Header->PHNum; ++PHIndex) {
			elf_pheader *Program = &PHeader[PHIndex];

			Base = MIN(Base, Program->VAddress);
			continue;

			if(Program->Type == EPHT_LOAD) {
				u64 SizePages = EFI_SIZE_TO_PAGES(Program->MemSize);
				u64 Memory = Program->PAddress;
				Status = gBS->AllocatePages(AllocateAddress, EfiLoaderData, SizePages, &Memory);
				if(EFI_ERROR(Status)) {
					PRINT(L"Error allocating pages for kernel program segment.\r\n");
					INT_08;
				}

				if(Program->FileSize > 0) {
					MemCopy((void*)Memory, (void*)((u64)KernelBuffer + Program->Offset), Program->FileSize);
				}
			}
		}

		u64 ProgramSize = 0;
		for(u16 PHIndex = 0; PHIndex < Header->PHNum; ++PHIndex) {
			elf_pheader *Program = &PHeader[PHIndex];
			u64 SegmentEnd = Program->VAddress - Base + Program->MemSize;
			ProgramSize = MAX(ProgramSize, SegmentEnd);
		}

		for(u16 PHIndex = 0; PHIndex < Header->PHNum; ++PHIndex) {
			elf_pheader *Program = &PHeader[PHIndex];
			void *Memory = (void*)(Base + Program->VAddress);
			MemSet(Memory, 0, Program->MemSize);
			MemCopy(Memory, KernelBuffer + Program->Offset, Program->FileSize);
		}
	}

	{
		CHAR16 PtrBuffer[33] = {0};
		PtrBuffer[31] = '\n';
		PtrBuffer[32] = '\r';
		CHAR16 *PtrStr = EfiIntToStr((u64)KernelBuffer, 16, PtrBuffer);
		PtrStr -= 2;
		PtrStr[0] = '0';
		PtrStr[1] = 'x';
		PRINT(L"Kernel Base: ");
		PRINT(PtrStr);

		PtrStr = EfiIntToStr((u64)KernelHeader->Entry, 16, PtrBuffer);
		PtrStr -= 2;
		PtrStr[0] = '0';
		PtrStr[1] = 'x';
		PRINT(L"Kernel Entry Base: ");
		PRINT(PtrStr);

		PtrStr = EfiIntToStr(KernelStackTop, 16, PtrBuffer);
		PtrStr -= 2;
		PtrStr[0] = '0';
		PtrStr[1] = 'x';
		PRINT(L"Kernel Stack Top: ");
		PRINT(PtrStr);
	}
	// ----/Load Kernel Image/----

	PRINT(L"\n\rPress any key to continue...\n\r");
	EFI_INPUT_KEY InputKey;
	while((Status = ST->ConIn->ReadKeyStroke(ST->ConIn, &InputKey)) == EFI_NOT_READY);

	// -----Exit Boot Services-----
	Status = gBS->GetMemoryMap(
			&MemoryMap.Size,
			MemoryMap.Map, &MemoryMap.Key,
			&MemoryMap.DescriptorSize, &MemoryMap.DescriptorVersion);
	if(Status != EFI_BUFFER_TOO_SMALL) {
		PRINT(L"Memory map invalid parameter(s).\n\r");
		return(Status);
	}

	MemoryMap.Size += 2 * MemoryMap.DescriptorSize;
	Status = gBS->AllocatePool(EfiLoaderData, MemoryMap.Size, (VOID**)&MemoryMap.Map);
	if(EFI_ERROR(Status)) {
		PRINT(L"Failed to allocate memory map.\n\r");
		return(Status);
	}

	Status = gBS->GetMemoryMap(
			&MemoryMap.Size,
			MemoryMap.Map, &MemoryMap.Key,
			&MemoryMap.DescriptorSize, &MemoryMap.DescriptorVersion);
	if(EFI_ERROR(Status)) {
		PRINT(L"Failed to get memory map.\n\r");
		return(Status);
	}

#if BOOT_PRINT_MEMORY
	PRINT(L"Conventional memory descriptors:\n\r");
#endif

	{
		u64 DescriptorBase = (u64)MemoryMap.Map;
		CHAR16 DescSizeBuffer[36] = {0};
		DescSizeBuffer[31] = 'K';
		DescSizeBuffer[32] = 'B';
		DescSizeBuffer[33] = '\n';
		DescSizeBuffer[34] = '\r';

		u64 BestDescStart = 0;
		u64 BestDescPages = 0;
		u64 AllKB = 0;
		for(UINTN MemoryIndex = 0; MemoryIndex < MemoryMap.Size; MemoryIndex += MemoryMap.DescriptorSize) {
			EFI_MEMORY_DESCRIPTOR *Descriptor = (EFI_MEMORY_DESCRIPTOR*)(DescriptorBase + MemoryIndex);
			if(Descriptor->Type != EfiConventionalMemory) {
				continue;
			}

			if(Descriptor->NumberOfPages > BestDescPages) {
				BestDescStart = Descriptor->PhysicalStart;
				BestDescPages = Descriptor->NumberOfPages;
			}

			u64 SizeKB = EFI_PAGES_TO_SIZE(Descriptor->NumberOfPages) / 1024;
			AllKB += SizeKB;

#if BOOT_PRINT_MEMORY
			u64 StartBuffer[34] = {0};
			// StartBuffer[31] = ',';
			// StartBuffer[32] = ' ';
			CHAR16 *PhysStartStr = EfiIntToStr(Descriptor->PhysicalStart, 16, StartBuffer);
			PRINT(PhysStartStr);
			PhysStartStr -= 2;
			PhysStartStr[0] = '0';
			PhysStartStr[2] = 'x';

			CHAR16 *SizeKBStr = EfiIntToStr(SizeKB, 10, DescSizeBuffer);
			PRINT(SizeKBStr);
#endif
		}
		PagesLeft = BestDescPages;
		NextAllocPage = BestDescStart;

#if BOOT_PRINT_MEMORY
		CHAR16 *SizeKBStr = EfiIntToStr(AllKB, 10, DescSizeBuffer);
		PRINT(L"\n\rAll conventional memory: ");
		PRINT(SizeKBStr);

		u64 BestKB = EFI_PAGES_TO_SIZE(BestDescPages) / 1024;
		CHAR16 *BestKBStr = EfiIntToStr(BestKB, 10, DescSizeBuffer);
		PRINT(L"\n\rBest allocation size: ");
		PRINT(BestKBStr);

		DescSizeBuffer[31] = 0;
		CHAR16 *StartStr = EfiIntToStr(BestDescStart, 16, DescSizeBuffer);
		PRINT(L"Best allocation start: 0x");
		PRINT(StartStr);
		
		for(;;);
#endif
	}

	Status = gBS->ExitBootServices(ImageHandle, MemoryMap.Key);
	if(EFI_ERROR(Status)) {
		PRINT(L"Failed to exit boot services.\n\r");
		return(Status);
	}
	// ----/Exit Boot Services/----

	// -----Load GDT-----
	u64 TSSBase = (u64)&TSS;
	GDT.TSS.Base15_0 = (u16)(TSSBase & 0xffff);
	GDT.TSS.Base23_16 = (u8)((TSSBase >> 16) & 0xff);
	GDT.TSS.Base31_24 = (u8)((TSSBase >> 24) & 0xff);
	GDT.TSS.Base63_32 = (u32)((TSSBase >> 32) & 0xffffffff);
	GDT.TSS.Limit15_0 = sizeof(TSS) - 1;

	TSS.IOPB = sizeof(TSS);
	TSS.RSP0 = KernelStackTop;

	u64 DescriptorBase = (u64)MemoryMap.Map;
	for(UINTN MemoryIndex = 0; MemoryIndex < MemoryMap.Size; MemoryIndex += MemoryMap.DescriptorSize) {
		EFI_MEMORY_DESCRIPTOR *Descriptor = (EFI_MEMORY_DESCRIPTOR*)(DescriptorBase + MemoryIndex);
		u64 Page = Descriptor->PhysicalStart;
		KMapPages(Page, Page, Descriptor->NumberOfPages, PAGE_IDENTITY);
		for(u64 Page = Descriptor->PhysicalStart; Page < Descriptor->PhysicalStart + Descriptor->NumberOfPages * PAGE_SIZE; Page += PAGE_SIZE) {
			KMapPages(Page, Page, 1, PAGE_IDENTITY);
		}
	}

	for(u64 StackAddress = (u64)&KernelStack; StackAddress < (u64)&KernelStack + STACK_SIZE; StackAddress += 0x1000) {
		KMapPages(StackAddress, StackAddress, 1, PAGE_IDENTITY);
	}

	// KMapPages(KernelHeader->Entry, KernelHeader->Entry, )

	LoadGDTAndPML4(&PML4, &GDTPtr);
	EnableSCE();
	// ----/Load GDT/----

	KernelConfig.FrameBuffer = &FrameBuffer;
	KernelConfig.MemoryMap = &MemoryMap;
	KernelConfig.Root = Root;

	SetStack(KernelStackTop, &KernelConfig, KernelHeader->Entry);
	// Should never reach here
	return(0);
}
