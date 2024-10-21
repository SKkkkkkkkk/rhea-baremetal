ARCH=arm64 CC=/sw/tools/arm-gnu-toolchain-13.2.Rel1-x86_64-aarch64-none-elf/bin/aarch64-none-elf-gcc cmake -S ../.. -B build -G Ninja --graphviz=build/graph/dep.dot -DPROJECT_NAME=pcie_ep_d2d -DPROJECT_PATH=.
cmake --build build -t hex && \
python3 tran2pld.py build/pcie_ep_d2d.hex && \
echo "python3 tran2pld.py build/pcie_ep_d2d.hex"
