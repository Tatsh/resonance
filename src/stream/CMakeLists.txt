set(RESONANCE_STREAM_SRCS
    hxmemstream.cpp
    hxstream.cpp
    ibfilestream.cpp
    ibstream.cpp
    iobmemstream.cpp
    iobpreallocmemstream.cpp
    obfilestream.cpp
    obstream.cpp)

add_library(resonance_stream STATIC ${RESONANCE_STREAM_SRCS})
target_link_libraries(resonance_stream PRIVATE resonance_common)
