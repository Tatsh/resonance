set(RESONANCE_RND_SRCS
    animatable.cpp
    arena.cpp
    blur.cpp
    bufstream.cpp
    button.cpp
    cam.cpp
    collideable.cpp
    drawable.cpp
    environ.cpp
    filestream.cpp
    font.cpp
    generator.cpp
    keychannel.cpp
    light.cpp
    lightanim.cpp
    manager.cpp
    mat.cpp
    matanim.cpp
    memstream.cpp
    mesh.cpp
    meshanim.cpp
    movie.cpp
    multimesh.cpp
    object.cpp
    particlesys.cpp
    particlesysanim.cpp
    pscam.cpp
    psmat.cpp
    psmesh.cpp
    psmultimesh.cpp
    psparticlesys.cpp
    pstex.cpp
    rndtex.cpp
    stream.cpp
    string.cpp
    text.cpp
    toolstream.cpp
    transanim.cpp
    transformable.cpp
    tunnel.cpp
    view.cpp)

# Each of these reconstructs a routine whose body ignores a parameter its
# signature declares. The call sites prove the routine takes that parameter. The
# warning therefore reports a property of the original rather than a defect here,
# and it is disabled for these sources only.
set_source_files_properties(
    animatable.cpp
    button.cpp
    environ.cpp
    font.cpp
    mat.cpp
    mesh.cpp
    toolstream.cpp
    PROPERTIES COMPILE_OPTIONS -Wno-unused-parameter)

add_library(resonance_rnd STATIC ${RESONANCE_RND_SRCS})
target_link_libraries(resonance_rnd PRIVATE resonance_common)
