cmake_minimum_required(VERSION 3.15...3.28)
include(cmake_helper/common/out_of_source.cmake)
include(cmake_helper/cross_compile/generic_gcc.cmake)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(SDK_PATH ${CMAKE_CURRENT_SOURCE_DIR})

set(PROJECT_NAME "a" CACHE STRING "Project Name")

set(PROJECT_PATH "" CACHE PATH "Project Path")
if("$CACHE{PROJECT_PATH}" STREQUAL "")
	message(FATAL_ERROR "PROJECT_PATH must be set!")
endif()
if(NOT IS_ABSOLUTE $CACHE{PROJECT_PATH})
	set(PROJECT_PATH "$ENV{PWD}/${PROJECT_PATH}" CACHE PATH "Project path" FORCE)
endif()

set(BOARD "FPGA" CACHE STRING "Board: QEMU, RTL, FPGA, EVB")

set(MEM_SCHEME "ROM" CACHE STRING "ROM, SRAM, DRAM, CUSTOM")
if($CACHE{MEM_SCHEME} STREQUAL "CUSTOM")
	set(CUSTOM_LINKER_SCRIPT "" CACHE FILEPATH "Custom Linker Script file")
	if(NOT "$CACHE{CUSTOM_LINKER_SCRIPT}" STREQUAL "")
		if(NOT IS_ABSOLUTE $CACHE{CUSTOM_LINKER_SCRIPT})
			set(CUSTOM_LINKER_SCRIPT "$ENV{PWD}/${CUSTOM_LINKER_SCRIPT}" CACHE FILEPATH "Project path" FORCE)
		endif()
	endif()
endif()


project("${PROJECT_NAME}"
	VERSION 0.1.0
	DESCRIPTION "${PROJECT_NAME}"
	LANGUAGES C ASM
)

# apply to all targets

## common
set(compile_options_common -Wall -Wno-unused-variable -Werror -ffunction-sections -fdata-sections)
set(common_inc -I${SDK_PATH}/common/inc) 
add_compile_options(-D$CACHE{BOARD} ${compile_options_common} ${common_inc})

set(link_options_common 
	-static -nostartfiles -Wl,--fatal-warning -Wl,--gc-sections 
	-Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/$CACHE{PROJECT_NAME}.map -Wl,--print-memory-usage
	-Wl,--wrap=memcpy -Wl,--wrap=memmove -Wl,--wrap=memset -Wl,--wrap=memcmp
)
## With ld.bfd version 2.39 and newer new warnings are added. Skip those since we are not loaded by a elf loader.
if(${CMAKE_C_COMPILER_VERSION} VERSION_GREATER_EQUAL 12.2.0)
    set(link_options_common ${link_options_common} -Wl,--no-warn-rwx-segments)
endif()

## architecture specific
set(compile_options_arch -mlittle-endian -mstrict-align -march=armv8.1-a -DA55)
add_compile_options(${compile_options_arch})

# targets
add_subdirectory(hw/cpu/aarch64 EXCLUDE_FROM_ALL)
add_subdirectory(hw/gic/v3_v4 EXCLUDE_FROM_ALL)
add_subdirectory(hw/uart/pl011 EXCLUDE_FROM_ALL)
add_subdirectory(hw/uart/dw_apb_uart EXCLUDE_FROM_ALL)
add_subdirectory(libs/newlib_stubs EXCLUDE_FROM_ALL)

# app targets
include($CACHE{PROJECT_PATH}/app.cmake)
add_executable($CACHE{PROJECT_NAME}.elf)

target_sources($CACHE{PROJECT_NAME}.elf
	PRIVATE
		${app_src}
)

target_include_directories($CACHE{PROJECT_NAME}.elf
    PRIVATE
        ${CMAKE_CURRENT_BINARY_DIR}
        ${app_inc}
)

target_link_options($CACHE{PROJECT_NAME}.elf
	PRIVATE
		${link_options_common}
		-T $CACHE{CUSTOM_LINKER_SCRIPT}
)
if(NOT "$CACHE{CUSTOM_LINKER_SCRIPT}" STREQUAL "")
	target_link_options($CACHE{PROJECT_NAME}.elf 
		PRIVATE
			-T $CACHE{CUSTOM_LINKER_SCRIPT}
	)
endif()

target_link_libraries($CACHE{PROJECT_NAME}.elf
	PRIVATE
		aarch64
		newlib_stubs
		wakeup_core
		gicv3
		${app_dep}
)

if("$CACHE{CUSTOM_LINKER_SCRIPT}" STREQUAL "")
	target_link_libraries($CACHE{PROJECT_NAME}.elf 
		PRIVATE 
			linker_script
	)
endif()

# Cumtom targets

## dump

add_custom_command(
	OUTPUT $CACHE{PROJECT_NAME}.dump
	COMMAND ${CMAKE_OBJDUMP} -Sx $CACHE{PROJECT_NAME}.elf > $CACHE{PROJECT_NAME}.dump
	DEPENDS $CACHE{PROJECT_NAME}.elf
	WORKING_DIRECTORY   "${CMAKE_BINARY_DIR}"
	VERBATIM
)
add_custom_target(dump
	DEPENDS $CACHE{PROJECT_NAME}.dump
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
		$CACHE{PROJECT_NAME}.verilog.hex
		$CACHE{PROJECT_NAME}.hex
	COMMAND ${CMAKE_OBJCOPY} -O verilog $CACHE{PROJECT_NAME}.elf $CACHE{PROJECT_NAME}.verilog.hex
	COMMAND hexdump -e '"%08x\\n"' -v $CACHE{PROJECT_NAME}.bin > $CACHE{PROJECT_NAME}.hex
	DEPENDS
		$CACHE{PROJECT_NAME}.elf
	WORKING_DIRECTORY   "${CMAKE_BINARY_DIR}"
)
add_custom_target(hex
	DEPENDS 
		$CACHE{PROJECT_NAME}.verilog.hex
		$CACHE{PROJECT_NAME}.hex
)

# additional clean files
set_property(TARGET $CACHE{PROJECT_NAME}.elf 
	APPEND PROPERTY 
	ADDITIONAL_CLEAN_FILES 
		$CACHE{PROJECT_NAME}.map
		$CACHE{PROJECT_NAME}.bin
)