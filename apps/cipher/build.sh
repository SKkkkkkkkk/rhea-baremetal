CC=aarch64-none-elf-gcc cmake -S ../.. -B build -G Ninja \
--graphviz=build/graph/dep.dot \
-DPROJECT_NAME=cipher \
-DPROJECT_PATH=. \
-DMEM_SCHEME=DRAM \
-DCMAKE_BUILD_TYPE=Debug
cmake --build build
