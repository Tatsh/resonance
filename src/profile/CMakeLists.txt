set(RESONANCE_PROFILE_SRCS
    profiler.cpp)

add_library(resonance_profile STATIC ${RESONANCE_PROFILE_SRCS})
target_link_libraries(resonance_profile PRIVATE resonance_common)
