# Builds the host tools under tools/ with the host compiler, in a separate build under
# ${CMAKE_BINARY_DIR}/tools. The tools are configured and built here, at configure time, for
# disc-region.cmake, and rebuilt by the resonance_tools target before a command runs one of them.
#
# Each RESONANCE_TOOL_* variable is the path of one tool. A command that runs a tool lists the
# variable and resonance_tools in its DEPENDS.

set(RESONANCE_TOOLS_CMAKE_ARGS
    ""
    CACHE STRING "Extra arguments for configuring the host tools, for example \
-DOPENSSL_ROOT_DIR=/opt/openssl.")
set(RESONANCE_TOOLS_VCPKG_ROOT
    ""
    CACHE PATH "vcpkg checkout that provides the host tools' dependencies listed in vcpkg.json at \
the repository root. The VCPKG_ROOT environment variable is used when RESONANCE_TOOLS_VCPKG_ROOT is \
empty.")

set(_resonance_tools_vcpkg_root "${RESONANCE_TOOLS_VCPKG_ROOT}")
if(NOT _resonance_tools_vcpkg_root)
  set(_resonance_tools_vcpkg_root "$ENV{VCPKG_ROOT}")
endif()
set(_resonance_tools_args ${RESONANCE_TOOLS_CMAKE_ARGS})
if(_resonance_tools_vcpkg_root)
  list(APPEND _resonance_tools_args
       "-DCMAKE_TOOLCHAIN_FILE=${_resonance_tools_vcpkg_root}/scripts/buildsystems/vcpkg.cmake"
       "-DVCPKG_MANIFEST_DIR=${CMAKE_SOURCE_DIR}")
  message(STATUS "The host tools use vcpkg from ${_resonance_tools_vcpkg_root}.")
endif()

set(RESONANCE_TOOLS_DIR "${CMAKE_BINARY_DIR}/tools")
set(RESONANCE_TOOL_BUILD_IMAGE "${RESONANCE_TOOLS_DIR}/build-image/build-image")
set(RESONANCE_TOOL_CREDITS_AVATAR "${RESONANCE_TOOLS_DIR}/credits-avatar/credits-avatar")
set(RESONANCE_TOOL_NAME_IRX "${RESONANCE_TOOLS_DIR}/name-irx/name-irx")

# The host build does not receive the cross toolchain file, on the command line or through the
# environment. Its only toolchain file is the host vcpkg one.
execute_process(
  COMMAND "${CMAKE_COMMAND}" -E env --unset=CMAKE_TOOLCHAIN_FILE "${CMAKE_COMMAND}" -S
          "${CMAKE_SOURCE_DIR}/tools" -B "${RESONANCE_TOOLS_DIR}" -G "${CMAKE_GENERATOR}"
          ${_resonance_tools_args}
  RESULT_VARIABLE _resonance_tools_result)
if(NOT _resonance_tools_result EQUAL 0)
  message(FATAL_ERROR "Configuring the host tools in ${RESONANCE_TOOLS_DIR} failed.")
endif()
execute_process(COMMAND "${CMAKE_COMMAND}" --build "${RESONANCE_TOOLS_DIR}"
                RESULT_VARIABLE _resonance_tools_result)
if(NOT _resonance_tools_result EQUAL 0)
  message(FATAL_ERROR "Building the host tools in ${RESONANCE_TOOLS_DIR} failed.")
endif()

add_custom_target(
  resonance_tools
  COMMAND "${CMAKE_COMMAND}" --build "${RESONANCE_TOOLS_DIR}"
  BYPRODUCTS "${RESONANCE_TOOL_BUILD_IMAGE}" "${RESONANCE_TOOL_CREDITS_AVATAR}"
             "${RESONANCE_TOOL_NAME_IRX}"
  COMMENT "Building the host tools"
  VERBATIM)
