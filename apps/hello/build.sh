CROSS_COMPILE=aarch64-none-elf- cmake -S ../.. -B build -G Ninja --graphviz=build/graph/dep.dot -DPROJECT_NAME=hello -DPROJECT_PATH=.
cmake --build build