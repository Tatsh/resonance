set(RESONANCE_STREAM_SRCS
    hxmemstream.cpp
    hxstream.cpp
    ibfilestream.cpp
    ibstream.cpp
    iobmemstream.cpp
    iobpreallocmemstream.cpp
    obfilestream.cpp
    obstream.cpp)

# Each of these reconstructs a routine whose body ignores a parameter its
# signature declares. The call sites prove the routine takes that parameter. The
# warning therefore reports a property of the original rather than a defect here,
# and it is disabled for these sources only.
set_source_files_properties(
    hxmemstream.cpp
    hxstream.cpp
    PROPERTIES COMPILE_OPTIONS -Wno-unused-parameter)

add_library(resonance_stream STATIC ${RESONANCE_STREAM_SRCS})
target_link_libraries(resonance_stream PRIVATE resonance_common)
