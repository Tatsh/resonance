# Six sources of this directory are not built. They reach the embedded interpreter's own
# Python 2 headers, and this tree records that interpreter's differences rather than
# carrying its headers.
set(RESONANCE_SCRIPT_SRCS
    scriptsink.cpp)

add_library(resonance_script STATIC ${RESONANCE_SCRIPT_SRCS})
target_link_libraries(resonance_script PRIVATE resonance_common)
