export BUTTER_DIR="$(pwd)"
export SYSROOT="$BUTTER_DIR/sysroot"
export GNU_EFI="$BUTTER_DIR/gnu-efi"
export CFLAGS="-ffreestanding -I$BUTTER_DIR/kernel/include -I/usr/include/efi -I/usr/include/efi/x86_64 -I/usr/include/efi/protocol -c"
export LFLAGS='-nostdlib'
PROJECTS="boot kernel"

mkdir $SYSROOT &> /dev/null

x86_64-w64-mingw32-gcc $CFLAGS -o gnu-efi/data.o gnu-efi/data.c

for PROJECT in $PROJECTS; do
	(pushd $PROJECT && ./build.sh && popd)
done
