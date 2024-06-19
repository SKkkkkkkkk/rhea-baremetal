CC=clang cmake -S ../.. -B build -G Ninja --graphviz=build/graph/dep.dot -DPROJECT_PATH=. -DPROJECT_NAME=gpio
cmake --build build