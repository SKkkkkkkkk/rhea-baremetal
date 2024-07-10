set(TARTGET_NAME "cru")

add_library(${TARTGET_NAME} STATIC)

target_sources(${TARTGET_NAME}
	PRIVATE
		src/cru.c
)

target_include_directories(${TARTGET_NAME}
	PUBLIC
		inc
)