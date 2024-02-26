set(TARTGET_NAME newlib_stubs)

add_library(${TARTGET_NAME} OBJECT)

target_sources(${TARTGET_NAME} 
	PRIVATE
		newlib_stubs.c
)

target_link_libraries(${TARTGET_NAME}
	PRIVATE
		aarch64
)

if($CACHE{BOARD} STREQUAL "QEMU")
	target_link_libraries(${TARTGET_NAME}
		PRIVATE
			pl011
	)
else()
	target_link_libraries(${TARTGET_NAME}
		PRIVATE
			dw_apb_uart
	)
endif()