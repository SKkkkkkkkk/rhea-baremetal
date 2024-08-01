set(TARGET_NAME aarch64_startup)

add_library(${TARGET_NAME} OBJECT)

target_sources(${TARGET_NAME} 
	PRIVATE
		startup/entrypoint.S
		startup/exceptions.S
		startup/_main.c
		startup/config_mmu.c
		wakeup_core/wakeup_core.c
		misc/misc_helpers.S
		misc/cache_helpers.S
)

if(NOT $CACHE{BOARD} STREQUAL "QEMU")
target_link_libraries(${TARGET_NAME}
	PRIVATE
		system_counter
)
endif()