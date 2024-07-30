set(TARGET_NAME freertos_kernel)

add_library( ${TARGET_NAME} STATIC )

target_sources( ${TARGET_NAME} 
	PRIVATE
		# common
		croutine.c
		event_groups.c
		list.c
		queue.c
		stream_buffer.c
		tasks.c
		timers.c
		# heap
		portable/MemMang/heap_4.c
		# port
		portable/GCC/ARM_AARCH64_SRE/port.c
		portable/GCC/ARM_AARCH64_SRE/portASM.S
		portable/GCC/ARM_AARCH64_SRE/portmacro.h
)

target_include_directories( ${TARGET_NAME}
	PUBLIC
		include
		${app_inc} # FreeRTOSConfig.h
		portable/GCC/ARM_AARCH64_SRE
)

target_link_libraries(${TARGET_NAME}
	PRIVATE
)

target_compile_options(${TARGET_NAME}
	PRIVATE
		# -DGUEST
)