cmake_minimum_required(VERSION 3.15...3.28)

set(TARGET_NAME "gicv3")

project(${TARGET_NAME}
	VERSION 0.1
	DESCRIPTION "${TARGET_NAME}"
	LANGUAGES C
)

add_library(${TARGET_NAME} STATIC)

target_sources(${TARGET_NAME}
	PRIVATE
		src/gicv3.c
		lpi/gicv3_cpuif.S
		lpi/gicv3_lpis.c
		lpi/gicv3_basic.c
)

target_include_directories(${TARGET_NAME}
	PUBLIC
		inc
		lpi
)