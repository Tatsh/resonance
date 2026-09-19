# Each subsystem directory builds its own static library, and each lists its sources explicitly.
# A recursive glob would hide what is built: a source added to the tree would join the build with no
# record of the decision, and a source removed would disappear the same way.
#
# Static libraries are deliberate. Archiving produces the object files and the archives without
# linking an executable, and the reconstruction is partial, so no executable link can succeed until
# every routine a call reaches has a body.

# Everything the subsystem libraries share. An interface library carries the settings without
# compiling anything itself.
add_library(resonance_common INTERFACE)
target_compile_options(resonance_common INTERFACE -Wall -Wextra -Wno-unknown-pragmas)

# _EE selects the Emotion Engine side of every SDK header serving both processors. The SDK toolchain
# file defines it, and a host configure has no toolchain file, where its absence makes tamtypes.h
# refuse to define u32 and every use of that type then fails.
target_compile_definitions(resonance_common INTERFACE _EE)

# compat comes first so that its #include_next reaches the real SDK header behind it. The original
# was built against Sony's official SDK, whose header names differ from the open-source ps2sdk in
# places, and compat forwards those names rather than redeclaring anything.
target_include_directories(resonance_common INTERFACE "${CMAKE_SOURCE_DIR}/include"
                                                      "${CMAKE_SOURCE_DIR}/compat")

# Five headers the reconstruction includes do not sit in the two directories the SDK toolchain file
# adds. An installed SDK flattens them into ee/include and a source checkout keeps them where they
# are built from, so each candidate is added only where it exists and neither layout is assumed.
foreach(
  SDK_INCLUDE
  ee/include
  common/include
  ee/kernel/include
  ee/rpc/cdvd/include
  ee/rpc/memorycard/include
  ee/rpc/multitap/include
  ee/rpc/sdr/include)
  if(IS_DIRECTORY "${PS2SDK}/${SDK_INCLUDE}")
    target_include_directories(resonance_common SYSTEM INTERFACE "${PS2SDK}/${SDK_INCLUDE}")
  endif()
endforeach()

set(RESONANCE_SUBSYSTEMS
    app
    game
    gfx
    gs
    math
    memcard
    met
    mid
    msg
    os
    rnd
    rndartt
    sch
    script
    stream
    synth)

foreach(SUBSYSTEM ${RESONANCE_SUBSYSTEMS})
  add_subdirectory(${SUBSYSTEM})
endforeach()

# The entry point sits at the root of the tree rather than in a subsystem, matching the original.
add_library(resonance_main STATIC main.cpp)
target_link_libraries(resonance_main PRIVATE resonance_common)

# Every archive the build produces, for a target that builds all of them at once.
set(RESONANCE_LIBRARIES resonance_main)
foreach(SUBSYSTEM ${RESONANCE_SUBSYSTEMS})
  list(APPEND RESONANCE_LIBRARIES resonance_${SUBSYSTEM})
endforeach()

add_custom_target(archives DEPENDS ${RESONANCE_LIBRARIES})

# The link step the reconstruction is working towards. It stays commented out until every routine a
# call reaches has a body, because an incomplete tree produces a wall of undefined references that
# reports nothing about the reconstruction. The startup file and the library set are what the
# original linked against, and the toolchain file the SDK ships supplies the linker flags and the
# link script.
#
# add_executable(${CMAKE_PROJECT_NAME} $<TARGET_OBJECTS:resonance_main>)
# target_link_libraries(
#   ${CMAKE_PROJECT_NAME} PRIVATE ${RESONANCE_LIBRARIES} kernel c syscall graph dma packet math3d)
# add_custom_command(
#   TARGET ${CMAKE_PROJECT_NAME}
#   POST_BUILD
#   COMMAND ${CMAKE_STRIP} --strip-all $<TARGET_FILE:${CMAKE_PROJECT_NAME}>
#   COMMENT "Stripping the executable, as the shipped disc image is stripped")
