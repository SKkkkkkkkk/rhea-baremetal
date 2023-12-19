cmake_minimum_required(VERSION 3.15...3.28)
include(cmake_helper/common/out_of_source.cmake)
include(cmake_helper/cross_compile/generic_gcc.cmake)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(SDK_PATH ${CMAKE_CURRENT_SOURCE_DIR})
set(PROJECT_NAME "a" CACHE STRING "Project Name")
set(PROJECT_PATH "" CACHE PATH "Project Path")
set(EXTRA_SRCS "" CACHE FILEPATH "Extra Sources to join the compiling.")
set(EXTRA_INCS "" CACHE PATH "Extra Directories to include.")
if(NOT $CACHE{PROJECT_PATH} STREQUAL "")
	if(NOT IS_ABSOLUTE ${PROJECT_PATH})
		set(PROJECT_PATH "$ENV{PWD}/$CACHE{PROJECT_PATH}" CACHE PATH "Project path" FORCE)
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
add_compile_options(${compile_options_common} ${common_inc})

set(link_options_common 
	-static -nostartfiles -Wl,--fatal-warning -Wl,--gc-sections 
	-Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/$CACHE{PROJECT_NAME}.map -Wl,--print-memory-usage
	-Wl,--wrap=memcpy -Wl,--wrap=memmove -Wl,--wrap=memset -Wl,--wrap=memcmp
)

## architecture specific
set(compile_options_arch -mlittle-endian -mstrict-align -march=armv8.1-a -DA55)
add_compile_options(${compile_options_arch})

## board specific
add_compile_options(-DQEMU_VIRT)

# targets
add_subdirectory(hw/cpu/aarch64 EXCLUDE_FROM_ALL)
add_subdirectory(hw/gic/v3_v4 EXCLUDE_FROM_ALL)
add_subdirectory(hw/uart/pl011 EXCLUDE_FROM_ALL)
add_subdirectory(libs/newlib_stubs EXCLUDE_FROM_ALL)
add_subdirectory(verification EXCLUDE_FROM_ALL)

# app targets
if(NOT $CACHE{PROJECT_PATH} STREQUAL "")
	include($CACHE{PROJECT_PATH}/app.cmake)
endif()
add_executable($CACHE{PROJECT_NAME}.elf)

target_sources($CACHE{PROJECT_NAME}.elf
	PRIVATE
		$CACHE{EXTRA_SRCS}
		${app_src}
)

target_include_directories($CACHE{PROJECT_NAME}.elf
    PRIVATE
		$CACHE{EXTRA_INCS}
        ${app_inc}
)

target_link_options($CACHE{PROJECT_NAME}.elf
	PRIVATE
		${link_options_common}
)

target_link_libraries($CACHE{PROJECT_NAME}.elf
	PRIVATE
		aarch64
		linker_script
		newlib_stubs
		wakeup_core
		gicv3
		verification_all_in_one
		${app_dep}
)

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