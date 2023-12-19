set(TARTGET_NAME newlib_stubs)

add_library(${TARTGET_NAME} OBJECT)

target_sources(${TARTGET_NAME} 
	PRIVATE
		newlib_stubs.c
)

target_link_libraries(${TARTGET_NAME}
	PRIVATE
		aarch64
		pl011
)
