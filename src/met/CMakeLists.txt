# The interpreter headers the two scripting-adjacent units need are vendored under 3rdparty.
set(RESONANCE_MET_SRCS
    albumcache.cpp
    creditsroll.cpp
    discswap.cpp
    fadeuser.cpp
    gameoptions.cpp
    listdataprovider.cpp
    metarenasscreen.cpp
    metbuttonlist.cpp
    metcommandmap.cpp
    metcommandrepeater.cpp
    metconfigcontrollerscreen.cpp
    metconfiggameoptionsscreen.cpp
    metconfigoptionsbuttonsscreen.cpp
    metcreditsscreen.cpp
    metdemostagesel.cpp
    metendgamegizmoscreen.cpp
    metexpansionpakscreen.cpp
    metfade.cpp
    metfreqcreatescreen.cpp
    metfreqloader.cpp
    metfreqmakerassetmanager.cpp
    metfreqmakerbuttonsscreen.cpp
    metfreqmakercanvasscreen.cpp
    metfreqmakerdirectionsscreen.cpp
    metfreqmakerinventoryscreen.cpp
    metfrontendstate.cpp
    metgameskillscreen.cpp
    metgizmopanel.cpp
    methelpscreen.cpp
    metglobalsettingssaverscreen.cpp
    metjukeboxbasescreen.cpp
    metjukeboxcustomremixesscreen.cpp
    metjukeboxeditplaylistscreen.cpp
    metjukeboxeditplaylistscreendone.cpp
    metjukeboxeditplaylistscreenlowerleft.cpp
    metjukeboxfactoryremixesscreen.cpp
    metjukeboxtopbuttonsscreen.cpp
    metkbuser.cpp
    metkeyboardrequest.cpp
    metkeyboardscreen.cpp
    metleftgizmoscreen.cpp
    metleftgizmosmallscreen.cpp
    metloadfreqbasescreen.cpp
    metloadfreqscreen.cpp
    metloadgamescreen.cpp
    metloadnewfreqscreen.cpp
    metloadprefabscreen.cpp
    metlocnumplayscreen.cpp
    metlocpickcharscreen.cpp
    metlogoscreen.cpp
    metmainscreen.cpp
    metmcfreqdelscreen.cpp
    metmemcardloadscreen.cpp
    metmemcardtypescreen.cpp
    metmemdetectscreen.cpp
    metmemdetectstartup.cpp
    metmodescreen.cpp
    metmsgscreen.cpp
    metmultiendremixscreen.cpp
    metmultiendscreen.cpp
    metmultisaveremixscreen.cpp
    metmultistatsscreen.cpp
    metmultitips1screen.cpp
    metmultitips2screen.cpp
    metmultitips3screen.cpp
    metmultitips4screen.cpp
    metmultitips5screen.cpp
    metmultitipsbasescreen.cpp
    metnullrenderer.cpp
    metpausebasescreen.cpp
    metpausegamescreen.cpp
    metpausemultiremixscreen.cpp
    metpausesologamescreen.cpp
    metpausesoloremixscreen.cpp
    metpersonadata.cpp
    metpersonasaverscreen.cpp
    metremixdatascreen.cpp
    metremixdelscreen.cpp
    metremixloadscreen.cpp
    metremixmanager.cpp
    metremixsaver.cpp
    metremixtypescreen.cpp
    metrenderer.cpp
    metrightgizmoscreen.cpp
    metsaveremix.cpp
    metsaveremixscreen.cpp
    metscreen.cpp
    metscreenmultisoundbank.cpp
    metscreentitlescreen.cpp
    metsoloendremixscreen.cpp
    metsololosescreen.cpp
    metsolostagesscreen.cpp
    metsolostatsscreen.cpp
    metsolowinscreen.cpp
    metsonglists.cpp
    metsonyscreen.cpp
    metstagefinishscreen.cpp
    metstrings.cpp
    mettoplogoscreen.cpp
    mettutorialscreen.cpp
    scrollinglist.cpp
    texturepairrecord.cpp)

add_library(resonance_met STATIC ${RESONANCE_MET_SRCS})
target_link_libraries(resonance_met PRIVATE resonance_common)
target_include_directories(resonance_met PRIVATE "${CMAKE_SOURCE_DIR}/src/python/PC"
                                                 "${CMAKE_SOURCE_DIR}/3rdparty/Python-2.0/Include")

# Patched builds show the commit and the build time on the title screen. The header is regenerated
# on every build.
if(ENABLE_PATCHES)
  set(_build_info "${CMAKE_BINARY_DIR}/generated/buildinfo.h")
  add_custom_target(
    resonance_build_info
    COMMAND "${CMAKE_COMMAND}" "-DSOURCE_DIR=${CMAKE_SOURCE_DIR}" "-DOUTPUT=${_build_info}" -P
            "${CMAKE_SOURCE_DIR}/cmake/build-info.cmake"
    BYPRODUCTS "${_build_info}"
    VERBATIM)
  add_dependencies(resonance_met resonance_build_info)
  target_include_directories(resonance_met PRIVATE "${CMAKE_BINARY_DIR}/generated")

  # The credits of patched builds open with this entry. The avatar is fetched once, when the
  # header is first generated. When it cannot be fetched, the entry shows the text alone.
  set(RESONANCE_CREDITS_GITHUB_USER
      "Tatsh"
      CACHE STRING "GitHub user whose avatar the first credit shows.")
  set(RESONANCE_CREDITS_TEXT
      "Reconstruction:\\nAndrew Udvare\\ngithub.com/Tatsh"
      CACHE STRING "Text of the first credit. \\n breaks a line.")
  find_program(RESONANCE_MAGICK magick)
  set(_credits_magick "${RESONANCE_MAGICK}")
  if(NOT RESONANCE_MAGICK)
    message(STATUS "ImageMagick was not found. The first credit has no avatar.")
    set(_credits_magick "")
  endif()
  set(_credits_avatar "${CMAKE_BINARY_DIR}/generated/creditsavatar.h")
  add_custom_command(
    OUTPUT "${_credits_avatar}"
    COMMAND
      "${RESONANCE_TOOL_CREDITS_AVATAR}" "${_credits_avatar}" --magick "${_credits_magick}" --user
      "${RESONANCE_CREDITS_GITHUB_USER}" --text "${RESONANCE_CREDITS_TEXT}"
    DEPENDS resonance_tools "${RESONANCE_TOOL_CREDITS_AVATAR}"
    COMMENT "Writing ${_credits_avatar}"
    VERBATIM)
  add_custom_target(resonance_credits_avatar DEPENDS "${_credits_avatar}")
  add_dependencies(resonance_met resonance_credits_avatar)
endif()
