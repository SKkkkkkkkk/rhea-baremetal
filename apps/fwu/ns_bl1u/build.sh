CC=clang cmake -S ../../../ -B build -G Ninja --graphviz=build/graph/dep.dot \
-DPROJECT_NAME=ns_bl1u -DPROJECT_PATH=. \
-DCMAKE_BUILD_TYPE=Release \
-DENABLE_LTO=ON \
-DMEM_SCHEME=CUSTOM -DCUSTOM_LINKER_SCRIPT=linker.ld
cmake --build build