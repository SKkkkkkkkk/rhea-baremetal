set(TARTGET_NAME dw_mmc)

add_library(${TARTGET_NAME} STATIC)

target_sources(${TARTGET_NAME} 
	PRIVATE
		src/dw_mmc.c
		src/mmc.c
)

target_include_directories(${TARTGET_NAME}
	PUBLIC
		inc
)

target_link_libraries(${TARTGET_NAME}
	PRIVATE
		aarch64
		dw_apb_gpio
)