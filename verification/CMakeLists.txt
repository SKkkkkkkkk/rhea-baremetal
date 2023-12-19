cmake_minimum_required(VERSION 3.15...3.28)

set(TARTGET_NAME "verification_all_in_one")

project(${TARTGET_NAME}
	VERSION 0.1
	DESCRIPTION "${TARTGET_NAME}"
	LANGUAGES C
)

file(GLOB_RECURSE sources  CONFIGURE_DEPENDS *.c *.s *.S)
add_library(${TARTGET_NAME} STATIC)

target_sources(${TARTGET_NAME}
	PRIVATE
		${sources}
)

target_include_directories(${TARTGET_NAME}
	PUBLIC
		.
)