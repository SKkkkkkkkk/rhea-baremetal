CC=aarch64-none-elf-gcc cmake -S ../.. -B build -G Ninja --graphviz=build/graph/dep.dot -DPROJECT_NAME=pcie_ep -DAPP_CMAKE=app.cmake -DCMAKE_BUILD_TYPE=Debug -DMEM_SCHEME=ROM -DBOARD=PLD
cmake --build build
