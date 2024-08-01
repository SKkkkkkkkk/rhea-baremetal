cmake_minimum_required(VERSION 3.15...3.28)

set(TARGET_NAME "pl011")

project(${TARGET_NAME}
	VERSION 0.1
	DESCRIPTION "${TARGET_NAME}"
	LANGUAGES C
)

add_library(${TARGET_NAME} STATIC)

target_sources(${TARGET_NAME}
	PRIVATE
		src/pl011.c
)

target_include_directories(${TARGET_NAME}
	PUBLIC
		inc
)