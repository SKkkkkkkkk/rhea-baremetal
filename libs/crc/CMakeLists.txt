set(TARTGET_NAME crc)

add_library( ${TARTGET_NAME} STATIC )

target_sources( ${TARTGET_NAME} 
	PRIVATE
		src/crc16.c
)

target_include_directories( ${TARTGET_NAME}
	PUBLIC
		inc
)