CC=aarch64-none-elf-gcc cmake -S ../.. -B build -G Ninja \
--graphviz=build/graph/dep.dot \
-DPROJECT_NAME=axi_dma \
-DPROJECT_PATH=. \
-DMEM_SCHEME=SRAM \
-DCMAKE_BUILD_TYPE=Debug
cmake --build build