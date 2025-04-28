CC=aarch64-none-elf-gcc cmake -S ../.. -B build -G Ninja --graphviz=build/graph/dep.dot -DAPP_CMAKE=app.cmake -DPROJECT_NAME=gtimer -DCMAKE_BUILD_TYPE=Debug
cmake --build build