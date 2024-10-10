cmake_minimum_required(VERSION 3.15...3.28)

set(TARGET_NAME "linker_script")

project(${TARGET_NAME}
	VERSION 0.1
	DESCRIPTION "linker script"
	LANGUAGES C
)

add_custom_command(
	OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/linker.ld
	DEPENDS
		${CMAKE_CURRENT_SOURCE_DIR}/linker.ld.S
	COMMAND 
		${CMAKE_C_COMPILER} -E -P -x assembler-with-cpp -D__LINKER__
		-I${SDK_PATH}/common/inc -D$CACHE{BOARD} 
		-DSTACK_SIZE=$CACHE{STACK_SIZE} -DHEAP_SIZE=$CACHE{HEAP_SIZE}
		-DMEM_SCHEME_ROM=1 -DMEM_SCHEME_SRAM=2 -DMEM_SCHEME_DRAM=3 -DMEM_SCHEME_CUSTOM=4 -DMEM_SCHEME=MEM_SCHEME_$CACHE{MEM_SCHEME} 
		${CMAKE_CURRENT_SOURCE_DIR}/linker.ld.S 
		-o ${CMAKE_CURRENT_BINARY_DIR}/linker.ld
	VERBATIM
)

add_custom_target(gen_linker.ld DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/linker.ld)

add_library(${TARGET_NAME} INTERFACE)
add_dependencies(${TARGET_NAME} gen_linker.ld)
target_link_options(${TARGET_NAME} INTERFACE -T ${CMAKE_CURRENT_BINARY_DIR}/linker.ld)
set_target_properties(${TARGET_NAME} PROPERTIES INTERFACE_LINK_DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/linker.ld)