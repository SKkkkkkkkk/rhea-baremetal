set(TARGET_NAME xmodem)

add_library( ${TARGET_NAME} STATIC )

target_sources( ${TARGET_NAME} 
	PRIVATE
		src/xmodem.c
		src/xmodem_port.c
)

target_include_directories( ${TARGET_NAME}
	PUBLIC
		inc
)

target_link_libraries(${TARGET_NAME} 
	PRIVATE 
		crc
)

if($CACHE{BOARD} STREQUAL "VIRT")
	target_link_libraries(${TARGET_NAME} PRIVATE pl011)
else()
	target_link_libraries(${TARGET_NAME} PRIVATE dw_apb_uart)
endif()