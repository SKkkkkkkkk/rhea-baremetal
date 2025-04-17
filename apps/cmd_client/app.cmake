# 添加app sources, 注意必须加上CMAKE_CURRENT_LIST_DIR!
set(app_src
	${CMAKE_CURRENT_LIST_DIR}/src/agic_cmdqueue.c
	${CMAKE_CURRENT_LIST_DIR}/src/c2c_ioctl.c
	${CMAKE_CURRENT_LIST_DIR}/src/c2c_machine.c
	${CMAKE_CURRENT_LIST_DIR}/src/agic_sprintf.c
	${CMAKE_CURRENT_LIST_DIR}/src/main.c
)

# 添加app include dirs, 注意必须加上CMAKE_CURRENT_LIST_DIR!
set(app_inc
	${CMAKE_CURRENT_LIST_DIR}/inc
)

# 添加app dependencies
set(app_dep
)