set(TARTGET_NAME systimer)

add_library( ${TARTGET_NAME} STATIC )

target_sources( ${TARTGET_NAME} 
	PRIVATE
		src/systimer.c
)

target_include_directories( ${TARTGET_NAME}
	PUBLIC
		inc
)

target_link_libraries(${TARTGET_NAME}
	PRIVATE
		dw_apb_timers
		gicv3
		cru
)