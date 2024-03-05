# 指定该模块(库)的名字
set(LIB_OBJS_NAME nand_flash)

# 指定该模块(库)的类型,这里为静态库
add_library(${LIB_OBJS_NAME} STATIC)

# 该模块的源文件
target_sources(${LIB_OBJS_NAME} 
	PRIVATE
		src/nand_flash.c
)

# 该模块的头文件路径
target_include_directories(${LIB_OBJS_NAME}
	PUBLIC
		inc
)

# 依赖模块
target_link_libraries(${LIB_OBJS_NAME} 
	PRIVATE
		dw_apb_ssi
		# systimer
)

# 注意！！！
# 3种属性
# PUBLIC: 除了参与自身编译，还会传递给被依赖模块。如：模块的接口头文件路径
# PRIVATE: 模块内部信息，只参与自身编译。如：该模块的源文件，模块内部头文件路径(不需要or不应该被外部include的)
# INTERFACE: 不参与自身编译，只传递给被依赖模块。如：模块的接口头文件路径
# 一般一个好的模块封装完后原则是：PUBLIC少用，能PRIVATE一定PRIVATE，头文件尽量为INTERFACE
