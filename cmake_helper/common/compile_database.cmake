
# CMAKE_EXPORT_COMPILE_COMMANDS will generate a compile database of all targets,
# even they are not involved and built in the current build process. This is
# not what we want, so we need to set it to OFF. And we generate the compile 
# commands database ourselves.
set(CMAKE_EXPORT_COMPILE_COMMANDS OFF CACHE INTERNAL "Generate compile commands database")

# Function to get all dependencies of a target
# Recursive function to find all dependencies
function(find_deps target all_deps)
	if (NOT TARGET ${target})
		# message(STATUS "Skipping non-target dependency: ${target}")
		return()
	endif()

	if(NOT ${target} IN_LIST local_deps)
		list(APPEND local_deps ${target})
		# message(STATUS "Added target: ${target}")
		get_target_property(link_targets ${target} LINK_LIBRARIES)
		foreach(linked_target IN LISTS link_targets)
			find_deps(${linked_target} local_deps)
		endforeach()
	endif()

	# Set the local_deps list in the parent scope to maintain scope integrity
	set(${all_deps} "${local_deps}" PARENT_SCOPE)
endfunction()

# Get all dependencies for the main executable
find_deps($CACHE{PROJECT_NAME}.elf all_deps)
# message(STATUS "Dependencies: ${all_deps}")

# Set EXPORT_COMPILE_COMMANDS for the executable and its dependencies
foreach(dep IN LISTS all_deps)
    set_target_properties(${dep} PROPERTIES EXPORT_COMPILE_COMMANDS ON)
endforeach()

