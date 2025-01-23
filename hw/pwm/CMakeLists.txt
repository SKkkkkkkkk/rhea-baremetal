set(TARGET_NAME "pwm")

add_library(${TARGET_NAME} STATIC)

target_sources(${TARGET_NAME}
	PRIVATE
		src/pwm.c
)

target_include_directories(${TARGET_NAME}
	PUBLIC
		inc
)