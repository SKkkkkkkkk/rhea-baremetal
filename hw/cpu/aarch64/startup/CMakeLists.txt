set(TARTGET_NAME aarch64)

add_library(${TARTGET_NAME} OBJECT)

target_sources(${TARTGET_NAME} 
	PRIVATE
		src/cache_helpers.S
		src/config_mmu.c
		src/entrypoint.S
		src/exceptions.S
)

target_include_directories(${TARTGET_NAME}
	PUBLIC
		inc
)