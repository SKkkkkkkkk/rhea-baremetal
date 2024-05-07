# 添加app sources, 注意必须加上CMAKE_CURRENT_LIST_DIR!
set(app_src
	${CMAKE_CURRENT_LIST_DIR}/src/main.c
	${CMAKE_CURRENT_LIST_DIR}/src/dw_apb_timer_test.c
)

# 添加app include dirs, 注意必须加上CMAKE_CURRENT_LIST_DIR!
set(app_inc
	${CMAKE_CURRENT_LIST_DIR}/inc
)

# 添加app dependencies
set(app_dep
	dw_apb_timers
	systimer
)