cmake_minimum_required(VERSION 3.15...3.28)

set(TARTGET_NAME "gicv3")

project(${TARTGET_NAME}
	VERSION 0.1
	DESCRIPTION "${TARTGET_NAME}"
	LANGUAGES C
)

add_library(${TARTGET_NAME} STATIC)

target_sources(${TARTGET_NAME}
	PRIVATE
		src/gicv3.c
)

target_include_directories(${TARTGET_NAME}
	PUBLIC
		inc
)