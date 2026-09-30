# Builds cmake/ieee-single-plugin.cpp with the host compiler and loads it into every cross
# compilation. The plugin is a GCC plugin. It builds only against the cross compiler's plugin
# headers and only when a host C++ compiler is present.
if(NOT CMAKE_C_COMPILER_ID STREQUAL "GNU" OR NOT CMAKE_C_COMPILER MATCHES "r5900")
  return()
endif()
execute_process(
  COMMAND "${CMAKE_C_COMPILER}" -print-file-name=plugin
  OUTPUT_VARIABLE _plugin_dir
  OUTPUT_STRIP_TRAILING_WHITESPACE)
find_program(HOST_CXX NAMES c++ g++)
set(_plugin "${CMAKE_BINARY_DIR}/ieee-single-plugin.so")
set(_source "${CMAKE_SOURCE_DIR}/cmake/ieee-single-plugin.cpp")
if(NOT HOST_CXX OR NOT EXISTS "${_plugin_dir}/include/gcc-plugin.h")
  message(WARNING "The IEEE single-precision plugin cannot be built. Float constants will be one "
                  "unit in the last place low wherever rounding and truncation differ.")
  return()
endif()
if(NOT EXISTS "${_plugin}" OR "${_source}" IS_NEWER_THAN "${_plugin}")
  execute_process(
    COMMAND "${HOST_CXX}" -shared -fPIC -fno-rtti -O2 "-I${_plugin_dir}/include" "${_source}" -o
            "${_plugin}"
    RESULT_VARIABLE _result)
  if(NOT _result EQUAL 0)
    message(FATAL_ERROR "Building the IEEE single-precision plugin failed.")
  endif()
endif()
add_compile_options("-fplugin=${_plugin}")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${_source}")
