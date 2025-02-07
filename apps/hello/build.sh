CC=aarch64-none-elf-gcc cmake -S ../.. -B build -G Ninja --graphviz=build/graph/dep.dot -DPROJECT_NAME=hello -DAPP_CMAKE=app.cmake -DMEM_SCHEME=ROM -DCMAKE_BUILD_TYPE=MinSizeRel
cmake --build build