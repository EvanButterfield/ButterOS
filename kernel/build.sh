gcc $CFLAGS -o kernel.o -c kernel.c -e KernelMain
ld $LFLAGS -static -Bsymbolic -o kernel.exe kernel.o -e KernelMain
mkdir -p "$SYSROOT/kernel"
cp kernel.exe "$SYSROOT/kernel"
rm *.o *.exe