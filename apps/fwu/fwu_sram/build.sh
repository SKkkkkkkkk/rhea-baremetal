CC=clang cmake -S ../../../ -B build -G Ninja \
--graphviz=build/graph/dep.dot \
-DPROJECT_NAME=fwu_sram \
-DAPP_CMAKE=app.cmake \
-DCMAKE_BUILD_TYPE=MinSizeRel \
-DENABLE_LTO=ON \
-DMEM_SCHEME=CUSTOM -DCUSTOM_LINKER_SCRIPT=linker.ld
cmake --build build