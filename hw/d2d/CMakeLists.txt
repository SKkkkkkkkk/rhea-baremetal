set(TARGET_NAME d2d)

add_library(${TARGET_NAME} STATIC)

target_sources(${TARGET_NAME} 
	PRIVATE
		src/d2d_api.c
		src/d2d_sync.c
		src/d2d_ring_buf.c
		src/crc32.c
		lib/src/clci_api.c
		lib/src/clci_error.c
		lib/src/commands_common.c
		lib/src/commands_sys.c
		lib/src/doorbell.c
		lib/src/interrupt.c
		lib/src/local_ctrl.c
		lib/src/mailbox_drv.c
		lib/src/mailbox_sys.c
		lib/src/ring_buf.c
)

target_include_directories(${TARGET_NAME}
	PUBLIC
		inc
		lib/inc
)

target_link_libraries(${TARGET_NAME}
	PRIVATE
		crc
		gicv3
)

if(NOT DEFINED SELF_DIE_ID)
    set(SELF_DIE_ID 0)
endif()

target_compile_definitions(d2d PUBLIC CONFIG_RHEA_DIE_MAX=2)
target_compile_definitions(d2d PUBLIC CONFIG_RHEA_D2D_SELF_ID=${SELF_DIE_ID})
target_compile_definitions(d2d PRIVATE CONFIG_RHEA_D2D_SYNC_HEADER_ADDR=0x48000000)
target_compile_definitions(d2d PRIVATE CONFIG_RHEA_D2D_SYNC_CMD_ADDR=0x48000010)
target_compile_definitions(d2d PRIVATE CONFIG_RHEA_D2D_SYNC_CMD_SIZE=0x960)
target_compile_definitions(d2d PRIVATE CONFIG_RHEA_D2D_SYNC_DATA_ADDR=0x48000970)
target_compile_definitions(d2d PRIVATE CONFIG_RHEA_D2D_SYNC_DATA_SIZE=0xFF690)