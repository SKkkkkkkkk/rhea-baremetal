CC=aarch64-none-elf-gcc cmake -S ../.. -B build -G Ninja --graphviz=build/graph/dep.dot \
-DPROJECT_NAME=rom2dram -DPROJECT_PATH=.
cmake --build build