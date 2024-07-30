set(TARGET_NAME "pcie_ep")

add_library(${TARGET_NAME} STATIC)

target_sources(${TARGET_NAME}
	PRIVATE
		src/pcie_ep.c
)

target_include_directories(${TARGET_NAME}
	PUBLIC
		inc
)