CROSS_COMPILE=aarch64-none-elf- cmake -S ../.. -B build -G Ninja --graphviz=build/graph/dep.dot -DPROJECT_PATH:PATH=. -DPROJECT_NAME=timer
cmake --build build