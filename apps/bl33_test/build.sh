CC=aarch64-none-elf-gcc cmake -S ../.. -B build -G Ninja --graphviz=build/graph/dep.dot -DAPP_CMAKE=app.cmake \
-DPROJECT_NAME=bl33_test -DMEM_SCHEME=CUSTOM -DCUSTOM_LINKER_SCRIPT=linker.ld -DCMAKE_BUILD_TYPE=Debug
cmake --build build