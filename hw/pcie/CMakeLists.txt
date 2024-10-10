set(TARGET_NAME "pcie")

add_library(${TARGET_NAME} STATIC)

target_sources(${TARGET_NAME}
	PRIVATE
		src/pcie.c
)

target_include_directories(${TARGET_NAME}
	PUBLIC
		inc
)
