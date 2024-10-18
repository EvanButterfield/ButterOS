find . -name "*.*~" -type f -delete

export BUTTER_DIR="$(pwd)"
export SYSROOT="$BUTTER_DIR/sysroot"
export GNU_EFI="$BUTTER_DIR/gnu-efi"
export CFLAGS="-ffreestanding -I$GNU_EFI -I$BUTTER_DIR/kernel/include -I/usr/include/efi -I/usr/include/efi/x86_64 -I/usr/include/efi/protocol -Wno-int-to-pointer-cast -Wno-pointer-to-int-cast -Wno-incompatible-pointer-types"
export LFLAGS="-nostdlib"
PROJECTS="boot kernel"

mkdir $SYSROOT &> /dev/null

for PROJECT in $PROJECTS; do
	(pushd $PROJECT && ./build.sh && popd)
done