CC=aarch64-none-elf-gcc cmake -S ../.. -B build -G Ninja \
--graphviz=build/graph/dep.dot \
-DPROJECT_NAME=pwm \
-DAPP_CMAKE=app.cmake \
-DMEM_SCHEME=SRAM \
-DCMAKE_BUILD_TYPE=Debug
cmake --build build