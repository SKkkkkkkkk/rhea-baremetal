CC=aarch64-none-elf-gcc \
cmake -S ../../.. -B build \
-G Ninja \
-DPROJECT_NAME=scan_spi \
-DPROJECT_PATH=. \
-DMEM_SCHEME=SRAM \
-DCMAKE_BUILD_TYPE=Debug

cmake --build build