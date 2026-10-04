add_executable(
  build-image
  directoryvolume.cpp
  githubclient.cpp
  imagebuilder.cpp
  main.cpp
  sectorencoder.cpp
  sourceimage.cpp
  volume.cpp
  ziparchive.cpp)
target_link_libraries(
  build-image PRIVATE argparse::argparse nlohmann_json::nlohmann_json resonance_tools_common
                      resonance_tools_https spdlog::spdlog ZLIB::ZLIB)
