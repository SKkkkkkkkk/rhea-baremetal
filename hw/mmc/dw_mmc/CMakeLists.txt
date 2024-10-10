set(TARGET_NAME dw_mmc)

add_library(${TARGET_NAME} STATIC)

target_sources(${TARGET_NAME} 
	PRIVATE
		src/dw_mmc.c
		src/mmc.c
)

target_include_directories(${TARGET_NAME}
	PUBLIC
		inc
)

target_link_libraries(${TARGET_NAME}
	PRIVATE
		dw_apb_gpio
		cru
)