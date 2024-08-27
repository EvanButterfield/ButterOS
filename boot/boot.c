#include <efi.h>
#include <efilib.h>

UINTN MemoryMapSize = 0;
EFI_MEMORY_DESCRIPTOR *MemoryMap = 0;
UINTN MapKey;
UINTN DescriptorSize;
UINT32 DescriptorVersion;

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

	// -----Exit Boot Services-----
	Status = gBS->GetMemoryMap(
			&MemoryMapSize,
			MemoryMap, &MapKey,
			&DescriptorSize, &DescriptorVersion);
	if(Status != EFI_BUFFER_TOO_SMALL) {
		PRINT(L"Memory map invalid parameter(s).\r\n");
		return(Status);
	}

	MemoryMapSize += 2 * DescriptorSize;
	Status = gBS->AllocatePool(EfiLoaderData, MemoryMapSize, (VOID**)&MemoryMap);
	if(EFI_ERROR(Status)) {
		PRINT(L"Failed to allocate memory map.\r\n");
		return(Status);
	}

	Status = gBS->GetMemoryMap(
			&MemoryMapSize,
			MemoryMap, &MapKey,
			&DescriptorSize, &DescriptorVersion);
	if(EFI_ERROR(Status)) {
		PRINT(L"Failed to get memory map.\r\n");
		return(Status);
	}

	Status = gBS->ExitBootServices(ImageHandle, MapKey);
	if(EFI_ERROR(Status)) {
		PRINT(L"Failed to exit boot services.\r\n");
		return(Status);
	}
	// ----/Exit Boot Services/----

	for(;;);
	ST->RuntimeServices->ResetSystem(EfiResetShutdown, EFI_SUCCESS, 0, 0);
	return(Status);
}
