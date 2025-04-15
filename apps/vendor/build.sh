CC=aarch64-none-elf-gcc cmake -S ../.. -B build -G Ninja \
--graphviz=build/graph/dep.dot \
-DPROJECT_NAME=vendor \
-DAPP_CMAKE=app.cmake \
-DMEM_SCHEME=CUSTOM \
-DCUSTOM_LINKER_SCRIPT=linker.ld \
-DCMAKE_BUILD_TYPE=Debug
cmake --build build