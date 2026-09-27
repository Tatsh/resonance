set(RESONANCE_EZMPEG_SRCS
    audiodec.c
    disp.c
    readbuf.c
    strfile.c
    vibuf.c
    videodec.c
    vobuf.c)

add_library(resonance_ezmpeg STATIC ${RESONANCE_EZMPEG_SRCS})
target_link_libraries(resonance_ezmpeg PRIVATE resonance_common)
