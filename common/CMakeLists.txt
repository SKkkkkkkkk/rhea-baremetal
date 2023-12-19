cmake_minimum_required(VERSION 3.26...3.26.3)

set(TARTGET_NAME "common")

project(${TARTGET_NAME}
	VERSION 0.1
	DESCRIPTION "${TARTGET_NAME} driver"
	LANGUAGES C
)

add_library(${TARTGET_NAME} INTERFACE)

target_include_directories(${TARTGET_NAME}
	INTERFACE
		inc
)
