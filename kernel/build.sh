x86_64-w64-mingw32-gcc $CFLAGS -o kernel.o kernel.c
x86_64-w64-mingw32-gcc $LFLAGS -o kernel.efi kernel.o "$GNU_EFI/data.o"
mkdir -p "$SYSROOT/kernel"
cp kernel.efi "$SYSROOT/kernel"
