cmake_minimum_required(VERSION 3.15...3.28)
include(cmake_helper/common/out_of_source.cmake)
include(cmake_helper/cross_compile/toolchian_file.cmake)
include(cmake_helper/common/custom_build_type.cmake)
set(CMAKE_INSTALL_PREFIX ${CMAKE_BINARY_DIR}/install CACHE INTERNAL "Install path")

set(SDK_PATH ${CMAKE_SOURCE_DIR})

set(PROJECT_NAME "a" CACHE STRING "Project Name")

set(PROJECT_PATH "" CACHE PATH "Project Path") # since the type is PATH, set comman will convert it to absolute automatically.
if("$CACHE{PROJECT_PATH}" STREQUAL "")
	message(FATAL_ERROR "PROJECT_PATH must be set!")
endif()

set(BOARD "FPGA" CACHE STRING "QEMU, RTL, FPGA, EVB")

set(MEM_SCHEME "ROM" CACHE STRING "ROM, SRAM, DRAM, CUSTOM")
if($CACHE{MEM_SCHEME} STREQUAL "CUSTOM")
	set(CUSTOM_LINKER_SCRIPT "" CACHE FILEPATH "Custom Linker Script file")
	if("$CACHE{CUSTOM_LINKER_SCRIPT}" STREQUAL "")
		message(FATAL_ERROR "CUSTOM_LINKER_SCRIPT must be set, with MEM_SCHEME=CUSTOM")
	endif()
endif()

set(STACK_SIZE "0x4000" CACHE STRING "Stack Size(In Bytes)")

set(HEAP_SIZE  "0x4000" CACHE STRING "Heap Size(In Bytes)")

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
	-I${SDK_PATH}/common/inc
)

set(link_options_common
	-static
	-nostdlib # we will explicitly link libc.a, libm.a and libgcc.a later
	-Wl,--fatal-warnings
	-Wl,--gc-sections
	-Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/$CACHE{PROJECT_NAME}.map
	-Wl,--print-memory-usage
	-Wl,--wrap=memcpy -Wl,--wrap=memmove -Wl,--wrap=memset -Wl,--wrap=memcmp
)

## if CMAKE_BUILD_TYPE is not Debug or RelWithDebInfo, strip all symbols
if(NOT $CACHE{CMAKE_BUILD_TYPE} STREQUAL "Debug" AND NOT $CACHE{CMAKE_BUILD_TYPE} STREQUAL "RelWithDebInfo")
	list(APPEND link_options_common -Wl,--strip-all)
endif()

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

### With ld.bfd version 2.39 and newer new warnings are added. 
### Skip those since we are not loaded by a elf loader.
if(${IS_GCC})
	if(${CMAKE_C_COMPILER_VERSION} VERSION_GREATER_EQUAL 12.2.0)
		set(link_options_common ${link_options_common} -Wl,--no-warn-rwx-segments)
	endif()
endif()

## this apply to all targets
add_compile_options(-DA55 -D$CACHE{BOARD} ${compile_options_arch} ${compile_options_common})

# targets
add_subdirectory(hw/cpu/aarch64 EXCLUDE_FROM_ALL)
add_subdirectory(hw/gic/v3_v4 EXCLUDE_FROM_ALL)
add_subdirectory(hw/uart/pl011 EXCLUDE_FROM_ALL)
add_subdirectory(hw/uart/dw_apb_uart EXCLUDE_FROM_ALL)
add_subdirectory(hw/gpio/dw_apb_gpio EXCLUDE_FROM_ALL)
add_subdirectory(hw/spi/dw_apb_ssi EXCLUDE_FROM_ALL)
add_subdirectory(hw/timer/dw_apb_timers EXCLUDE_FROM_ALL)
add_subdirectory(hw/system_counter EXCLUDE_FROM_ALL)
add_subdirectory(hw/mailbox EXCLUDE_FROM_ALL)

add_subdirectory(libs/newlib_stubs EXCLUDE_FROM_ALL)
add_subdirectory(libs/flash/nor EXCLUDE_FROM_ALL)
add_subdirectory(libs/flash/nand EXCLUDE_FROM_ALL)
add_subdirectory(libs/systimer EXCLUDE_FROM_ALL)
add_subdirectory(libs/crc EXCLUDE_FROM_ALL)

# app targets
if(NOT $CACHE{PROJECT_PATH} STREQUAL "")
	include($CACHE{PROJECT_PATH}/app.cmake)
endif()
add_executable($CACHE{PROJECT_NAME}.elf)

target_sources($CACHE{PROJECT_NAME}.elf
	PRIVATE
		${app_src}
)

target_include_directories($CACHE{PROJECT_NAME}.elf
    PRIVATE
        ${app_inc}
)

target_link_options($CACHE{PROJECT_NAME}.elf
	PRIVATE
		${link_options_common}
)

target_link_libraries($CACHE{PROJECT_NAME}.elf
	PRIVATE
		aarch64
		newlib_stubs
		wakeup_core
		gicv3
		${app_dep}
		-lc -lm ${LIBGCC}
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
