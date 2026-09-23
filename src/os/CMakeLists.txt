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
    random.cpp
    seccache.cpp
    zone.cpp)

add_library(resonance_os STATIC ${RESONANCE_OS_SRCS})
target_link_libraries(resonance_os PRIVATE resonance_common)
