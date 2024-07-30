set(TARGET_NAME "mailbox")

add_library(${TARGET_NAME} STATIC)

target_sources(${TARGET_NAME}
	PRIVATE
		src/mailbox.c
)

target_include_directories(${TARGET_NAME}
	PUBLIC
		inc
)