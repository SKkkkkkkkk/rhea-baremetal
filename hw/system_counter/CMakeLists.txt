set(TARGET_NAME system_counter)

add_library(${TARGET_NAME} STATIC)

target_sources(${TARGET_NAME} 
	PRIVATE
		src/system_counter.c
)

target_include_directories(${TARGET_NAME}
	PUBLIC
		inc
)