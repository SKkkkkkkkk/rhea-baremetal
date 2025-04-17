CC=aarch64-none-elf-gcc cmake -S ../.. -B build -G Ninja --graphviz=build/graph/dep.dot -DPROJECT_NAME=pcie_rc -DAPP_CMAKE=app.cmake -DCMAKE_BUILD_TYPE=Debug -DBOARD=PLD -DMEM_SCHEME=ROM
cmake --build build
