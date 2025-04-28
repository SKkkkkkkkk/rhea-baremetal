cmake_minimum_required(VERSION 3.15...3.28)
include(cmake_helper/out_of_source.cmake)
include(cmake_helper/helper_func.cmake)
include(cmake_helper/toolchain_file.cmake)
include(cmake_helper/custom_build_type.cmake)
set(CMAKE_INSTALL_PREFIX ${CMAKE_BINARY_DIR}/install CACHE INTERNAL "Install path")

set(SDK_PATH ${CMAKE_SOURCE_DIR})

set(PROJECT_NAME "a" CACHE STRING "Project Name")
project($CACHE{PROJECT_NAME}
	VERSION 0.1.0
	DESCRIPTION $CACHE{PROJECT_NAME}
	LANGUAGES C ASM
)

# board specific
set(BOARD "FPGA" CACHE STRING "VIRT, RTL, FPGA, EVB, PLD")
set_property(CACHE BOARD PROPERTY STRINGS "VIRT" "RTL" "FPGA" "EVB" "PLD")

set(MEM_SCHEME "ROM" CACHE STRING "ROM, SRAM, DRAM, CUSTOM")
set_property(CACHE MEM_SCHEME PROPERTY STRINGS "ROM" "SRAM" "DRAM" "CUSTOM")
if($CACHE{MEM_SCHEME} STREQUAL "CUSTOM")
	unset(STACK_SIZE CACHE)
	unset(HEAP_SIZE CACHE)
	set(CUSTOM_LINKER_SCRIPT "" CACHE FILEPATH "Custom Linker Script file, Cannot be empty!")
else()
	unset(CUSTOM_LINKER_SCRIPT CACHE)
	set(STACK_SIZE "0x4000" CACHE STRING "Stack Size(In Bytes)")
	set(HEAP_SIZE  "0x4000" CACHE STRING "Heap Size(In Bytes)")
endif()

set(CPU_ARCH "A55" CACHE STRING "A55 AX65")
set_property(CACHE CPU_ARCH PROPERTY STRINGS "A55" "AX65")

set(ENABLE_LTO "OFF" CACHE BOOL "Enable Link Time Optimization")

# for all targets

## common
set(compile_options_common
	-Wall -Werror -Wno-unused-variable -Wno-unused-function
	-ffunction-sections -fdata-sections
	-mstrict-align
	$<$<BOOL:$CACHE{ENABLE_LTO}>:-flto>
	-D$CACHE{BOARD}
	-I${SDK_PATH}/common/inc
)

set(link_options
	-static
	-nostdlib # we will explicitly link libc.a, libm.a and libgcc.a later
	$<$<BOOL:$CACHE{ENABLE_LTO}>:-flto>
	-Wl,--fatal-warnings
	-Wl,--gc-sections
	-Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/$CACHE{PROJECT_NAME}.map
	-Wl,--print-memory-usage
)

## compiler specific

if(${IS_GCC})
	# Check if linker supports --no-warn-rwx-segments option
	execute_process(
		COMMAND ${CMAKE_LINKER} --no-warn-rwx-segments -v
		ERROR_QUIET
		OUTPUT_QUIET
		RESULT_VARIABLE LINKER_OPTION_RESULT
	)
	if(${LINKER_OPTION_RESULT} EQUAL 0)
		list(APPEND link_options -Wl,--no-warn-rwx-segments)
	endif()
	# https://gcc.gnu.org/bugzilla/show_bug.cgi?id=105523
	list(APPEND compile_options_common --param=min-pagesize=0)
endif()

## Adds options to the COMPILE_OPTIONS directory property. 
## These options are used when compiling targets from the current directory and below.
add_compile_options(${compile_options_common}) # 扩散到所有目标

# cpu arch target
include(hw/cpu/cpu_arch.cmake)

# targets
set_non_empty_cache(APP_CMAKE "" FILEPATH "Path to the app.cmake") # set APP_CMAKE default value to empty string, so that it must be provided by the user
include($CACHE{APP_CMAKE})
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
		${compile_options_arch}
		${link_options}
)

target_link_libraries($CACHE{PROJECT_NAME}.elf
	PRIVATE
		${CPU_ARCH_LIB}
		${app_dep}
		newlib_stubs
		-lc
		-lm
		-lgcc
)

if("$CACHE{MEM_SCHEME}" STREQUAL "CUSTOM")
	target_link_options($CACHE{PROJECT_NAME}.elf 
		PRIVATE
			$<$<BOOL:$CACHE{CUSTOM_LINKER_SCRIPT}>:-T $CACHE{CUSTOM_LINKER_SCRIPT}> # CUSTOM_LINKER_SCRIPT should not be empty
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
include(cmake_helper/compile_database.cmake)
