CC=clang cmake -S ../.. -B build -G Ninja --graphviz=build/graph/dep.dot -DAPP_CMAKE=app.cmake -DPROJECT_NAME=gpio
cmake --build build