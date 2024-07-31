set(TARTGET_NAME "pcie")

add_library(${TARTGET_NAME} STATIC)

target_sources(${TARTGET_NAME}
	PRIVATE
		src/pcie.c
)

target_include_directories(${TARTGET_NAME}
	PUBLIC
		inc
)
