cmake_minimum_required(VERSION 3.15...3.28)

# set(TARTGET_NAME "aarch64")

# project(${TARTGET_NAME}
# 	VERSION 0.1
# 	DESCRIPTION "${TARTGET_NAME}"
# 	LANGUAGES C
# )

# add_library(${TARTGET_NAME} INTERFACE)

# target_link_libraries(${TARTGET_NAME}
# 	INTERFACE
# 		startup
# 		linker_script
# 		wakeup_core
# )

add_subdirectory(startup EXCLUDE_FROM_ALL)
add_subdirectory(linker_script EXCLUDE_FROM_ALL)
add_subdirectory(wakeup_core EXCLUDE_FROM_ALL)