set(RESONANCE_EZMPEG_SRCS
    sample/audiodec.c
    sample/disp.c
    sample/readbuf.c
    sample/strfile.c
    sample/vibuf.c
    sample/videodec.c
    sample/vobuf.c)

add_library(resonance_ezmpeg STATIC ${RESONANCE_EZMPEG_SRCS})
target_link_libraries(resonance_ezmpeg PRIVATE resonance_common)
target_include_directories(resonance_ezmpeg PRIVATE "${CMAKE_SOURCE_DIR}/sce/ee/include")
