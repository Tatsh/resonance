# Every reconstructed source compiles into one object library. An object library is compiled and
# never archived or linked, which is what this project wants: the reconstruction is partial, so no
# link can succeed until every routine a call reaches exists.

file(GLOB_RECURSE RESONANCE_SRCS CONFIGURE_DEPENDS *.cpp)

# The bindings to the embedded interpreter need that interpreter's own Python 2 headers, which this
# tree records the differences of rather than carrying. scriptsink.cpp is not excluded, because it
# reaches none of them.
set(RESONANCE_EXCLUDED
    script/cxx/object.cpp
    script/hxmodule.cpp
    script/pyshell.cpp
    script/scripteval.cpp
    script/scriptfunc.cpp
    script/scripthost.cpp)

foreach(EXCLUDED ${RESONANCE_EXCLUDED})
  list(REMOVE_ITEM RESONANCE_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/${EXCLUDED}")
endforeach()

list(LENGTH RESONANCE_SRCS RESONANCE_SRC_COUNT)
message(STATUS "Compiling ${RESONANCE_SRC_COUNT} reconstructed source(s)")

add_library(resonance_objects OBJECT ${RESONANCE_SRCS})

target_include_directories(resonance_objects PUBLIC "${CMAKE_SOURCE_DIR}/include")

target_compile_options(resonance_objects PRIVATE -Wall -Wextra -Wno-unknown-pragmas)

# The link step the reconstruction is working towards. It stays commented out until every routine a
# call reaches has a body, because an incomplete tree produces a wall of undefined references that
# says nothing about the reconstruction. The startup file and the library set are what the original
# linked against.
#
# add_executable(${CMAKE_PROJECT_NAME} $<TARGET_OBJECTS:resonance_objects>)
# target_link_options(
#   ${CMAKE_PROJECT_NAME} PRIVATE -T"${PS2SDK}/ee/startup/linkfile"
#                                 -L"${PS2SDK}/ee/lib" -nostartfiles)
# target_link_libraries(
#   ${CMAKE_PROJECT_NAME} PRIVATE "${PS2SDK}/ee/startup/crt0.o" kernel c syscall
#                                 graph dma packet math3d)
# add_custom_command(
#   TARGET ${CMAKE_PROJECT_NAME}
#   POST_BUILD
#   COMMAND ${CMAKE_STRIP} --strip-all $<TARGET_FILE:${CMAKE_PROJECT_NAME}>
#   COMMENT "Stripping the executable, as the shipped disc image is stripped")
