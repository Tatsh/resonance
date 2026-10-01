# The original build passed each source file by its bare name. __FILE__ in an allocation tag or an
# assertion is therefore the file's basename (tupleobject.c, cutscene.c). Mapping each source
# file's directory to nothing gives the same strings.

function(_resonance_collect_targets directory out)
  get_property(targets DIRECTORY "${directory}" PROPERTY BUILDSYSTEM_TARGETS)
  get_property(children DIRECTORY "${directory}" PROPERTY SUBDIRECTORIES)
  foreach(child IN LISTS children)
    _resonance_collect_targets("${child}" child_targets)
    list(APPEND targets ${child_targets})
  endforeach()
  set(${out} "${targets}" PARENT_SCOPE)
endfunction()

function(resonance_basename_file_macros)
  foreach(directory IN LISTS ARGN)
    _resonance_collect_targets("${directory}" targets)
    foreach(target IN LISTS targets)
      get_target_property(type ${target} TYPE)
      if(type STREQUAL "INTERFACE_LIBRARY" OR type STREQUAL "UTILITY")
        continue()
      endif()
      get_target_property(sources ${target} SOURCES)
      get_target_property(source_dir ${target} SOURCE_DIR)
      foreach(source IN LISTS sources)
        if(source MATCHES "^\\$<")
          continue()
        endif()
        cmake_path(ABSOLUTE_PATH source BASE_DIRECTORY "${source_dir}" OUTPUT_VARIABLE absolute)
        cmake_path(GET absolute PARENT_PATH parent)
        set_property(
          SOURCE "${absolute}"
          DIRECTORY "${source_dir}"
          APPEND
          PROPERTY COMPILE_OPTIONS "-fmacro-prefix-map=${parent}/=")
      endforeach()
    endforeach()
  endforeach()
endfunction()
