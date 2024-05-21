CC=aarch64-none-elf-gcc cmake -S ../.. -B build -G Ninja -DPROJECT_PATH=. -DPROJECT_NAME=spi -DMEM_SCHEME=DRAM -DCMAKE_BUILD_TYPE=Debug
cmake --build build