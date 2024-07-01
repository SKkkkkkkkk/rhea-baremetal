set(TARTGET_NAME xmodem)

add_library( ${TARTGET_NAME} STATIC )

target_sources( ${TARTGET_NAME} 
	PRIVATE
		src/xmodem.c
		src/xmodem_port.c
)

target_include_directories( ${TARTGET_NAME}
	PUBLIC
		inc
)

target_link_libraries(${TARTGET_NAME} 
	PRIVATE 
		crc
)

if($CACHE{BOARD} STREQUAL "QEMU")
	target_link_libraries(${TARTGET_NAME} PRIVATE pl011)
else()
	target_link_libraries(${TARTGET_NAME} PRIVATE dw_apb_uart)
endif()