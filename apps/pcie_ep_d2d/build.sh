CC=aarch64-none-elf-gcc \
cmake -S ../.. -B build -G Ninja \
--graphviz=build/graph/dep.dot \
-DPROJECT_NAME=pcie_ep_d2d \
-DCMAKE_BUILD_TYPE=Release \
-DPROJECT_PATH=. \
-DSELF_DIE_ID=0
cmake --build build -t hex && \
python3 tran2pld.py build/pcie_ep_d2d.hex boot_rom_die0.hex && \
echo "python3 tran2pld.py build/pcie_ep_d2d.hex  boot_rom_die0.hex"

CC=aarch64-none-elf-gcc \
cmake -S ../.. -B build -G Ninja \
--graphviz=build/graph/dep.dot \
-DPROJECT_NAME=pcie_ep_d2d \
-DCMAKE_BUILD_TYPE=Release \
-DPROJECT_PATH=. \
-DSELF_DIE_ID=1
cmake --build build -t hex && \
python3 tran2pld.py build/pcie_ep_d2d.hex boot_rom_die1.hex && \
echo "python3 tran2pld.py build/pcie_ep_d2d.hex  boot_rom_die1.hex"
