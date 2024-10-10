set(TARGET_NAME dw_apb_timers)

add_library(${TARGET_NAME} STATIC)

target_sources(${TARGET_NAME} 
	PRIVATE
		src/dw_apb_timers.c
)

target_include_directories(${TARGET_NAME}
	PUBLIC
		inc
)