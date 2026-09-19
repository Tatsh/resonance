set(RESONANCE_GFX_SRCS
    gfxdevice.cpp
    vramtable.cpp)

add_library(resonance_gfx STATIC ${RESONANCE_GFX_SRCS})
target_link_libraries(resonance_gfx PRIVATE resonance_common)
