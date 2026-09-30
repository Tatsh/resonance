# Each subsystem directory builds its own static library and lists its sources explicitly. A
# recursive glob would hide what is built. A source added to the tree would join the build with no
# record of the decision, and a source removed would disappear the same way.
#
# Static libraries are deliberate. Archiving produces the object files and the archives without
# linking an executable. The reconstruction is partial, and no executable link can succeed until
# every routine a call arrives at has a body.

# Everything the subsystem libraries share. An interface library provides the settings without
# compiling anything itself.
add_library(resonance_common INTERFACE)
target_compile_options(resonance_common INTERFACE -Wall -Wextra -Wno-unknown-pragmas)

# _EE selects the Emotion Engine side of the headers serving both processors.
target_compile_definitions(resonance_common INTERFACE _EE)

# The reconstructed Sony headers sit under sce/ee/include, and compat declares the rest of the Sony
# interfaces the game calls.
target_include_directories(
  resonance_common INTERFACE "${CMAKE_SOURCE_DIR}/include" "${CMAKE_SOURCE_DIR}/sce/ee/include"
                             "${CMAKE_SOURCE_DIR}/compat")

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
    netflow
    os
    profile
    python
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
# It is an object library so the executable below links the same objects the archive step builds.
add_library(resonance_main OBJECT main.cpp)
target_link_libraries(resonance_main PRIVATE resonance_common)

# Every archive the build produces, for a target that builds all of them at once.
set(RESONANCE_LIBRARIES resonance_main)
foreach(SUBSYSTEM ${RESONANCE_SUBSYSTEMS})
  list(APPEND RESONANCE_LIBRARIES resonance_${SUBSYSTEM})
endforeach()

# The Sony sample archive lives under sce/ee rather than with the game subsystems.
list(APPEND RESONANCE_LIBRARIES resonance_ezmpeg)

# The Sony library reconstructions live under sce/ee with the sample.
list(APPEND RESONANCE_LIBRARIES resonance_sonylibs)

add_custom_target(archives DEPENDS ${RESONANCE_LIBRARIES})

# The executable links the reconstructed start-up code, every archive, and the compiler runtime
# (the C and C++ libraries and libgcc) with the reconstructed link script. The compiler driver's
# default start files and libraries belong to an SDK this tree does not use. Both are replaced.
add_executable(${CMAKE_PROJECT_NAME} $<TARGET_OBJECTS:resonance_main>
                                     "${CMAKE_SOURCE_DIR}/sce/ee/src/crt0.s")
enable_language(ASM)
# The original linked every object. The metagame screens and the script interface register
# themselves from static initialisation with nothing else referring to them. Both therefore link
# whole.
set(RESONANCE_ARCHIVE_LIBRARIES ${RESONANCE_LIBRARIES})
list(REMOVE_ITEM RESONANCE_ARCHIVE_LIBRARIES resonance_main resonance_met resonance_script)
foreach(_crt crti.o crtbegin.o crtend.o crtn.o)
  execute_process(
    COMMAND ${CMAKE_C_COMPILER} -print-file-name=${_crt}
    OUTPUT_VARIABLE _crt_path
    OUTPUT_STRIP_TRAILING_WHITESPACE)
  string(REPLACE "." "_" _crt_var "${_crt}")
  set(RESONANCE_${_crt_var} "${_crt_path}")
endforeach()
target_link_libraries(
  ${CMAKE_PROJECT_NAME}
  PRIVATE "${RESONANCE_crti_o}"
          "${RESONANCE_crtbegin_o}"
          -Wl,--whole-archive
          resonance_met
          resonance_script
          -Wl,--no-whole-archive
          -Wl,--start-group
          ${RESONANCE_ARCHIVE_LIBRARIES}
          resonance_runtime
          stdc++
          m
          c
          gcc
          -Wl,--end-group
          "${RESONANCE_crtend_o}"
          "${RESONANCE_crtn_o}")
# MemOpenLog() at 0x004a7c40 reports the original stack as 512 KiB at 0x01F80000.
target_link_options(
  ${CMAKE_PROJECT_NAME}
  PRIVATE
  -nostartfiles
  -nodefaultlibs
  -T${CMAKE_SOURCE_DIR}/sce/ee/app.ld
  -Wl,--defsym,_stack=0x01F80000
  -Wl,--defsym,_stack_size=0x80000)
set_property(TARGET ${CMAKE_PROJECT_NAME} APPEND PROPERTY LINK_DEPENDS
                                                          "${CMAKE_SOURCE_DIR}/sce/ee/app.ld")
# The shipped disc image is stripped, so release builds strip the executable. Debug builds keep
# their symbols.
if(CMAKE_BUILD_TYPE STREQUAL "Release")
  add_custom_command(
    TARGET ${CMAKE_PROJECT_NAME}
    POST_BUILD
    COMMAND ${CMAKE_STRIP} --strip-all $<TARGET_FILE:${CMAKE_PROJECT_NAME}>
    COMMENT "Stripping the executable, as the shipped disc image is stripped")
endif()
