CFLAGS='-ffreestanding -I/usr/include/efi -I/usr/include/efi/x86_64 -I/usr/include/efi/protocol -c' 

x86_64-w64-mingw32-gcc $CFLAGS -o hello.o hello.c
x86_64-w64-mingw32-gcc $CFLAGS -o data.o data.c
x86_64-w64-mingw32-gcc -nostdlib -Wl,-dll -shared -Wl,--subsystem,10 -e efi_main -o build/EFI/BOOT/BOOTX64.EFI hello.o data.o
