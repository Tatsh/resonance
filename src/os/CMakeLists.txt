set(RESONANCE_OS_SRCS
    arkfile.cpp
    async.cpp
    asynccallback.cpp
    cycles.cpp
    failsink.cpp
    formatstring.cpp
    heap.cpp
    hostmode.cpp
    hxstr.cpp
    iop.cpp
    loadfile.cpp
    log.cpp
    mem.cpp
    seccache.cpp
    zone.cpp)

# Each of these reconstructs a routine whose body ignores a parameter its
# signature carries, which the call sites prove the routine takes. The warning is
# therefore reporting a property of the original rather than a defect here, and
# it is disabled for these sources only.
set_source_files_properties(
    cycles.cpp
    failsink.cpp
    heap.cpp
    mem.cpp
    PROPERTIES COMPILE_OPTIONS -Wno-unused-parameter)

add_library(resonance_os STATIC ${RESONANCE_OS_SRCS})
target_link_libraries(resonance_os PRIVATE resonance_common)
