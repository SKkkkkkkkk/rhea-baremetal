qemu-system-aarch64 \
-M virt,secure=on,virtualization=on,gic-version=3 \
-nographic \
-cpu cortex-a55 \
-m 3G \
-smp 4 \
-d unimp \
-semihosting-config enable=on,target=native \
-bios build/gicv3.bin \
-S -s