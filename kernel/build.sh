KERNEL_PROJECTS="boot"

for PROJECT in $KERNEL_PROJECTS; do
	(pushd $PROJECT && ./build.sh && popd)
done
