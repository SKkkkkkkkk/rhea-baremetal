set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Debug, Release, RelWithDebInfo, MinSizeRel")

set(CMAKE_C_FLAGS_DEBUG "-Og -g3 -DDEBUG=1" CACHE INTERNAL "Flags used by the C compiler during DEBUG builds.")
set(CMAKE_C_FLAGS_MINSIZEREL "-Os -DNDEBUG" CACHE INTERNAL "Flags used by the C compiler during MINSIZEREL builds.")
set(CMAKE_C_FLAGS_RELEASE "-O3 -DNDEBUG" CACHE INTERNAL "Flags used by the C compiler during RELEASE builds.")
set(CMAKE_C_FLAGS_RELWITHDEBINFO "-O3 -g3 -DNDEBUG" CACHE INTERNAL "Flags used by the C compiler during RELWITHDEBINFO builds.")

set(CMAKE_ASM_FLAGS_DEBUG ${CMAKE_C_FLAGS_DEBUG} CACHE INTERNAL "Flags used by the ASM compiler during DEBUG builds.")
set(CMAKE_ASM_FLAGS_MINSIZEREL ${CMAKE_C_FLAGS_MINSIZEREL} CACHE INTERNAL "Flags used by the ASM compiler during MINSIZEREL builds.")
set(CMAKE_ASM_FLAGS_RELEASE ${CMAKE_C_FLAGS_RELEASE} CACHE INTERNAL "Flags used by the ASM compiler during RELEASE builds.")
set(CMAKE_ASM_FLAGS_RELWITHDEBINFO ${CMAKE_C_FLAGS_RELWITHDEBINFO} CACHE INTERNAL "Flags used by the ASM compiler during RELWITHDEBINFO builds.")