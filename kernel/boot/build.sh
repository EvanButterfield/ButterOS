x86_64-w64-mingw32-gcc $CFLAGS -o boot.o boot.c
x86_64-w64-mingw32-gcc $LFLAGS -Wl,-dll -shared -Wl,--subsystem,10 -e EfiMain -o BOOTX64.EFI boot.o "$GNU_EFI/data.o"
mkdir -p $SYSROOT/EFI/BOOT
cp BOOTX64.EFI $SYSROOT/EFI/BOOT
