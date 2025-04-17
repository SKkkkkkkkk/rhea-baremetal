set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Debug, Release, MinSizeRel")
set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel")

set(CMAKE_C_FLAGS_DEBUG "-Og -g3" CACHE INTERNAL "Flags used by the C compiler during DEBUG builds.")
set(CMAKE_C_FLAGS_MINSIZEREL "-Oz -g3" CACHE INTERNAL "Flags used by the C compiler during MINSIZEREL builds.")
set(CMAKE_C_FLAGS_RELEASE "-O3 -g3" CACHE INTERNAL "Flags used by the C compiler during RELEASE builds.")

set(CMAKE_ASM_FLAGS_DEBUG ${CMAKE_C_FLAGS_DEBUG} CACHE INTERNAL "Flags used by the ASM compiler during DEBUG builds.")
set(CMAKE_ASM_FLAGS_MINSIZEREL ${CMAKE_C_FLAGS_MINSIZEREL} CACHE INTERNAL "Flags used by the ASM compiler during MINSIZEREL builds.")
set(CMAKE_ASM_FLAGS_FAST ${CMAKE_C_FLAGS_RELEASE} CACHE INTERNAL "Flags used by the ASM compiler during RELEASE builds.")