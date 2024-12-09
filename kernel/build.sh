mkdir -p build
pushd build

gcc $CFLAGS -mno-red-zone -o kernel.o -c ../kernel.c -e KernelMain
ld $LFLAGS -static -o kernel.exe kernel.o -e KernelMain
mkdir -p "$SYSROOT/kernel"
cp kernel.exe "$SYSROOT/kernel"

popd