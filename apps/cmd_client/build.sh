CC=aarch64-none-elf-gcc \
cmake -S ../.. -B build -G Ninja \
--graphviz=build/graph/dep.dot \
-DPROJECT_NAME=cmd_client \
-DAPP_CMAKE=app.cmake \
-DMEM_SCHEME=ROM \
-DCMAKE_BUILD_TYPE=Debug \
-DBOARD=ASIC
cmake --build build