CC=/sw/tools/arm-gnu-toolchain-13.2.Rel1-x86_64-aarch64-none-elf/bin/aarch64-none-elf-gcc \
cmake -S ../.. -B build -G Ninja \
--graphviz=build/graph/dep.dot \
-DPROJECT_NAME=cmd_client \
-DAPP_CMAKE=app.cmake \
-DPROJECT_PATH=. \
-DMEM_SCHEME=ROM \
-DCMAKE_BUILD_TYPE=Debug \
-DBOARD=ASIC
cmake --build build