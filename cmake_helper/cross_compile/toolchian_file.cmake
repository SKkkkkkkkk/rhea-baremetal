set(CMAKE_SYSTEM_NAME Generic)

set(CMAKE_C_COMPILER $ENV{CC} CACHE FILEPATH "")

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

# Set the archiver and ranlib for GCC
if(IS_GCC)
    get_filename_component(COMPILER_PATH ${CMAKE_C_COMPILER} DIRECTORY)
    get_filename_component(COMPILER_NAME ${CMAKE_C_COMPILER} NAME)
    
    # Extract the prefix (up to and including the last hyphen)
    string(REGEX MATCH "^[^-]+-[^-]+-[^-]+-" COMPILER_PREFIX ${COMPILER_NAME})
    if(COMPILER_PATH STREQUAL "")
        set(CMAKE_AR "${COMPILER_PREFIX}gcc-ar" CACHE FILEPATH "")
        set(CMAKE_RANLIB "${COMPILER_PREFIX}gcc-ranlib" CACHE FILEPATH "")
    else()
        set(CMAKE_AR "${COMPILER_PATH}/${COMPILER_PREFIX}gcc-ar" CACHE FILEPATH "")
        set(CMAKE_RANLIB "${COMPILER_PATH}/${COMPILER_PREFIX}gcc-ranlib" CACHE FILEPATH "")
    endif()
endif()


set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)