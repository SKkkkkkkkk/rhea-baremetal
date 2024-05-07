set(TARTGET_NAME dw_apb_ssi)

add_library(${TARTGET_NAME} STATIC)

target_sources(${TARTGET_NAME} 
	PRIVATE
		src/dw_apb_ssi.c
)

target_include_directories(${TARTGET_NAME}
	PUBLIC
		inc
)

target_link_libraries(${TARTGET_NAME}
	PRIVATE
		dw_apb_gpio
		aarch64
)