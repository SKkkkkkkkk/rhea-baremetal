set(TARTGET_NAME dw_apb_timers)

add_library(${TARTGET_NAME} STATIC)

target_sources(${TARTGET_NAME} 
	PRIVATE
		src/dw_apb_timers.c
)

target_include_directories(${TARTGET_NAME}
	PUBLIC
		inc
)