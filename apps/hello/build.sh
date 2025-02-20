CC=riscv64-unknown-elf-gcc \
cmake \
-S ../.. -B build \
-G Ninja \
--graphviz=build/graph/dep.dot \
-DPROJECT_NAME=hello \
-DAPP_CMAKE=app.cmake \
-DCPU_ARCH=C920 \
-DMEM_SCHEME=ROM \
-DCMAKE_BUILD_TYPE=Debug
cmake --build build