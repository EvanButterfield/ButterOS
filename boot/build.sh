x86_64-w64-mingw32-gcc $CFLAGS -o boot.o -c boot.c
x86_64-w64-mingw32-gcc $LFLAGS -Wl,-dll -shared -Wl,--subsystem,10 -e efi_main -o BOOTX64.EFI boot.o
mkdir -p $SYSROOT/EFI/BOOT
cp BOOTX64.EFI $SYSROOT/EFI/BOOT
rm *.o *.EFI