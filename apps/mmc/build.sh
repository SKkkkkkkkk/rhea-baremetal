CC=aarch64-none-elf-gcc cmake -S ../.. -B build -G Ninja \
--graphviz=build/graph/dep.dot \
-DPROJECT_NAME=mmc \
-DAPP_CMAKE=app.cmake \
-DMEM_SCHEME=ROM \
-DCMAKE_BUILD_TYPE=Debug
cmake --build build