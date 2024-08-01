set(TARGET_NAME newlib_stubs)

add_library(${TARGET_NAME} OBJECT)

target_sources(${TARGET_NAME} 
	PRIVATE
		newlib_stubs.c
)

if($CACHE{BOARD} STREQUAL "QEMU")
	target_link_libraries(${TARGET_NAME}
		PRIVATE
			pl011
	)
else()
	target_link_libraries(${TARGET_NAME}
		PRIVATE
			dw_apb_uart
			cru
	)
endif()