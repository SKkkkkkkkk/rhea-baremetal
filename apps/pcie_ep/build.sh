CC=aarch64-none-elf-gcc cmake -S ../.. -B build -G Ninja --graphviz=build/graph/dep.dot -DPROJECT_NAME=pcie_ep -DPROJECT_PATH=. -DCMAKE_BUILD_TYPE=Debug -DMEM_SCHEME=DRAM
cmake --build build
