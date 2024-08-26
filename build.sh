export SYSROOT="$(pwd)/sysroot"
PROJECTS="kernel"

mkdir $SYSROOT &> /dev/null

for PROJECT in $PROJECTS; do
	(pushd $PROJECT && ./build.sh && popd)
done
