CC=aarch64-none-elf-gcc cmake -S ../.. -B build -G Ninja --graphviz=build/graph/dep.dot -DPROJECT_NAME=dw_mbox_a2b -DPROJECT_PATH=.
cmake --build build
