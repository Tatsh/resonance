# Six sources of this directory are not built. Each one includes the embedded
# interpreter's own Python 2 headers. This tree records that interpreter's differences
# rather than including its headers.
set(RESONANCE_SCRIPT_SRCS
    scriptsink.cpp
    scripttemplatemap.cpp)

add_library(resonance_script STATIC ${RESONANCE_SCRIPT_SRCS})
target_link_libraries(resonance_script PRIVATE resonance_common)
