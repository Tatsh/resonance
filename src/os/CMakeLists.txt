set(RESONANCE_OS_SRCS
    arkfile.cpp
    assert.cpp
    async.cpp
    asynccallback.cpp
    bytepairstatic.cpp
    circbuff.cpp
    cycles.cpp
    datetime.cpp
    devconsole.cpp
    failsink.cpp
    filelog.cpp
    formatmessage.cpp
    formatstring.cpp
    genpath.cpp
    heap.cpp
    hostmode.cpp
    hxstr.cpp
    inflate.c
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

# inflate.c includes the vendored gzip 1.2.4 inflate.c. The sign-compare warning concerns upstream
# style.
set_source_files_properties(inflate.c PROPERTIES INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/3rdparty"
                                                 COMPILE_OPTIONS -Wno-sign-compare)
