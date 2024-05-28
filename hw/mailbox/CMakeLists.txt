set(TARTGET_NAME "mailbox")

add_library(${TARTGET_NAME} STATIC)

target_sources(${TARTGET_NAME}
	PRIVATE
		src/mailbox.c
)

target_include_directories(${TARTGET_NAME}
	PUBLIC
		inc
)

target_link_libraries(${TARTGET_NAME}
	PRIVATE
		aarch64
)
