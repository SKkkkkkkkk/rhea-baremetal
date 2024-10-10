set(TARGET_NAME systimer)

add_library( ${TARGET_NAME} STATIC )

target_sources( ${TARGET_NAME} 
	PRIVATE
		src/systimer.c
)

target_include_directories( ${TARGET_NAME}
	PUBLIC
		inc
)

target_link_libraries(${TARGET_NAME}
	PRIVATE
		dw_apb_timers
		gicv3
		cru
)