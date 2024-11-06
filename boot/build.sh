mkdir -p build
pushd build

x86_64-w64-mingw32-gcc -c ../boot_asm.s
x86_64-w64-mingw32-gcc $CFLAGS -DBOOT_PRINT_MEMORY=0 -o boot.o -c ../boot.c
x86_64-w64-mingw32-gcc $LFLAGS -Wl,-dll -shared -Wl,--subsystem,10 -e efi_main -o BOOTX64.EFI boot_asm.o boot.o

x86_64-w64-mingw32-gcc -ggdb3 -O0 -c ../boot_asm.s -o debug_boot_asm.o
x86_64-w64-mingw32-gcc -ggdb3 -O0 $CFLAGS -DBOOT_PRINT_MEMORY=0 -o debug_boot.o -c ../boot.c
x86_64-w64-mingw32-gcc $LFLAGS -Wl,-dll -shared -Wl,--subsystem,10 -e efi_main -o DEBUG_BOOTX64.EFI debug_boot_asm.o debug_boot.o
mkdir -p $SYSROOT/EFI/BOOT
cp BOOTX64.EFI $SYSROOT/EFI/BOOT

popd