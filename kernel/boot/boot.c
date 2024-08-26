#include <efi.h>
#include <efilib.h>

EFI_STATUS EfiMain(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
	EFI_STATUS Status;
	EFI_INPUT_KEY Key;

	ST = SystemTable;

	Status = ST->ConOut->ClearScreen(ST->ConOut);
	if(EFI_ERROR(Status)) {
		return(Status);
	}

	Status = ST->ConOut->OutputString(ST->ConOut, L"Hello, Boot!\r\n");
	if(EFI_ERROR(Status)) {
		return(Status);
	}

	Status = ST->ConIn->Reset(ST->ConIn, FALSE);
	if(EFI_ERROR(Status)) {
		return(Status);
	}

	while((Status = ST->ConIn->ReadKeyStroke(ST->ConIn, &Key)) == EFI_NOT_READY);

	ST->RuntimeServices->ResetSystem(EfiResetShutdown, EFI_SUCCESS, 0, 0);

	return(Status);
}
