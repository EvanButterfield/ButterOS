CFLAGS='-ffreestanding -I/usr/include/efi -I/usr/include/efi/x86_64 -I/usr/include/efi/protocol -c'

x86_64-w64-mingw32-gcc $CFLAGS -o boot.o boot.c
x86_64-w64-mingw32-gcc $CFLAGS -o efi/data.o efi/data.c
x86_64-w64-mingw32-gcc -nostdlib -Wl,-dll -shared -Wl,--subsystem,10 -e EfiMain -o BOOTX64.EFI boot.o efi/data.o
mkdir -p $SYSROOT/EFI/BOOT
mv BOOTX64.EFI $SYSROOT/EFI/BOOT
