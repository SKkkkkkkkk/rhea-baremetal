set(TARTGET_NAME wakeup_core)

add_library(${TARTGET_NAME} STATIC)

target_sources(${TARTGET_NAME} 
	PRIVATE
		src/wakeup_core.c
)

target_include_directories(${TARTGET_NAME}
	INTERFACE
		inc
)