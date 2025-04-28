CC=aarch64-none-elf-gcc cmake -S ../.. -B build -G Ninja --graphviz=build/graph/dep.dot -DPROJECT_NAME=pcie_ep_hdma -DAPP_CMAKE=app.cmake -DCMAKE_BUILD_TYPE=Debug -DMEM_SCHEME=ROM -DBOARD=PLD
cmake --build build -t hex
python3 tran2pld.py build/pcie_ep_hdma.hex
