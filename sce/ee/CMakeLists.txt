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

# The Sony library reconstructions. Each unit lists its sources explicitly, as the subsystem
# libraries do. libdma is absent until its routines are recovered.
set(RESONANCE_SONYLIB_SRCS
    src/fileio.c
    src/iopcontrol.c
    src/iopheap.c
    src/libcdvd.c
    src/libdev.c
    src/libdma.c
    src/libgifpk.c
    src/libgraph.c
    src/libkernel.c
    src/libkernel_alarm.s
    src/libkernel_syscalls.s
    src/libmc.c
    src/libmpeg.c
    src/libmtap.c
    src/libpad.c
    src/libsdr.c
    src/libvifpk.c
    src/libvu0.c
    src/loadfile.c
    src/msin.c
    src/libscf.c
    src/sifcmd.c
    src/sifdma.c
    src/sifrpc.c
    src/tty.c)

add_library(resonance_sonylibs STATIC ${RESONANCE_SONYLIB_SRCS})
target_link_libraries(resonance_sonylibs PRIVATE resonance_common)
# The original libraries assert in the shipped build. No build type may compile the assertions out.
target_compile_options(resonance_sonylibs PRIVATE -UNDEBUG)
target_include_directories(resonance_sonylibs PRIVATE "${CMAKE_SOURCE_DIR}/sce/ee/include")

# The system layer the compiler's C and C++ runtime needs, over the reconstructed kernel.
add_library(resonance_runtime STATIC runtime/newlib.c runtime/pthread.c runtime/softfloat.c)
target_link_libraries(resonance_runtime PRIVATE resonance_common)
