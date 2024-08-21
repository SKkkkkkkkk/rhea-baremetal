CC=aarch64-none-elf-gcc cmake -S ../.. -B build -G Ninja --graphviz=build/graph/dep.dot -DPROJECT_NAME=dw_mbox_a2b -DMEM_SCHEME=DRAM -DPROJECT_PATH=.
cmake --build build
