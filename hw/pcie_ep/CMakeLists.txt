set(TARTGET_NAME "pcie_ep")

add_library(${TARTGET_NAME} STATIC)

target_sources(${TARTGET_NAME}
	PRIVATE
		src/pcie_ep.c
)

target_include_directories(${TARTGET_NAME}
	PUBLIC
		inc
)