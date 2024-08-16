# 该工程包含几个不同的测试case
set(TEST_CASE "BASIC" CACHE STRING "BASIC, LPI, D2D")
set_property(CACHE TEST_CASE PROPERTY STRINGS "BASIC" "LPI" "D2D")

# 添加app sources, 注意必须加上CMAKE_CURRENT_LIST_DIR!
set(app_src
	${CMAKE_CURRENT_LIST_DIR}/src/main.c
	${CMAKE_CURRENT_LIST_DIR}/src/gicv3_lpis.c
	${CMAKE_CURRENT_LIST_DIR}/src/gicv3_basic.c
	${CMAKE_CURRENT_LIST_DIR}/src/gicv3_cpuif.S
)

# 添加app include dirs, 注意必须加上CMAKE_CURRENT_LIST_DIR!
set(app_inc
	${CMAKE_CURRENT_LIST_DIR}/inc
)

# 添加app dependencies
set(app_dep
	dw_apb_timers
)

# 添加app_compile_options
set(app_compile_options
	-DCASE_$CACHE{TEST_CASE}
)