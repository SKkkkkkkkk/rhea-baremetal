CC=aarch64-none-elf-gcc cmake -S ../.. -B build -G Ninja --graphviz=build/graph/dep.dot \
-DPROJECT_NAME=freertos -DAPP_CMAKE=app.cmake -DCMAKE_BUILD_TYPE=Debug \
-DMEM_SCHEME=CUSTOM -DCUSTOM_LINKER_SCRIPT=linker.ld -DPRIMARY_CORE=3
cmake --build build