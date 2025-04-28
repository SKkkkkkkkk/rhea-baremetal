set(TARGET_NAME "dw_axi_dma")

add_library(${TARGET_NAME} STATIC)

target_sources(${TARGET_NAME}
	PRIVATE
		src/dw_axi_dma.c
)

target_include_directories(${TARGET_NAME}
	PUBLIC
		inc
)