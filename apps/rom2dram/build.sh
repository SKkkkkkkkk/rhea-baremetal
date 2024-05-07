# CROSS_COMPILE=aarch64-none-elf- cmake -S ../.. -B build -G Ninja --graphviz=build/graph/dep.dot \
# -DPROJECT_NAME=hello -DPROJECT_PATH=.
CROSS_COMPILE=aarch64-none-elf- cmake -S ../.. -B build -G Ninja --graphviz=build/graph/dep.dot \
-DCMAKE_BUILD_TYPE=Release -DMEM_SCHEME=ROM \
-DPROJECT_NAME=rom2dram -DEXTRA_SRCS="src/main.c;src/1.c" -DEXTRA_INCS="inc;inc1"
cmake --build build