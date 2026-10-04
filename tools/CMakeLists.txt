# The main build runs the host tools in this project to write the disc image, write the module name
# into EZMIDI.IRX, and generate the credits avatar header. The main build configures this project with the host
# compiler. The tools are never installed.
#
# Each dependency comes from vcpkg when the vcpkg toolchain is in use, otherwise from the system,
# and otherwise from the pinned release below. zlib and OpenSSL come only from vcpkg or the system.
# The vcpkg manifest is vcpkg.json at the repository root, selected through VCPKG_MANIFEST_DIR.
cmake_minimum_required(VERSION 3.24)
project(resonance_tools CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

include(FetchContent)

# The options below apply to the pinned releases only when a release is downloaded.
set(ARGPARSE_INSTALL OFF)
set(ELFIO_BUILD_EXAMPLES OFF)
set(HTTPLIB_INSTALL OFF)
set(HTTPLIB_USE_BROTLI_IF_AVAILABLE OFF)
set(HTTPLIB_USE_OPENSSL_IF_AVAILABLE OFF)
set(HTTPLIB_USE_ZLIB_IF_AVAILABLE OFF)
set(HTTPLIB_USE_ZSTD_IF_AVAILABLE OFF)
set(JSON_Install OFF)
set(SPDLOG_INSTALL OFF)
FetchContent_Declare(
  argparse
  URL https://github.com/p-ranav/argparse/archive/refs/tags/v3.2.tar.gz
  URL_HASH SHA256=9dcb3d8ce0a41b2a48ac8baa54b51a9f1b6a2c52dd374e28cc713bab0568ec98
  FIND_PACKAGE_ARGS CONFIG)
FetchContent_Declare(
  elfio
  URL https://github.com/serge1/ELFIO/archive/refs/tags/Release_3.12.tar.gz
  URL_HASH SHA256=e4ebc9ce3d6916461bc3e7765bb45e6210f0a9b93978bf91e59b05388c024489
  FIND_PACKAGE_ARGS CONFIG)
FetchContent_Declare(
  httplib
  URL https://github.com/yhirose/cpp-httplib/archive/refs/tags/v0.59.0.tar.gz
  URL_HASH SHA256=7c8cc7df044abb837d75f7c1e56333305acb40335c3788b8f5fbf5927e63e148
  FIND_PACKAGE_ARGS CONFIG)
FetchContent_Declare(
  nlohmann_json
  URL https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz
  URL_HASH SHA256=42f6e95cad6ec532fd372391373363b62a14af6d771056dbfc86160e6dfff7aa
  FIND_PACKAGE_ARGS CONFIG)
FetchContent_Declare(
  spdlog
  URL https://github.com/gabime/spdlog/archive/refs/tags/v1.17.0.tar.gz
  URL_HASH SHA256=d8862955c6d74e5846b3f580b1605d2428b11d97a410d86e2fb13e857cd3a744
  FIND_PACKAGE_ARGS CONFIG)
FetchContent_MakeAvailable(argparse elfio httplib nlohmann_json spdlog)
find_package(ZLIB REQUIRED)
find_package(OpenSSL 3.0 COMPONENTS Crypto SSL)

# Reports whether a dependency came from vcpkg, the system, or a download. location is a path
# from the package search, empty when the package was downloaded.
function(resonance_tools_report_source dependency location)
  if(NOT location)
    set(_source "the pinned release (FetchContent)")
  elseif(VCPKG_INSTALLED_DIR)
    string(FIND "${location}" "${VCPKG_INSTALLED_DIR}" _vcpkg_at)
    if(_vcpkg_at EQUAL 0)
      set(_source "vcpkg (${location})")
    else()
      set(_source "the system (${location})")
    endif()
  else()
    set(_source "the system (${location})")
  endif()
  message(STATUS "${dependency} comes from ${_source}.")
endfunction()

foreach(_package argparse elfio httplib nlohmann_json spdlog)
  set(_location "")
  if(${_package}_FOUND)
    set(_location "${${_package}_DIR}")
  endif()
  resonance_tools_report_source(${_package} "${_location}")
endforeach()
list(GET ZLIB_INCLUDE_DIRS 0 _zlib_location)
resonance_tools_report_source(zlib "${_zlib_location}")

# resonance_tools_https is the HTTP client, with HTTPS when OpenSSL is available. Without OpenSSL, the tools report that
# network features are unavailable.
add_library(resonance_tools_https INTERFACE)
target_link_libraries(resonance_tools_https INTERFACE httplib::httplib)
if(OpenSSL_FOUND)
  resonance_tools_report_source(OpenSSL "${OPENSSL_INCLUDE_DIR}")
  target_compile_definitions(resonance_tools_https INTERFACE CPPHTTPLIB_OPENSSL_SUPPORT)
  target_link_libraries(resonance_tools_https INTERFACE OpenSSL::Crypto OpenSSL::SSL)
else()
  message(STATUS "OpenSSL was not found. The tools cannot use HTTPS.")
endif()

add_subdirectory(common)
add_subdirectory(build-image)
add_subdirectory(credits-avatar)
add_subdirectory(name-irx)
