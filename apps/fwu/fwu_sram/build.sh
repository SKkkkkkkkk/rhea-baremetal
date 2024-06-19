CC=aarch64-none-elf-gcc cmake -S ../../../ -B build -G Ninja \
--graphviz=build/graph/dep.dot \
-DPROJECT_NAME=fwu_sram \
-DPROJECT_PATH=. \
-DBOARD=FPGA \
-DMEM_SCHEME=SRAM \
-DCMAKE_BUILD_TYPE=Debug
cmake --build build