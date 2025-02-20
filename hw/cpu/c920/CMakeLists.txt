set(TARGET_NAME c920_startup)

add_library(${TARGET_NAME} OBJECT)

target_sources(${TARGET_NAME} 
	PRIVATE
		startup.S
)