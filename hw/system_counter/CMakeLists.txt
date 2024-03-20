set(TARTGET_NAME system_counter)

add_library(${TARTGET_NAME} STATIC)

target_sources(${TARTGET_NAME} 
	PRIVATE
		src/system_counter.c
)

target_include_directories(${TARTGET_NAME}
	PUBLIC
		inc
)