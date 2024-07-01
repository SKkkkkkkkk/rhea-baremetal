set(TARTGET_NAME aarch64_startup)

add_library(${TARTGET_NAME} OBJECT)

target_sources(${TARTGET_NAME} 
	PRIVATE
		startup/entrypoint.S
		startup/exceptions.S
		startup/_main.c
		wakeup_core/wakeup_core.c
		misc/misc_helpers.S
		misc/cache_helpers.S
)

target_link_libraries(${TARTGET_NAME}
	PRIVATE
		system_counter
)