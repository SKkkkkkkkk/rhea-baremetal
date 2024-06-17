# Generate build system for hello app
# -S specifies the source directory(where Top CMakeLists.txt is located)
# -B specifies the build directory
# -G specifies the generator(Ninja is faster than Unix Makefiles)
# -D specifies the options to pass to CMake
## PROJECT_NAME: Name of the project
## PROJECT_PATH: Path of app.cmake(app.cmake used to specify app sources, headers, etc.)
## MEM_SCHEME: Memory scheme to use(ROM, SRAM, DRAM etc.)
## CMAKE_BUILD_TYPE: Build type(Debug, Release, etc.)
CC=aarch64-none-elf-gcc \
cmake -S ../.. -B build \
-G Ninja \
-DPROJECT_NAME=hello \
-DPROJECT_PATH=. \
-DMEM_SCHEME=ROM \
-DCMAKE_BUILD_TYPE=Release

# Build hello app
cmake --build build

# Note:
## 1. ccmake(or cmake-gui) is a CMake curses interface that allows you to configure project options interactively
##    for specifying options in ccmake, press 'c' to configure, 't' to toggle advanced mode, 'g' to generate, 'q' to quit

## 2. for specifying app sources, headers, etc. u can use -DPROJECT_PATH=/path/to/app.cmake
##    or -DEXTRA_SRCS="/path/to/src1.c;/path/to/src2.c" -DEXTRA_INCS="/path/to/hdr1.h;/path/to/hdr2.h"
##    anyways, -DPROJECT_PATH=/path/to/app.cmake is clean and easy to maintain