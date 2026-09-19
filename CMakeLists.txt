cmake_minimum_required(VERSION 3.25)
project(resonance VERSION 0.0.2 LANGUAGES CXX)

option(BUILD_DOCS "Build documentation alongside the sources." OFF)
option(BUILD_DOCS_ONLY "Build documentation and nothing else." OFF)
option(INSTALL_DOCS "Install documentation." OFF)

# The reconstruction is checked as C++17. The target toolchain accepts a later standard, and raising
# it here would mean the tree is no longer checked at the level it is written to.
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(BUILD_DOCS OR BUILD_DOCS_ONLY)
  set(DOXYGEN_GENERATE_HTML YES)
  set(DOXYGEN_GENERATE_TREEVIEW YES)
  set(DOXYGEN_HTML_OUTPUT "${CMAKE_BINARY_DIR}/html")
  set(DOXYGEN_JAVADOC_AUTOBRIEF YES)
  set(DOXYGEN_MARKDOWN_ID_STYLE GITHUB)
  set(DOXYGEN_USE_MDFILE_AS_MAINPAGE "${CMAKE_SOURCE_DIR}/README.md")
  set(DOXYGEN_INTERACTIVE_SVG YES)
  set(DOXYGEN_GENERATE_TAGFILE "${CMAKE_BINARY_DIR}/html/${CMAKE_PROJECT_NAME}.tags")
  set(DOXYGEN_EXTRACT_ALL YES)
  set(DOXYGEN_EXTRACT_STATIC YES)
  set(DOXYGEN_SORT_MEMBER_DOCS NO)
  set(DOXYGEN_ENABLE_PREPROCESSING YES)
  set(DOXYGEN_MACRO_EXPANSION YES)
  set(DOXYGEN_EXPAND_ONLY_PREDEF YES)
  set(DOXYGEN_PREDEFINED "_EE")
  # Every reconstructed routine records the address it was recovered from with this tag.
  set(DOXYGEN_VERBATIM_VARS DOXYGEN_ALIASES)
  set(DOXYGEN_ALIASES [[ghidraAddress="\par Ghidra address:^^"]])
  set(DOXYGEN_EXCLUDE_PATTERNS "*/node_modules/*" "*/build/*" "*/src/python/*")
  find_package(Doxygen REQUIRED COMPONENTS dot)
  doxygen_add_docs(docs README.md PROGRESS.md include src ALL)
  if(INSTALL_DOCS)
    install(
      DIRECTORY "${CMAKE_BINARY_DIR}/html"
      DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/doc/${CMAKE_PROJECT_NAME}-${CMAKE_PROJECT_VERSION}")
  endif()
endif()

# A docs-only configure must not require the cross toolchain, so the sources are skipped entirely.
if(NOT BUILD_DOCS_ONLY)
  # The SDK toolchain file sets this in its own scope, which does not reach here, and a host build
  # configures without any toolchain file at all. Reading the environment covers both, and the
  # sibling checkout is the fallback for a host build with nothing exported.
  if(NOT PS2SDK)
    if(DEFINED ENV{PS2SDK})
      set(PS2SDK "$ENV{PS2SDK}" CACHE PATH "PlayStation 2 SDK root.")
    else()
      set(PS2SDK "${CMAKE_SOURCE_DIR}/../ps2sdk" CACHE PATH "PlayStation 2 SDK root.")
    endif()
  endif()
  message(STATUS "PlayStation 2 SDK at ${PS2SDK}")
  include(FeatureSummary)
  include(GNUInstallDirs)
  feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
  add_subdirectory(src)
endif()
