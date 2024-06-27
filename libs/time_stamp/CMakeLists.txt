set(TARTGET_NAME time_stamp)

add_library( ${TARTGET_NAME} INTERFACE )

target_include_directories( ${TARTGET_NAME}
	INTERFACE
		inc
)