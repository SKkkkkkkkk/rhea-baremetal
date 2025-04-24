set(TARGET_NAME ax65_startup)

add_library(${TARGET_NAME} OBJECT)

target_sources(${TARGET_NAME} 
	PRIVATE
		startup.S
		trap.c
)