cmake_minimum_required(VERSION 3.15...3.28)

set(TARGET_NAME "dw_apb_uart")

project(${TARGET_NAME}
	VERSION 0.1
	DESCRIPTION "${TARGET_NAME}"
	LANGUAGES C
)

add_library(${TARGET_NAME} STATIC)

target_sources(${TARGET_NAME}
	PRIVATE
		src/dw_apb_uart.c
)

target_include_directories(${TARGET_NAME}
	PUBLIC
		inc
)