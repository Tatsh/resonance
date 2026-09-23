set(RESONANCE_OS_SRCS
    arkfile.cpp
    async.cpp
    asynccallback.cpp
    circbuff.cpp
    cycles.cpp
    datetime.cpp
    devconsole.cpp
    failsink.cpp
    formatstring.cpp
    genpath.cpp
    heap.cpp
    hostmode.cpp
    hxstr.cpp
    iop.cpp
    loadfile.cpp
    log.cpp
    mem.cpp
    r250.cpp
    random.cpp
    seccache.cpp
    spew.cpp
    spewregistrar.cpp
    zone.cpp)

add_library(resonance_os STATIC ${RESONANCE_OS_SRCS})
target_link_libraries(resonance_os PRIVATE resonance_common)
