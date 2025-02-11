cmake_minimum_required(VERSION 3.15...3.28)
include(cmake_helper/common/out_of_source.cmake)
include(cmake_helper/cross_compile/toolchain_file.cmake)
include(cmake_helper/common/custom_build_type.cmake)
set(CMAKE_INSTALL_PREFIX ${CMAKE_BINARY_DIR}/install CACHE INTERNAL "Install path")

set(SDK_PATH ${CMAKE_SOURCE_DIR})

set(PROJECT_NAME "a" CACHE STRING "Project Name")

set(APP_CMAKE "" CACHE FILEPATH "Path to the app.cmake") # since the type is PATH, set comman will convert it to absolute automatically.
if("$CACHE{APP_CMAKE}" STREQUAL "")
	message(FATAL_ERROR "APP_CMAKE must be set!")
endif()

set(BOARD "FPGA" CACHE STRING "QEMU, RTL, FPGA, EVB, PLD")
set_property(CACHE BOARD PROPERTY STRINGS "QEMU" "RTL" "FPGA" "EVB" "PLD")

set(MEM_SCHEME "ROM" CACHE STRING "ROM, SRAM, DRAM, CUSTOM")
set_property(CACHE MEM_SCHEME PROPERTY STRINGS "ROM" "SRAM" "DRAM" "CUSTOM")
if($CACHE{MEM_SCHEME} STREQUAL "CUSTOM")
	set(CUSTOM_LINKER_SCRIPT "" CACHE FILEPATH "Custom Linker Script file")
	if("$CACHE{CUSTOM_LINKER_SCRIPT}" STREQUAL "")
		message(FATAL_ERROR "CUSTOM_LINKER_SCRIPT must be set, with MEM_SCHEME=CUSTOM")
	endif()
else()
	set(STACK_SIZE "0x4000" CACHE STRING "Stack Size(In Bytes)")
	set(HEAP_SIZE  "0x4000" CACHE STRING "Heap Size(In Bytes)")
endif()

set(ENABLE_LTO "OFF" CACHE BOOL "Enable Link Time Optimization")

set(ENABLE_MMU "OFF" CACHE BOOL "Enable MMU")

set(PRIMARY_CORE "0" CACHE STRING "Primary Core: 0, 1, 2, 3")
set_property(CACHE PRIMARY_CORE PROPERTY STRINGS  0 1 2 3)

project("${PROJECT_NAME}"
	VERSION 0.1.0
	DESCRIPTION "${PROJECT_NAME}"
	LANGUAGES C ASM
)
# for all targets

## common
set(compile_options_common
	-Wall -Werror -Wno-unused-variable -Wno-unused-function
	-ffunction-sections -fdata-sections
	-mstrict-align
	$<$<BOOL:${ENABLE_LTO}>:-flto>
	-DA55 -D$CACHE{BOARD} -DPRIMARY_CORE=$CACHE{PRIMARY_CORE}
	$<$<BOOL:${ENABLE_MMU}>:-DENABLE_MMU>
	-I${SDK_PATH}/common/inc
	-I${SDK_PATH}/hw/cpu/aarch64/inc # aarch64 helper
)

set(link_options_common
	-static
	-nostdlib # we will explicitly link libc.a, libm.a and libgcc.a later
	$<$<BOOL:${ENABLE_LTO}>:-flto>
	-Wl,--fatal-warnings
	-Wl,--gc-sections
	-Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/$CACHE{PROJECT_NAME}.map
	-Wl,--print-memory-usage
	-Wl,--wrap=memcpy -Wl,--wrap=memmove -Wl,--wrap=memset -Wl,--wrap=memcmp -Wl,--wrap=strlen
	$<$<NOT:$<CONFIG:Debug,RelWithDebInfo>>:-Wl,--strip-all>
)

## architecture specific
set(compile_options_arch -mlittle-endian -mcpu=cortex-a55 -mtune=cortex-a55)

## compiler specific

### set sysroot for clang
if(${IS_CLANG})
	set(compile_options_common --sysroot=${SDK_PATH}/libs/arm_gnu/13.2.rel1/sysroot ${compile_options_common})
	set(link_options_common --sysroot=${SDK_PATH}/libs/arm_gnu/13.2.rel1/sysroot ${link_options_common})
endif()

### libgcc
if(${IS_CLANG})
	set(LIBGCC ${SDK_PATH}/libs/arm_gnu/13.2.rel1/builtins/libgcc.a)
else()
	set(LIBGCC "-lgcc")
endif()

### set target triple for clang
if(${IS_CLANG})
	set(compile_options_arch -target aarch64-none-elf ${compile_options_arch})
	set(link_options_common -target aarch64-none-elf ${link_options_common})
endif()

if(${IS_GCC})
	### With ld.bfd version 2.39 and newer new warnings are added. 
	### Skip those since we are not loaded by a elf loader.
	if(${CMAKE_C_COMPILER_VERSION} VERSION_GREATER_EQUAL 12.2.0)
		set(link_options_common ${link_options_common} -Wl,--no-warn-rwx-segments)
	endif()
	# https://gcc.gnu.org/bugzilla/show_bug.cgi?id=105523
	set(compile_options_common --param=min-pagesize=0 ${compile_options_common})
endif()

## this apply to all targets

### Adds options to the COMPILE_OPTIONS directory property. 
### These options are used when compiling targets from the current directory and below.
add_compile_options(${compile_options_arch} ${compile_options_common})

# targets
include($CACHE{APP_CMAKE}) # APP_CMAKE cannot be empty
add_executable($CACHE{PROJECT_NAME}.elf)
include(targets.cmake)

# app targets
target_sources($CACHE{PROJECT_NAME}.elf
	PRIVATE
		${app_src}
)

target_include_directories($CACHE{PROJECT_NAME}.elf
    PRIVATE
        ${app_inc}
)

target_compile_options($CACHE{PROJECT_NAME}.elf
	PRIVATE
		${app_compile_options}
)

target_link_options($CACHE{PROJECT_NAME}.elf
	PRIVATE
		${link_options_common}
)

target_link_libraries($CACHE{PROJECT_NAME}.elf
	PRIVATE
		aarch64_startup
		newlib_stubs
		gicv3
		${app_dep}
		-lc
		-lm 
		${LIBGCC}
)

if("$CACHE{MEM_SCHEME}" STREQUAL "CUSTOM")
	target_link_options($CACHE{PROJECT_NAME}.elf 
		PRIVATE
			-T $CACHE{CUSTOM_LINKER_SCRIPT}
	)
else()
	target_link_libraries($CACHE{PROJECT_NAME}.elf 
		PRIVATE 
			linker_script
	)
endif()

# Cumtom targets

## dump
add_custom_command(
	TARGET $CACHE{PROJECT_NAME}.elf POST_BUILD
	COMMAND ${CMAKE_OBJDUMP} -dx $CACHE{PROJECT_NAME}.elf > $CACHE{PROJECT_NAME}.dump
	WORKING_DIRECTORY   "${CMAKE_BINARY_DIR}"
)

## bin
add_custom_command(
	TARGET $CACHE{PROJECT_NAME}.elf POST_BUILD
	COMMAND ${CMAKE_OBJCOPY} -O binary $CACHE{PROJECT_NAME}.elf $CACHE{PROJECT_NAME}.bin
	WORKING_DIRECTORY   "${CMAKE_BINARY_DIR}"
)

## hex
add_custom_command(
	OUTPUT 
		$CACHE{PROJECT_NAME}.hex
	COMMAND hexdump -e '"%08x\\n"' -v $CACHE{PROJECT_NAME}.bin > $CACHE{PROJECT_NAME}.hex
	DEPENDS
		$CACHE{PROJECT_NAME}.elf
	WORKING_DIRECTORY   "${CMAKE_BINARY_DIR}"
)
add_custom_target(hex
	DEPENDS 
		$CACHE{PROJECT_NAME}.hex
)

# additional clean files
set_property(TARGET $CACHE{PROJECT_NAME}.elf 
	APPEND PROPERTY 
	ADDITIONAL_CLEAN_FILES 
		$CACHE{PROJECT_NAME}.map
		$CACHE{PROJECT_NAME}.bin
		$CACHE{PROJECT_NAME}.dump
)

# compile database
include(cmake_helper/common/compile_database.cmake)
