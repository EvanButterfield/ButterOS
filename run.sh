qemu-system-x86_64 -m 512M -vga std -bios OVMF.fd -drive format=raw,file=fat:rw:sysroot -d int,cpu_reset -no-reboot -no-shutdown
