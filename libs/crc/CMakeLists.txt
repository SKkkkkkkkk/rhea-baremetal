set(TARGET_NAME crc)

add_library( ${TARGET_NAME} STATIC )

target_sources( ${TARGET_NAME} 
	PRIVATE
		src/crc16.c
)

target_include_directories( ${TARGET_NAME}
	PUBLIC
		inc
)