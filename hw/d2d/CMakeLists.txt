set(TARGET_NAME d2d)

add_library(${TARGET_NAME} STATIC)

target_sources(${TARGET_NAME} 
	PRIVATE
		src/d2d_api.c
		src/d2d_sync.c
		src/d2d_ring_buf.c
)

target_include_directories(${TARGET_NAME}
	PUBLIC
		inc
)

target_link_libraries(${TARGET_NAME}
	PRIVATE
		crc
)

add_definitions(-DCONFIG_RHEA_DUAL_DIE)
add_definitions(-DCONFIG_RHEA_D2D_WORK_MODE=1)
add_definitions(-DCONFIG_RHEA_D2D_SYNC_HEADER_ADDR=0x48000000)
add_definitions(-DCONFIG_RHEA_D2D_SYNC_CMD_ADDR=0x48000010)
add_definitions(-DCONFIG_RHEA_D2D_SYNC_CMD_SIZE=0x7D0)
add_definitions(-DCONFIG_RHEA_D2D_SYNC_DATA_ADDR=0x480007E0)
add_definitions(-DCONFIG_RHEA_D2D_SYNC_DATA_SIZE=0xFF820)