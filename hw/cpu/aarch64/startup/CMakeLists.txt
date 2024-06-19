set(TARTGET_NAME aarch64)

add_library(${TARTGET_NAME} OBJECT)

target_sources(${TARTGET_NAME} 
	PRIVATE
		src/misc_helpers.S
		src/cache_helpers.S
		src/config_mmu.c
		src/entrypoint.S
		src/exceptions.S
		src/_main.c
)

target_include_directories(${TARTGET_NAME}
	PUBLIC
		inc
)

if(NOT $CACHE{BOARD} STREQUAL "QEMU")
	target_link_libraries(${TARTGET_NAME}
		PRIVATE
			system_counter
	)
endif()