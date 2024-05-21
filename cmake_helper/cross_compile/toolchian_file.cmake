set(CMAKE_SYSTEM_NAME Generic)

set(CMAKE_C_COMPILER $ENV{CC} CACHE STRING "")

# Define the function to check for a substring
function(find_substring STRING SUBSTRING RESULT_VAR)
    string(FIND "${STRING}" "${SUBSTRING}" SUBSTRING_INDEX)
    if("${SUBSTRING_INDEX}" EQUAL -1)
        set(${RESULT_VAR} FALSE PARENT_SCOPE)
    else()
        set(${RESULT_VAR} TRUE PARENT_SCOPE)
    endif()
endfunction()

# Detecting compiler
find_substring("${CMAKE_C_COMPILER}" "clang" IS_CLANG)
find_substring("${CMAKE_C_COMPILER}" "gcc" IS_GCC)
if(NOT IS_GCC AND NOT IS_CLANG)
	message(FATAL_ERROR "Unsupported compiler: ${CMAKE_C_COMPILER}")
endif()

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)