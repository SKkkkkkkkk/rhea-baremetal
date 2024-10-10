set(TARGET_NAME time_stamp)

add_library( ${TARGET_NAME} INTERFACE )

target_include_directories( ${TARGET_NAME}
	INTERFACE
		inc
)