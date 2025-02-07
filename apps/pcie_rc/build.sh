CC=aarch64-none-elf-gcc cmake -S ../.. -B build -G Ninja --graphviz=build/graph/dep.dot -DPROJECT_NAME=pcie_ep -DAPP_CMAKE=app.cmake -DBOARD=PLD -DMEM_SCHEME=ROM
cmake --build build
