# 定义函数：强制缓存变量非空
function(set_non_empty_cache VAR_NAME VALUE TYPE DESCRIPTION)
    set(${VAR_NAME} "${VALUE}" CACHE ${TYPE} "${DESCRIPTION}")
    if("${${VAR_NAME}}" STREQUAL "")
        message(FATAL_ERROR "Cache variable '${VAR_NAME}' must be non-empty!")
    endif()
endfunction()
# 使用示例
# set_non_empty_cache(MY_PATH "/default/path" PATH "Must provide a valid path")