# The embedded interpreter. Upstream CPython 2.0 translation units the manifest accounts for,
# compiled against the port's own PC/config.h, which comes first on the path so its allocator
# redirect wins over anything upstream with the same name. The sizes match the console rather
# than the host. posixmodule.c builds under the console module name through a define.
set(RESONANCE_PYTHON_SRCS
    "${CMAKE_SOURCE_DIR}/3rdparty/Python-2.0/Parser/acceler.c"
    "${CMAKE_SOURCE_DIR}/3rdparty/Python-2.0/Parser/myreadline.c"
    "${CMAKE_SOURCE_DIR}/3rdparty/Python-2.0/Parser/node.c"
    "${CMAKE_SOURCE_DIR}/3rdparty/Python-2.0/Parser/parser.c"
    "${CMAKE_SOURCE_DIR}/3rdparty/Python-2.0/Parser/parsetok.c"
    "${CMAKE_SOURCE_DIR}/3rdparty/Python-2.0/Parser/tokenizer.c"
    "${CMAKE_SOURCE_DIR}/3rdparty/Python-2.0/Python/bltinmodule.c"
    "${CMAKE_SOURCE_DIR}/3rdparty/Python-2.0/Python/ceval.c"
    "${CMAKE_SOURCE_DIR}/3rdparty/Python-2.0/Python/codecs.c"
    "${CMAKE_SOURCE_DIR}/3rdparty/Python-2.0/Python/compile.c"
    "${CMAKE_SOURCE_DIR}/3rdparty/Python-2.0/Python/errors.c"
    "${CMAKE_SOURCE_DIR}/3rdparty/Python-2.0/Python/exceptions.c"
    "${CMAKE_SOURCE_DIR}/3rdparty/Python-2.0/Python/frozen.c"
    "${CMAKE_SOURCE_DIR}/3rdparty/Python-2.0/Python/getargs.c"
    "${CMAKE_SOURCE_DIR}/3rdparty/Python-2.0/Python/graminit.c"
    "${CMAKE_SOURCE_DIR}/3rdparty/Python-2.0/Python/import.c"
    "${CMAKE_SOURCE_DIR}/3rdparty/Python-2.0/Python/marshal.c"
    "${CMAKE_SOURCE_DIR}/3rdparty/Python-2.0/Python/modsupport.c"
    "${CMAKE_SOURCE_DIR}/3rdparty/Python-2.0/Python/pystate.c"
    "${CMAKE_SOURCE_DIR}/3rdparty/Python-2.0/Python/pythonrun.c"
    "${CMAKE_SOURCE_DIR}/3rdparty/Python-2.0/Python/structmember.c"
    "${CMAKE_SOURCE_DIR}/3rdparty/Python-2.0/Python/sysmodule.c"
    "${CMAKE_SOURCE_DIR}/3rdparty/Python-2.0/Python/traceback.c"
    "${CMAKE_SOURCE_DIR}/3rdparty/Python-2.0/Objects/abstract.c"
    "${CMAKE_SOURCE_DIR}/3rdparty/Python-2.0/Objects/bufferobject.c"
    "${CMAKE_SOURCE_DIR}/3rdparty/Python-2.0/Objects/cobject.c"
    "${CMAKE_SOURCE_DIR}/3rdparty/Python-2.0/Objects/classobject.c"
    "${CMAKE_SOURCE_DIR}/3rdparty/Python-2.0/Objects/dictobject.c"
    "${CMAKE_SOURCE_DIR}/3rdparty/Python-2.0/Objects/fileobject.c"
    "${CMAKE_SOURCE_DIR}/3rdparty/Python-2.0/Objects/floatobject.c"
    "${CMAKE_SOURCE_DIR}/3rdparty/Python-2.0/Objects/frameobject.c"
    "${CMAKE_SOURCE_DIR}/3rdparty/Python-2.0/Objects/funcobject.c"
    "${CMAKE_SOURCE_DIR}/3rdparty/Python-2.0/Objects/intobject.c"
    "${CMAKE_SOURCE_DIR}/3rdparty/Python-2.0/Objects/listobject.c"
    "${CMAKE_SOURCE_DIR}/3rdparty/Python-2.0/Objects/longobject.c"
    "${CMAKE_SOURCE_DIR}/3rdparty/Python-2.0/Objects/methodobject.c"
    "${CMAKE_SOURCE_DIR}/3rdparty/Python-2.0/Objects/moduleobject.c"
    "${CMAKE_SOURCE_DIR}/3rdparty/Python-2.0/Objects/object.c"
    "${CMAKE_SOURCE_DIR}/3rdparty/Python-2.0/Objects/rangeobject.c"
    "${CMAKE_SOURCE_DIR}/3rdparty/Python-2.0/Objects/sliceobject.c"
    "${CMAKE_SOURCE_DIR}/3rdparty/Python-2.0/Objects/stringobject.c"
    "${CMAKE_SOURCE_DIR}/3rdparty/Python-2.0/Objects/tupleobject.c"
    "${CMAKE_SOURCE_DIR}/3rdparty/Python-2.0/Objects/typeobject.c"
    "${CMAKE_SOURCE_DIR}/3rdparty/Python-2.0/Objects/unicodeobject.c"
    "${CMAKE_SOURCE_DIR}/3rdparty/Python-2.0/Modules/arraymodule.c"
    "${CMAKE_SOURCE_DIR}/3rdparty/Python-2.0/Modules/_codecsmodule.c"
    "${CMAKE_SOURCE_DIR}/3rdparty/Python-2.0/Modules/cPickle.c"
    "${CMAKE_SOURCE_DIR}/3rdparty/Python-2.0/Modules/cStringIO.c"
    "${CMAKE_SOURCE_DIR}/3rdparty/Python-2.0/Modules/errnomodule.c"
    "${CMAKE_SOURCE_DIR}/3rdparty/Python-2.0/Modules/newmodule.c"
    "${CMAKE_SOURCE_DIR}/3rdparty/Python-2.0/Modules/pcremodule.c"
    "${CMAKE_SOURCE_DIR}/3rdparty/Python-2.0/Modules/posixmodule.c"
    "${CMAKE_SOURCE_DIR}/3rdparty/Python-2.0/Modules/pypcre.c"
    "${CMAKE_SOURCE_DIR}/3rdparty/Python-2.0/Modules/regexmodule.c"
    "${CMAKE_SOURCE_DIR}/3rdparty/Python-2.0/Modules/regexpr.c"
    "${CMAKE_SOURCE_DIR}/3rdparty/Python-2.0/Modules/signalmodule.c"
    "${CMAKE_SOURCE_DIR}/3rdparty/Python-2.0/Modules/socketmodule.c"
    "${CMAKE_SOURCE_DIR}/3rdparty/Python-2.0/Modules/_sre.c"
    "${CMAKE_SOURCE_DIR}/3rdparty/Python-2.0/Modules/stropmodule.c"
    "${CMAKE_SOURCE_DIR}/3rdparty/Python-2.0/Modules/structmodule.c"
    "${CMAKE_SOURCE_DIR}/3rdparty/Python-2.0/PC/getpathp.c"
    PC/config.c)

add_library(resonance_python STATIC ${RESONANCE_PYTHON_SRCS})
target_link_libraries(resonance_python PRIVATE resonance_common)
target_include_directories(resonance_python PRIVATE "${CMAKE_SOURCE_DIR}/src/python/PC"
                                                   "${CMAKE_SOURCE_DIR}/3rdparty/Python-2.0/Include")
target_compile_definitions(
  resonance_python
  PRIVATE _GNU_SOURCE
          HAVE_PROTOTYPES=1
          HAVE_STDARG_PROTOTYPES=1
          HAVE_DIRENT_H=1
          HAVE_FCNTL_H=1
          HAVE_NETDB_H=1
          HAVE_SIGNAL_H=1
          HAVE_SYS_SOCKET_H=1
          HAVE_SYS_WAIT_H=1
          HAVE_UNISTD_H=1
          HAVE_UTIME_H=1
          SIZEOF_INT=4
          SIZEOF_LONG=4
          SIZEOF_VOID_P=4
          SIZEOF_SHORT=2
          SIZEOF_DOUBLE=8)
target_compile_options(
  resonance_python PRIVATE "-include${CMAKE_SOURCE_DIR}/src/python/PC/pycompat.h")
# Four translation units defeat the optimiser's register allocator on this toolchain at every
# standard level, and the shipped image carries no stack guards. They build small without
# small-data addressing, multiply-add fusion, builtin expansion, scheduling, delay slots,
# branch-likely, or stack protection until the failure is understood.
set_source_files_properties(
  "${CMAKE_SOURCE_DIR}/3rdparty/Python-2.0/Python/getargs.c"
  "${CMAKE_SOURCE_DIR}/3rdparty/Python-2.0/Python/marshal.c"
  "${CMAKE_SOURCE_DIR}/3rdparty/Python-2.0/Python/compile.c"
  "${CMAKE_SOURCE_DIR}/3rdparty/Python-2.0/Python/bltinmodule.c"
  PROPERTIES
    COMPILE_OPTIONS
    "-Os;-G0;-mno-explicit-relocs;-mno-mad;-fno-builtin;-fno-schedule-insns;-fno-schedule-insns2;-fno-delayed-branch;-mno-branch-likely;-fno-stack-protector;-fno-stack-clash-protection"
)
set_source_files_properties(
  "${CMAKE_SOURCE_DIR}/3rdparty/Python-2.0/Modules/posixmodule.c" PROPERTIES COMPILE_DEFINITIONS
                                                                            initposix=initps2)
