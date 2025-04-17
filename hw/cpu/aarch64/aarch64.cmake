set(PRIMARY_CORE "0" CACHE STRING "Primary Core: 0, 1, 2, 3")
set_property(CACHE PRIMARY_CORE PROPERTY STRINGS  0 1 2 3)
set(compile_options_arch 
    -D$CACHE{CPU_ARCH} 
    -DPRIMARY_CORE=$CACHE{PRIMARY_CORE}
    -mlittle-endian
    -mcpu=cortex-a55
    -mtune=cortex-a55
    -I${CMAKE_CURRENT_LIST_DIR}/inc
)

### set target triple for clang
if(${IS_CLANG})
    list(APPEND compile_options_arch -target aarch64-none-elf)
endif()

### set sysroot for clang
if(${IS_CLANG})
    list(APPEND compile_options_arch --sysroot=${SDK_PATH}/libs/arm_gnu/13.2.rel1/sysroot)
endif()

### libgcc
if(${IS_CLANG})
    list(APPEND link_options -L${SDK_PATH}/libs/arm_gnu/13.2.rel1/builtins)
endif()


list(APPEND link_options
    -Wl,--wrap=memcpy -Wl,--wrap=memmove -Wl,--wrap=memset -Wl,--wrap=memcmp -Wl,--wrap=strlen
)

add_compile_options(${compile_options_arch}) # 扩散到所有目标

add_subdirectory(${CMAKE_CURRENT_LIST_DIR})
set(CPU_ARCH_LIB 
    aarch64_startup
    gicv3
)