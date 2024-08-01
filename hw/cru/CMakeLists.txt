set(TARGET_NAME "cru")

add_library(${TARGET_NAME} STATIC)

target_sources(${TARGET_NAME}
	PRIVATE
		src/cru.c
)

target_include_directories(${TARGET_NAME}
	PUBLIC
		inc
)