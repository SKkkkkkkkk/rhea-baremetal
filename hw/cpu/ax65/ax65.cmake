set(compile_options_arch 
    -D${CPU_ARCH} 
    -march=rv64gc 
    -mabi=lp64d
    -mcmodel=medany
)
add_compile_options(${compile_options_arch}) # 扩散到所有目标

add_subdirectory(${CMAKE_CURRENT_LIST_DIR})
set(CPU_ARCH_LIB ax65_startup)