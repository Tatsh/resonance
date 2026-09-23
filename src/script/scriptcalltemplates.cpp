#include "os/hxstr.h"
#include "script/scripthost.h"
#include "script/scripttemplatemap.h"

namespace {

// The identifiers RegisterScriptCallTemplates() registers, each named after its template text.
constexpr int kScriptTemplateExitInstanceException = 100;
constexpr int kScriptTemplateRunException = 101;
constexpr int kScriptTemplateGraphicsChannelError = 102;
constexpr int kScriptTemplateGetAutoexec = 200;
constexpr int kScriptTemplateSkipMetagame = 203;
constexpr int kScriptTemplateDefaultGameParams = 204;
constexpr int kScriptTemplateInitNewAlbum = 205;
constexpr int kScriptTemplateDoCheat = 206;
constexpr int kScriptTemplateTogglePowerupCheatsEnabled = 207;
constexpr int kScriptTemplateGetScreenConfig0 = 400;
constexpr int kScriptTemplateGetScreenConfig1 = 401;
constexpr int kScriptTemplateGetScreenConfig2 = 402;
constexpr int kScriptTemplateGetScreenConfig3 = 403;
constexpr int kScriptTemplateGetMidiDevice = 404;
constexpr int kScriptTemplateGetGlobalSynth = 405;
constexpr int kScriptTemplateShowSeerplayer = 406;
constexpr int kScriptTemplateConfigInput = 407;
constexpr int kScriptTemplateGetPs2MetagameHardBank = 500;
constexpr int kScriptTemplateGetPs2MetagameHardBankHeader = 501;
constexpr int kScriptTemplateLevelGetBeatnikBank = 502;
constexpr int kScriptTemplateLevelGetSeermusicBank = 503;
constexpr int kScriptTemplateLevelGetPs2HardBank = 504;
constexpr int kScriptTemplateLevelGetPs2HardBankHeader = 505;
constexpr int kScriptTemplateLevelGetPs2SoftBank = 506;
constexpr int kScriptTemplateGetEffectsBank = 507;
constexpr int kScriptTemplateGetEffectsBankHeader = 508;
constexpr int kScriptTemplateGetMetString = 600;
constexpr int kScriptTemplateGetMetStringList = 601;
constexpr int kScriptTemplateGetPersonaData = 602;
constexpr int kScriptTemplateLevelListIsAvailGame = 603;
constexpr int kScriptTemplateLevelListIsAvailJam = 604;
constexpr int kScriptTemplateLevelListStage = 605;
constexpr int kScriptTemplateLevelListOrder = 606;
constexpr int kScriptTemplateArenaListOrder = 607;
constexpr int kScriptTemplateResetCurrentLevel = 609;
constexpr int kScriptTemplateSetCommunity = 610;
constexpr int kScriptTemplateSetRuleset = 611;
constexpr int kScriptTemplateSetGemDifficulty = 612;
constexpr int kScriptTemplateLevelLocSplitscreen = 613;
constexpr int kScriptTemplateSetNumPlayers = 614;
constexpr int kScriptTemplateSetRemixPlayback = 615;
constexpr int kScriptTemplateGetHelpFST = 616;
constexpr int kScriptTemplateGetMetTitleString = 617;
constexpr int kScriptTemplateChooseRandomRecording = 618;
constexpr int kScriptTemplateUseAttractMode = 619;
constexpr int kScriptTemplateIdleSecsBeforeAttract = 620;
constexpr int kScriptTemplateGetLevels = 630;
constexpr int kScriptTemplateSetCurrentLevel = 631;
constexpr int kScriptTemplateLevelDirectoryName = 632;
constexpr int kScriptTemplateGetArenas = 634;
constexpr int kScriptTemplateArenaListMakeCurrent = 635;
constexpr int kScriptTemplateArenaDirectoryName = 636;
constexpr int kScriptTemplateGetStageBeatval = 637;
constexpr int kScriptTemplateLevelGetHorizon = 702;
constexpr int kScriptTemplateLevelListGetArtist = 800;
constexpr int kScriptTemplateLevelListGetGenre = 801;
constexpr int kScriptTemplateLevelListGetTempo = 802;
constexpr int kScriptTemplateLevelListGetDescription = 803;
constexpr int kScriptTemplateLevelListGetVersion = 804;
constexpr int kScriptTemplateLevelListGetName = 805;
constexpr int kScriptTemplateArenaListGetName = 806;
constexpr int kScriptTemplateLevelListGetAbbrevName = 807;
constexpr int kScriptTemplateLevelGetNumTracks = 900;
constexpr int kScriptTemplateLevelGetNumLaps = 901;
constexpr int kScriptTemplateLevelGetBackingTrackCriteria = 902;
constexpr int kScriptTemplateLevelGetTrackEnablementCriteria = 903;
constexpr int kScriptTemplateLevelGetPowerbar = 904;
constexpr int kScriptTemplateLevelGetMfdPowerups = 905;
constexpr int kScriptTemplateLevelGetGemDifficulty = 906;
constexpr int kScriptTemplateLevelGetCatchPoints = 907;
constexpr int kScriptTemplateLevelGetInitialPoints = 908;
constexpr int kScriptTemplateLevelGetIntroLength = 909;
constexpr int kScriptTemplateLevelGetMidiFile = 910;
constexpr int kScriptTemplateLevelFxFiltPeriod = 911;
constexpr int kScriptTemplateLevelFxFiltMax = 912;
constexpr int kScriptTemplateLevelFxStt = 913;
constexpr int kScriptTemplateLevelFxVolume = 914;
constexpr int kScriptTemplateLevelGetAxeFxCtrl = 915;
constexpr int kScriptTemplateLevelGetMaxPoints = 916;
constexpr int kScriptTemplateLevelGetFreestylePoints = 917;
constexpr int kScriptTemplateLevelGetSectionBoundaries = 918;
constexpr int kScriptTemplateShowTimers = 919;
constexpr int kScriptTemplateLevelWackoPanning = 920;
constexpr int kScriptTemplateLevelBoostVolume = 921;
constexpr int kScriptTemplateLevelGetJam16ths = 922;
constexpr int kScriptTemplateLevelEnableAll = 923;
constexpr int kScriptTemplateLevelGetSlop = 924;
constexpr int kScriptTemplateLevelGetSectionInstances = 925;
constexpr int kScriptTemplateLevelUseLinearSongForm = 926;
constexpr int kScriptTemplateLevelTrackAttenuations = 927;
constexpr int kScriptTemplateLevelSectionNames = 928;
constexpr int kScriptTemplateLevelGetIsTutorial = 929;
constexpr int kScriptTemplateShowStats = 930;
constexpr int kScriptTemplateLevelGetLinkedSections = 931;
constexpr int kScriptTemplateLevelUseBankSwapping = 932;
constexpr int kScriptTemplateLevelGetSoundbankMovie = 933;
constexpr int kScriptTemplateLevelGetStartingTrack = 934;
constexpr int kScriptTemplatePointsPerPeriod = 935;
constexpr int kScriptTemplateScoreBoundaries = 936;
constexpr int kScriptTemplateHandlerOnGameBegin = 1000;
constexpr int kScriptTemplateHandlerOnGameOver = 1001;
constexpr int kScriptTemplateHandlerOnHit = 1002;
constexpr int kScriptTemplateHandlerOnMiss = 1003;
constexpr int kScriptTemplateHandlerOnInvalidPitch = 1004;
constexpr int kScriptTemplateHandlerOnPhraseCapture = 1005;
constexpr int kScriptTemplateHandlerOnScoreTooLow = 1006;
constexpr int kScriptTemplateHandlerOnTrackSelect = 1007;
constexpr int kScriptTemplateOnKeyDown = 1008;
constexpr int kScriptTemplateHandlerOnKeyDown = 1009;
constexpr int kScriptTemplateOnDebugPadDown = 1010;
constexpr int kScriptTemplateHandlerOnToggleLoop = 1011;
constexpr int kScriptTemplateHandlerOnErase = 1012;
constexpr int kScriptTemplateHandlerOnAdvanceSection = 1013;
constexpr int kScriptTemplateHandlerOnAxeButton = 1014;
constexpr int kScriptTemplateHandlerOnAxeStick = 1015;
constexpr int kScriptTemplateHandlerOnChoosePowerup = 1016;
constexpr int kScriptTemplateHandlerOnJamEffect = 1017;
constexpr int kScriptTemplateHandlerOnMidiNote = 1018;
constexpr int kScriptTemplateHandlerOnHealth = 1019;
constexpr int kScriptTemplateHandlerOnPitch = 1020;
constexpr int kScriptTemplateHandlerOnToggleGhosts = 1021;
constexpr int kScriptTemplateGetFbEnabled = 1200;
constexpr int kScriptTemplateGetMetroFbInfo = 1201;
constexpr int kScriptTemplateGetCrippledFbInfo = 1202;
constexpr int kScriptTemplateGetNeutralizedFbInfo = 1203;
constexpr int kScriptTemplateGetAutocatchFbInfo = 1204;
constexpr int kScriptTemplateGetBumpFbInfo = 1205;
constexpr int kScriptTemplateGetCaughtPowFbInfo = 1206;
constexpr int kScriptTemplateGetKickFbInfo = 1207;
constexpr int kScriptTemplateGetAlbumNum = 1300;
constexpr int kScriptTemplateGetAlbumName = 1301;

} // namespace

// 0x004016c8
void RegisterScriptCallTemplates() {
    RegisterScriptTemplate(
        kScriptTemplateExitInstanceException,
        HxStr("An exception was thrown by the function Application::ExitInstance().\n\n%s"));
    RegisterScriptTemplate(
        kScriptTemplateRunException,
        HxStr("An exception was thrown by the function Application::Run().\n\n%s"));
    RegisterScriptTemplate(
        kScriptTemplateGraphicsChannelError,
        HxStr("An error occurred while attempting to realize the graphics channel."));
    RegisterScriptTemplate(kScriptTemplateAutoexec, HxStr("autoexec()"));
    RegisterScriptTemplate(kScriptTemplateGetAutoexec, HxStr("get_autoexec()"));
    RegisterScriptTemplate(kScriptTemplateSkipMetagame, HxStr("skip_metagame()"));
    RegisterScriptTemplate(kScriptTemplateDefaultGameParams, HxStr("default_game_params"));
    RegisterScriptTemplate(kScriptTemplateInitNewAlbum, HxStr("init_new_album()"));
    RegisterScriptTemplate(kScriptTemplateDoCheat, HxStr("do_cheat('%s', %d)"));
    RegisterScriptTemplate(kScriptTemplateTogglePowerupCheatsEnabled,
                           HxStr("toggle_powerup_cheats_enabled()"));
    RegisterScriptTemplate(kScriptTemplateGetScreenConfig0, HxStr("get_screen_config()[0]"));
    RegisterScriptTemplate(kScriptTemplateGetScreenConfig1, HxStr("get_screen_config()[1]"));
    RegisterScriptTemplate(kScriptTemplateGetScreenConfig2, HxStr("get_screen_config()[2]"));
    RegisterScriptTemplate(kScriptTemplateGetScreenConfig3, HxStr("get_screen_config()[3]"));
    RegisterScriptTemplate(kScriptTemplateGetMidiDevice, HxStr("get_midi_device()"));
    RegisterScriptTemplate(kScriptTemplateGetGlobalSynth, HxStr("get_global_synth()"));
    RegisterScriptTemplate(kScriptTemplateShowSeerplayer, HxStr("show_seerplayer()"));
    RegisterScriptTemplate(kScriptTemplateConfigInput, HxStr("config_input()"));
    RegisterScriptTemplate(kScriptTemplateGetPs2MetagameHardBank,
                           HxStr("get_ps2_metagame_hard_bank(%d)"));
    RegisterScriptTemplate(kScriptTemplateGetPs2MetagameHardBankHeader,
                           HxStr("get_ps2_metagame_hard_bank_header(%d)"));
    RegisterScriptTemplate(kScriptTemplateLevelGetPs2HardBank,
                           HxStr("current_level.get_ps2_hard_bank(%d,%d)"));
    RegisterScriptTemplate(kScriptTemplateLevelGetPs2HardBankHeader,
                           HxStr("current_level.get_ps2_hard_bank_header(%d,%d)"));
    RegisterScriptTemplate(kScriptTemplateLevelGetPs2SoftBank,
                           HxStr("current_level.get_ps2_soft_bank(%d)"));
    RegisterScriptTemplate(kScriptTemplateLevelGetBeatnikBank,
                           HxStr("current_level.get_beatnik_bank()"));
    RegisterScriptTemplate(kScriptTemplateLevelGetSeermusicBank,
                           HxStr("current_level.get_seermusic_bank()"));
    RegisterScriptTemplate(kScriptTemplateGetEffectsBank, HxStr("get_effects_bank()"));
    RegisterScriptTemplate(kScriptTemplateGetEffectsBankHeader, HxStr("get_effects_bank_header()"));
    RegisterScriptTemplate(kScriptTemplateGetMetString, HxStr("get_met_string('%s')"));
    RegisterScriptTemplate(kScriptTemplateGetMetStringList, HxStr("get_met_stringList('%s')"));
    RegisterScriptTemplate(kScriptTemplateGetPersonaData, HxStr("get_persona_data('%s')"));
    RegisterScriptTemplate(kScriptTemplateLevelListIsAvailGame,
                           HxStr("level_list['%s'].is_avail_game()"));
    RegisterScriptTemplate(kScriptTemplateLevelListIsAvailJam,
                           HxStr("level_list['%s'].is_avail_jam()"));
    RegisterScriptTemplate(kScriptTemplateLevelListStage, HxStr("level_list['%s'].stage"));
    RegisterScriptTemplate(kScriptTemplateLevelListOrder, HxStr("level_list['%s'].order"));
    RegisterScriptTemplate(kScriptTemplateArenaListOrder, HxStr("arena_list['%s'].order"));
    RegisterScriptTemplate(kScriptTemplateGetLevels, HxStr("get_levels()"));
    RegisterScriptTemplate(kScriptTemplateGetArenas, HxStr("get_arenas('%s')"));
    RegisterScriptTemplate(kScriptTemplateSetCurrentLevel, HxStr("set_current_level('%s')"));
    RegisterScriptTemplate(kScriptTemplateArenaListMakeCurrent,
                           HxStr("arena_list['%s'].make_current()"));
    RegisterScriptTemplate(kScriptTemplateLevelDirectoryName,
                           HxStr("current_level.directory_name"));
    RegisterScriptTemplate(kScriptTemplateArenaDirectoryName,
                           HxStr("current_arena.directory_name"));
    RegisterScriptTemplate(kScriptTemplateResetCurrentLevel, HxStr("reset_current_level()"));
    RegisterScriptTemplate(kScriptTemplateSetCommunity, HxStr("set_community ('%s')"));
    RegisterScriptTemplate(kScriptTemplateSetRuleset, HxStr("set_ruleset ('%s')"));
    RegisterScriptTemplate(kScriptTemplateSetRemixPlayback, HxStr("set_remix_playback (%s)"));
    RegisterScriptTemplate(kScriptTemplateSetGemDifficulty, HxStr("set_gem_difficulty(%d)"));
    RegisterScriptTemplate(kScriptTemplateLevelLocSplitscreen,
                           HxStr("current_level.loc_splitscreen"));
    RegisterScriptTemplate(kScriptTemplateSetNumPlayers, HxStr("set_num_players(%d)"));
    RegisterScriptTemplate(kScriptTemplateGetHelpFST, HxStr("get_help_FST('%s')"));
    RegisterScriptTemplate(kScriptTemplateGetMetTitleString, HxStr("get_met_title_string('%s')"));
    RegisterScriptTemplate(kScriptTemplateChooseRandomRecording,
                           HxStr("choose_random_recording()"));
    RegisterScriptTemplate(kScriptTemplateUseAttractMode, HxStr("use_attract_mode()"));
    RegisterScriptTemplate(kScriptTemplateIdleSecsBeforeAttract,
                           HxStr("idle_secs_before_attract()"));
    RegisterScriptTemplate(kScriptTemplateGetStageBeatval, HxStr("get_stage_beatval(%d, %d)"));
    RegisterScriptTemplate(kScriptTemplateLevelGetHorizon, HxStr("current_level.get_horizon()"));
    RegisterScriptTemplate(kScriptTemplateLevelListGetArtist,
                           HxStr("level_list['%s'].get_artist()"));
    RegisterScriptTemplate(kScriptTemplateLevelListGetGenre, HxStr("level_list['%s'].get_genre()"));
    RegisterScriptTemplate(kScriptTemplateLevelListGetTempo, HxStr("level_list['%s'].get_tempo()"));
    RegisterScriptTemplate(kScriptTemplateLevelListGetDescription,
                           HxStr("level_list['%s'].get_description()"));
    RegisterScriptTemplate(kScriptTemplateLevelListGetVersion,
                           HxStr("level_list['%s'].get_version('%s')"));
    RegisterScriptTemplate(kScriptTemplateLevelListGetName, HxStr("level_list['%s'].get_name()"));
    RegisterScriptTemplate(kScriptTemplateLevelListGetAbbrevName,
                           HxStr("level_list['%s'].get_abbrev_name()"));
    RegisterScriptTemplate(kScriptTemplateArenaListGetName, HxStr("arena_list['%s'].get_name()"));
    RegisterScriptTemplate(kScriptTemplateLevelGetNumTracks,
                           HxStr("current_level.get_num_tracks()"));
    RegisterScriptTemplate(kScriptTemplateLevelGetNumLaps, HxStr("current_level.get_num_laps()"));
    RegisterScriptTemplate(kScriptTemplateLevelGetBackingTrackCriteria,
                           HxStr("current_level.get_backing_track_criteria(%d)"));
    RegisterScriptTemplate(kScriptTemplateLevelGetTrackEnablementCriteria,
                           HxStr("current_level.get_track_enablement_criteria(%d)"));
    RegisterScriptTemplate(kScriptTemplateLevelGetPowerbar,
                           HxStr("current_level.get_powerbar(%d,%d)"));
    RegisterScriptTemplate(kScriptTemplateLevelGetMfdPowerups,
                           HxStr("current_level.get_mfd_powerups()"));
    RegisterScriptTemplate(kScriptTemplateLevelGetGemDifficulty,
                           HxStr("current_level.get_gem_difficulty()"));
    RegisterScriptTemplate(kScriptTemplateLevelGetCatchPoints,
                           HxStr("current_level.get_catch_points()"));
    RegisterScriptTemplate(kScriptTemplateLevelGetInitialPoints,
                           HxStr("current_level.get_initial_points()"));
    RegisterScriptTemplate(kScriptTemplateLevelGetIntroLength,
                           HxStr("current_level.get_intro_length()"));
    RegisterScriptTemplate(kScriptTemplateLevelGetMidiFile, HxStr("current_level.get_midi_file()"));
    RegisterScriptTemplate(kScriptTemplateLevelFxFiltPeriod,
                           HxStr("current_level.fx_filt_period[%d]"));
    RegisterScriptTemplate(kScriptTemplateLevelFxFiltMax, HxStr("current_level.fx_filt_max[%d]"));
    RegisterScriptTemplate(kScriptTemplateLevelFxStt, HxStr("current_level.fx_stt"));
    RegisterScriptTemplate(kScriptTemplateLevelFxVolume, HxStr("current_level.fx_volume"));
    RegisterScriptTemplate(kScriptTemplateLevelGetAxeFxCtrl,
                           HxStr("current_level.get_axe_fx_ctrl()"));
    RegisterScriptTemplate(kScriptTemplateLevelGetMaxPoints,
                           HxStr("current_level.get_max_points()"));
    RegisterScriptTemplate(kScriptTemplateLevelGetFreestylePoints,
                           HxStr("current_level.get_freestyle_points()"));
    RegisterScriptTemplate(kScriptTemplateLevelGetSectionBoundaries,
                           HxStr("current_level.get_section_boundaries()"));
    RegisterScriptTemplate(kScriptTemplateShowTimers, HxStr("show_timers()"));
    RegisterScriptTemplate(kScriptTemplateShowStats, HxStr("show_stats()"));
    RegisterScriptTemplate(kScriptTemplateLevelWackoPanning,
                           HxStr("current_level.wacko_panning()"));
    RegisterScriptTemplate(kScriptTemplateLevelBoostVolume, HxStr("current_level.boost_volume()"));
    RegisterScriptTemplate(kScriptTemplateLevelGetJam16ths, HxStr("current_level.get_jam_16ths()"));
    RegisterScriptTemplate(kScriptTemplateLevelEnableAll, HxStr("current_level.enable_all"));
    RegisterScriptTemplate(kScriptTemplateLevelGetSlop, HxStr("current_level.get_slop()"));
    RegisterScriptTemplate(kScriptTemplateLevelGetSectionInstances,
                           HxStr("current_level.get_section_instances()"));
    RegisterScriptTemplate(kScriptTemplateLevelUseLinearSongForm,
                           HxStr("current_level.use_linear_song_form()"));
    RegisterScriptTemplate(kScriptTemplateLevelTrackAttenuations,
                           HxStr("current_level.track_attenuations"));
    RegisterScriptTemplate(kScriptTemplateLevelSectionNames, HxStr("current_level.section_names"));
    RegisterScriptTemplate(kScriptTemplateLevelGetIsTutorial,
                           HxStr("current_level.get_is_tutorial()"));
    RegisterScriptTemplate(kScriptTemplateLevelGetLinkedSections,
                           HxStr("current_level.get_linked_sections(%d)"));
    RegisterScriptTemplate(kScriptTemplateLevelUseBankSwapping,
                           HxStr("current_level.use_bank_swapping()"));
    RegisterScriptTemplate(kScriptTemplateLevelGetSoundbankMovie,
                           HxStr("current_level.get_soundbank_movie()"));
    RegisterScriptTemplate(kScriptTemplateLevelGetStartingTrack,
                           HxStr("current_level.get_starting_track()"));
    RegisterScriptTemplate(kScriptTemplatePointsPerPeriod, HxStr("points_per_period"));
    RegisterScriptTemplate(kScriptTemplateScoreBoundaries, HxStr("score_boundaries"));
    RegisterScriptTemplate(kScriptTemplateHandlerOnGameBegin,
                           HxStr("call_handler('on_game_begin')"));
    RegisterScriptTemplate(kScriptTemplateHandlerOnGameOver, HxStr("call_handler('on_game_over')"));
    RegisterScriptTemplate(kScriptTemplateHandlerOnHit, HxStr("call_handler('on_hit')"));
    RegisterScriptTemplate(kScriptTemplateHandlerOnMiss, HxStr("call_handler('on_miss')"));
    RegisterScriptTemplate(kScriptTemplateHandlerOnInvalidPitch,
                           HxStr("call_handler('on_invalid_pitch')"));
    RegisterScriptTemplate(kScriptTemplateHandlerOnPhraseCapture,
                           HxStr("call_handler('on_phrase_capture',%u)"));
    RegisterScriptTemplate(kScriptTemplateHandlerOnScoreTooLow,
                           HxStr("call_handler('on_score_too_low',%u)"));
    RegisterScriptTemplate(kScriptTemplateHandlerOnTrackSelect,
                           HxStr("call_handler('on_track_select',%u)"));
    RegisterScriptTemplate(kScriptTemplateOnKeyDown, HxStr("on_key_down(%u)"));
    RegisterScriptTemplate(kScriptTemplateHandlerOnKeyDown,
                           HxStr("call_handler ('on_key_down', %d)"));
    RegisterScriptTemplate(kScriptTemplateOnDebugPadDown, HxStr("on_debug_pad_down(%u)"));
    RegisterScriptTemplate(kScriptTemplateHandlerOnToggleLoop,
                           HxStr("call_handler('on_toggle_loop')"));
    RegisterScriptTemplate(kScriptTemplateHandlerOnErase, HxStr("call_handler('on_erase')"));
    RegisterScriptTemplate(kScriptTemplateHandlerOnAdvanceSection,
                           HxStr("call_handler('on_advance_section')"));
    RegisterScriptTemplate(kScriptTemplateHandlerOnAxeButton,
                           HxStr("call_handler('on_axe_button')"));
    RegisterScriptTemplate(kScriptTemplateHandlerOnAxeStick, HxStr("call_handler('on_axe_stick')"));
    RegisterScriptTemplate(kScriptTemplateHandlerOnChoosePowerup,
                           HxStr("call_handler('on_choose_powerup', %d)"));
    RegisterScriptTemplate(kScriptTemplateHandlerOnJamEffect,
                           HxStr("call_handler('on_jam_effect')"));
    RegisterScriptTemplate(kScriptTemplateHandlerOnMidiNote,
                           HxStr("call_handler('on_midi_note', %d, %d)"));
    RegisterScriptTemplate(kScriptTemplateHandlerOnHealth, HxStr("call_handler('on_health', %d)"));
    RegisterScriptTemplate(kScriptTemplateHandlerOnPitch, HxStr("call_handler('on_pitch')"));
    RegisterScriptTemplate(kScriptTemplateHandlerOnToggleGhosts,
                           HxStr("call_handler('on_toggle_ghosts')"));
    RegisterScriptTemplate(kScriptTemplateGetFbEnabled, HxStr("get_fb_enabled()"));
    RegisterScriptTemplate(kScriptTemplateGetMetroFbInfo, HxStr("get_metro_fb_info()"));
    RegisterScriptTemplate(kScriptTemplateGetKickFbInfo, HxStr("get_kick_fb_info()"));
    RegisterScriptTemplate(kScriptTemplateGetCrippledFbInfo, HxStr("get_crippled_fb_info()"));
    RegisterScriptTemplate(kScriptTemplateGetNeutralizedFbInfo, HxStr("get_neutralized_fb_info()"));
    RegisterScriptTemplate(kScriptTemplateGetAutocatchFbInfo, HxStr("get_autocatch_fb_info()"));
    RegisterScriptTemplate(kScriptTemplateGetBumpFbInfo, HxStr("get_bump_fb_info()"));
    RegisterScriptTemplate(kScriptTemplateGetCaughtPowFbInfo, HxStr("get_caught_pow_fb_info()"));
    RegisterScriptTemplate(kScriptTemplateGetAlbumNum, HxStr("get_album_num()"));
    RegisterScriptTemplate(kScriptTemplateGetAlbumName, HxStr("get_album_name()"));
}
