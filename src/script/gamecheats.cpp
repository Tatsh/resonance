#include <exception>

#include "app/application.h"
#include "app/globals.h"
#include "app/playsound.h"
#include "game/enablemgr.h"
#include "game/gamemanagerimpl.h"
#include "game/gameparams.h"
#include "game/gamer.h"
#include "game/globalsettings.h"
#include "game/grooveworld.h"
#include "game/metagameworld.h"
#include "met/albumcache.h"
#include "met/gameoptions.h"
#include "met/metrenderer.h"
#include "mid/mbt.h"
#include "os/formatstring.h"
#include "os/hostmode.h"
#include "os/hxstr.h"
#include "rnd/filepath.h"
#include "rnd/manager.h"
#include "script/cxx/config.h"
#include "script/cxx/int.h"
#include "script/cxx/object.h"
#include "script/cxx/tuple.h"
#include "script/cxx/typeerror.h"
#include "script/scriptfunc.h"
#include "script/scripthost.h"

namespace {

// Tracks the enable-all-tracks cheat frees, one SetFreeUntil() each.
constexpr int kCheatTrackCount = 8;

// Template the powerup cheat runs.
constexpr int kPowerupCheatTemplate = 207;

// Add juice to the gamer.
//
// The error names the enable_freestyle command, whose check this repeats.
// NTSC-U/C: 0x00147e40, PAL: 0x00148a10
Py::Object ScriptAddJuice(const Py::Tuple &args) {
    if (args.length() != 1) {
        throw Py::TypeError(HxStr("wrong # args for enable_freestyle"));
    }
    const int nAmount = Py::Int(args.getItem(0));
    Application::shared()->GetWorld()->mGamer->AddJuice(static_cast<int>(nAmount));
    return Py::Object();
}

// Move the gamer to a song position.
// NTSC-U/C: 0x00148540, PAL: 0x00149130
Py::Object ScriptAdvanceSection(const Py::Tuple &args) {
    if (args.length() != 1) {
        throw Py::TypeError(HxStr("wrong # args for advance_section"));
    }
    const int nPosition = Py::Int(args.getItem(0));
    Mid::MBT position(static_cast<int>(nPosition));
    Application::shared()->GetWorld()->mGamer->AdvanceAt(position);
    return Py::Object();
}

// End the game with a score.
// NTSC-U/C: 0x0014a200, PAL: 0x0014ae10
Py::Object ScriptWinWithPointsCheat(const Py::Tuple &args) {
    PlayActivateSound();
    if (args.length() != 1) {
        throw Py::TypeError(HxStr("wrong # args for do_win_with_points_cheat"));
    }
    const int nScore = Py::Int(args.getItem(0));
    Application::shared()->GetWorld()->mGamer->EndWithScore(static_cast<int>(nScore));
    return Py::Object();
}

// Free every track of its requirements.
//
// Each track stays free until bar -1, a bar that never arrives. Jam sessions retain their
// requirements.
// NTSC-U/C: 0x0014a6d8, PAL: 0x0014b308
Py::Object ScriptEnableAllTracksCheat([[maybe_unused]] const Py::Tuple &args) {
    PlayActivateSound();
    GrooveWorld *pWorld = Application::shared()->GetWorld();
    if (pWorld != nullptr && Application::shared()->GetPlayMode() != kPlayModeJam) {
        if (pWorld->mGamer->mEnableMgr != nullptr) {
            for (int nTrack = 0; nTrack < kCheatTrackCount; ++nTrack) {
                pWorld->mGamer->mEnableMgr->SetFreeUntil(nTrack, 0, -1);
            }
            pWorld->mGamer->mCheated = 1;
        }
    }
    return Py::Object();
}

// Enter listen mode.
//
// Ends the game scoreless and arms the cheat flag, everywhere but in a jam session.
// NTSC-U/C: 0x00148c68, PAL: 0x00149878
Py::Object ScriptActivateListenMode([[maybe_unused]] const Py::Tuple &args) {
    PlayActivateSound();
    GrooveWorld *pWorld = Application::shared()->GetWorld();
    if (pWorld != nullptr && Application::shared()->GetPlayMode() != kPlayModeJam) {
        pWorld->mGamer->EndWithScore(0);
        pWorld->mGamer->mCheated = 1;
    }
    return Py::Object();
}

// Save the live scene to tunnel/livegame.rnd.
// NTSC-U/C: 0x00146590, PAL: 0x001470a8
Py::Object ScriptSaveRnd([[maybe_unused]] const Py::Tuple &args) {
    Rnd::FilePath::SetRoot(MakeFreqPath(HxStr("tunnel")));
    Rnd::g_manager.SaveFile(MakeFreqPath(HxStr("tunnel/livegame.rnd")));
    return Py::Object();
}

// Empty the album caches.
//
// The binary expands the three steps of ClearAlbumCache() inline; calling it repeats them
// without duplicating the body. met/albumcache.h records the expansion.
// NTSC-U/C: 0x003f6860, PAL: 0x0042f070
Py::Object ScriptEmptyAlbumCaches([[maybe_unused]] Py::Tuple args) {
    ClearAlbumCache();
    return Py::Object();
}

// Enter practice mode.
// NTSC-U/C: 0x0014e848, PAL: 0x0014f510
// PyInvokeActivatePracticeMode() expands this inline, and the out-of-line copy has no caller.
inline Py::Object ScriptActivatePracticeMode([[maybe_unused]] const Py::Tuple &args) {
    PlayActivateSound();
    GrooveWorld *pWorld = Application::shared()->GetWorld();
    if (pWorld != nullptr) {
        pWorld->mGamer->mJuiceFrozen = 1;
        pWorld->mGamer->mCheated = 1;
    }
    return Py::Object();
}

// Run ScriptActivatePracticeMode() on the interpreter's argument tuple.
// NTSC-U/C: 0x00148f98, PAL: 0x00149ba8
PyObject *PyInvokeActivatePracticeMode(PyObject *, PyObject *pArgs) {
    try {
        Py::Tuple args(pArgs);
        Py::Object result = ScriptActivatePracticeMode(args);
        return Py::new_reference_to(result);
    } catch (Py::Exception &) {
        return nullptr;
    } catch (std::exception &error) {
        PyErr_SetString(PyExc_RuntimeError, const_cast<char *>(error.what()));
        return nullptr;
    }
}

// Unlock every stage.
// NTSC-U/C: 0x0014e8d0, PAL: 0x0014f598
// PyInvokeActivateAllAccessMode() expands this inline, and the out-of-line copy has no caller.
inline Py::Object ScriptActivateAllAccessMode([[maybe_unused]] const Py::Tuple &args) {
    MetRenderer *pRenderer =
        dynamic_cast<MetRenderer *>(Application::shared()->GetMetaWorld()->GetRenderer());
    if (pRenderer != nullptr) {
        pRenderer->UnlockAllStages();
    }
    return Py::Object();
}

// Run ScriptActivateAllAccessMode() on the interpreter's argument tuple.
// NTSC-U/C: 0x00149230, PAL: 0x00149e40
PyObject *PyInvokeActivateAllAccessMode(PyObject *, PyObject *pArgs) {
    try {
        Py::Tuple args(pArgs);
        Py::Object result = ScriptActivateAllAccessMode(args);
        return Py::new_reference_to(result);
    } catch (Py::Exception &) {
        return nullptr;
    } catch (std::exception &error) {
        PyErr_SetString(PyExc_RuntimeError, const_cast<char *>(error.what()));
        return nullptr;
    }
}

// Offer the team identities.
//
// Only from the logo screen, where the cheat is entered.
// NTSC-U/C: 0x0014e980, PAL: 0x0014f648
// PyInvokeEnableTeamFreqs() expands this inline, and the out-of-line copy has no caller.
inline Py::Object ScriptEnableTeamFreqs([[maybe_unused]] const Py::Tuple &args) {
    MetRenderer *pRenderer =
        dynamic_cast<MetRenderer *>(Application::shared()->GetMetaWorld()->GetRenderer());
    if (pRenderer->IsLogoScreenActive() != 0) {
        PlayActivateSound();
        GlobalSettings::shared()->mTeamFreqUnlocked = 1;
    }
    return Py::Object();
}

// Run ScriptEnableTeamFreqs() on the interpreter's argument tuple.
// NTSC-U/C: 0x001494f8, PAL: 0x0014a108
PyObject *PyInvokeEnableTeamFreqs(PyObject *, PyObject *pArgs) {
    try {
        Py::Tuple args(pArgs);
        Py::Object result = ScriptEnableTeamFreqs(args);
        return Py::new_reference_to(result);
    } catch (Py::Exception &) {
        return nullptr;
    } catch (std::exception &error) {
        PyErr_SetString(PyExc_RuntimeError, const_cast<char *>(error.what()));
        return nullptr;
    }
}

// Grant a powerup through its script template.
//
// Only from the logo screen, where the cheat is entered.
// NTSC-U/C: 0x0014ea48, PAL: 0x0014f710
// PyInvokeEnablePowerupCheats() expands this inline, and the out-of-line copy has no caller.
inline Py::Object ScriptEnablePowerupCheats([[maybe_unused]] const Py::Tuple &args) {
    MetRenderer *pRenderer =
        dynamic_cast<MetRenderer *>(Application::shared()->GetMetaWorld()->GetRenderer());
    if (pRenderer->IsLogoScreenActive() != 0) {
        PlayActivateSound();
        CallScriptTemplate(kPowerupCheatTemplate);
    }
    return Py::Object();
}

// Run ScriptEnablePowerupCheats() on the interpreter's argument tuple.
// NTSC-U/C: 0x001497d8, PAL: 0x0014a3e8
PyObject *PyInvokeEnablePowerupCheats(PyObject *, PyObject *pArgs) {
    try {
        Py::Tuple args(pArgs);
        Py::Object result = ScriptEnablePowerupCheats(args);
        return Py::new_reference_to(result);
    } catch (Py::Exception &) {
        return nullptr;
    } catch (std::exception &error) {
        PyErr_SetString(PyExc_RuntimeError, const_cast<char *>(error.what()));
        return nullptr;
    }
}

// Play the powerup cheat sound.
// NTSC-U/C: 0x0014eb08, PAL: 0x0014f7d0
// PyInvokeDoPowerupCheat() expands this inline, and the out-of-line copy has no caller.
inline Py::Object ScriptDoPowerupCheat([[maybe_unused]] const Py::Tuple &args) {
    PlayActivateSound();
    return Py::Object();
}

// Run ScriptDoPowerupCheat() on the interpreter's argument tuple.
// NTSC-U/C: 0x00149ab0, PAL: 0x0014a6c0
PyObject *PyInvokeDoPowerupCheat(PyObject *, PyObject *pArgs) {
    try {
        Py::Tuple args(pArgs);
        Py::Object result = ScriptDoPowerupCheat(args);
        return Py::new_reference_to(result);
    } catch (Py::Exception &) {
        return nullptr;
    } catch (std::exception &error) {
        PyErr_SetString(PyExc_RuntimeError, const_cast<char *>(error.what()));
        return nullptr;
    }
}

// Play the big-gem cheat sound.
// NTSC-U/C: 0x0014eb60, PAL: 0x0014f828
// PyInvokeDoBigGemModeCheat() expands this inline, and the out-of-line copy has no caller.
inline Py::Object ScriptDoBigGemModeCheat([[maybe_unused]] const Py::Tuple &args) {
    PlayActivateSound();
    return Py::Object();
}

// Run ScriptDoBigGemModeCheat() on the interpreter's argument tuple.
// NTSC-U/C: 0x00149d20, PAL: 0x0014a930
PyObject *PyInvokeDoBigGemModeCheat(PyObject *, PyObject *pArgs) {
    try {
        Py::Tuple args(pArgs);
        Py::Object result = ScriptDoBigGemModeCheat(args);
        return Py::new_reference_to(result);
    } catch (Py::Exception &) {
        return nullptr;
    } catch (std::exception &error) {
        PyErr_SetString(PyExc_RuntimeError, const_cast<char *>(error.what()));
        return nullptr;
    }
}

// Play the no-lattice cheat sound.
// NTSC-U/C: 0x0014ebb8, PAL: 0x0014f880
// PyInvokeDoNoLatticeModeCheat() expands this inline, and the out-of-line copy has no caller.
inline Py::Object ScriptDoNoLatticeModeCheat([[maybe_unused]] const Py::Tuple &args) {
    PlayActivateSound();
    return Py::Object();
}

// Run ScriptDoNoLatticeModeCheat() on the interpreter's argument tuple.
// NTSC-U/C: 0x00149f90, PAL: 0x0014aba0
PyObject *PyInvokeDoNoLatticeModeCheat(PyObject *, PyObject *pArgs) {
    try {
        Py::Tuple args(pArgs);
        Py::Object result = ScriptDoNoLatticeModeCheat(args);
        return Py::new_reference_to(result);
    } catch (Py::Exception &) {
        return nullptr;
    } catch (std::exception &error) {
        PyErr_SetString(PyExc_RuntimeError, const_cast<char *>(error.what()));
        return nullptr;
    }
}

// Play the arena-cycle cheat sound.
// NTSC-U/C: 0x0014ec60, PAL: 0x0014f928
// PyInvokeDoArenaStateCycleCheat() expands this inline, and the out-of-line copy has no caller.
inline Py::Object ScriptDoArenaStateCycleCheat([[maybe_unused]] const Py::Tuple &args) {
    PlayActivateSound();
    return Py::Object();
}

// Run ScriptDoArenaStateCycleCheat() on the interpreter's argument tuple.
// NTSC-U/C: 0x0014aa58, PAL: 0x0014b688
PyObject *PyInvokeDoArenaStateCycleCheat(PyObject *, PyObject *pArgs) {
    try {
        Py::Tuple args(pArgs);
        Py::Object result = ScriptDoArenaStateCycleCheat(args);
        return Py::new_reference_to(result);
    } catch (Py::Exception &) {
        return nullptr;
    } catch (std::exception &error) {
        PyErr_SetString(PyExc_RuntimeError, const_cast<char *>(error.what()));
        return nullptr;
    }
}

// Flip the expansion-pack flag.
//
// Only from the logo screen, where the cheat is entered.
// NTSC-U/C: 0x0014ecb8, PAL: 0x0014f980
// PyInvokeDoExpansionPackToggleCheat() expands this inline, and the out-of-line copy has no
// caller.
inline Py::Object ScriptDoExpansionPackToggleCheat([[maybe_unused]] const Py::Tuple &args) {
    MetRenderer *pRenderer =
        dynamic_cast<MetRenderer *>(Application::shared()->GetMetaWorld()->GetRenderer());
    if (pRenderer->IsLogoScreenActive() != 0) {
        PlayActivateSound();
        GlobalSettings::shared()->mGameOptions.mExpansionPack ^= 1;
    }
    return Py::Object();
}

// Run ScriptDoExpansionPackToggleCheat() on the interpreter's argument tuple.
// NTSC-U/C: 0x0014acc8, PAL: 0x0014b8f8
PyObject *PyInvokeDoExpansionPackToggleCheat(PyObject *, PyObject *pArgs) {
    try {
        Py::Tuple args(pArgs);
        Py::Object result = ScriptDoExpansionPackToggleCheat(args);
        return Py::new_reference_to(result);
    } catch (Py::Exception &) {
        return nullptr;
    } catch (std::exception &error) {
        PyErr_SetString(PyExc_RuntimeError, const_cast<char *>(error.what()));
        return nullptr;
    }
}

// Flag the win sequence to run.
// NTSC-U/C: 0x0014ed88, PAL: 0x0014fa50
// PyInvokeDoWinSequenceCheat() expands this inline, and the out-of-line copy has no caller.
inline Py::Object ScriptDoWinSequenceCheat([[maybe_unused]] const Py::Tuple &args) {
    SetDoWinSequence(1);
    return Py::Object();
}

// Run ScriptDoWinSequenceCheat() on the interpreter's argument tuple.
// NTSC-U/C: 0x0014afa8, PAL: 0x0014bbd8
PyObject *PyInvokeDoWinSequenceCheat(PyObject *, PyObject *pArgs) {
    try {
        Py::Tuple args(pArgs);
        Py::Object result = ScriptDoWinSequenceCheat(args);
        return Py::new_reference_to(result);
    } catch (Py::Exception &) {
        return nullptr;
    } catch (std::exception &error) {
        PyErr_SetString(PyExc_RuntimeError, const_cast<char *>(error.what()));
        return nullptr;
    }
}

// Run ScriptAddJuice() on the interpreter's argument tuple.
// NTSC-U/C: 0x001480d0, PAL: 0x00148cc0
PyObject *PyInvokeAddJuice(PyObject *, PyObject *pArgs) {
    try {
        Py::Tuple args(pArgs);
        Py::Object result = ScriptAddJuice(args);
        return Py::new_reference_to(result);
    } catch (Py::Exception &) {
        return nullptr;
    } catch (std::exception &error) {
        PyErr_SetString(PyExc_RuntimeError, const_cast<char *>(error.what()));
        return nullptr;
    }
}

// Run ScriptAdvanceSection() on the interpreter's argument tuple.
// NTSC-U/C: 0x001487d8, PAL: 0x001493e8
PyObject *PyInvokeAdvanceSection(PyObject *, PyObject *pArgs) {
    try {
        Py::Tuple args(pArgs);
        Py::Object result = ScriptAdvanceSection(args);
        return Py::new_reference_to(result);
    } catch (Py::Exception &) {
        return nullptr;
    } catch (std::exception &error) {
        PyErr_SetString(PyExc_RuntimeError, const_cast<char *>(error.what()));
        return nullptr;
    }
}

// Run ScriptWinWithPointsCheat() on the interpreter's argument tuple.
// NTSC-U/C: 0x0014a490, PAL: 0x0014b0c0
PyObject *PyInvokeDoWinWithPointsCheat(PyObject *, PyObject *pArgs) {
    try {
        Py::Tuple args(pArgs);
        Py::Object result = ScriptWinWithPointsCheat(args);
        return Py::new_reference_to(result);
    } catch (Py::Exception &) {
        return nullptr;
    } catch (std::exception &error) {
        PyErr_SetString(PyExc_RuntimeError, const_cast<char *>(error.what()));
        return nullptr;
    }
}

// Run ScriptEnableAllTracksCheat() on the interpreter's argument tuple.
// NTSC-U/C: 0x0014a810, PAL: 0x0014b440
PyObject *PyInvokeDoEnableAllTracksCheat(PyObject *, PyObject *pArgs) {
    try {
        Py::Tuple args(pArgs);
        Py::Object result = ScriptEnableAllTracksCheat(args);
        return Py::new_reference_to(result);
    } catch (Py::Exception &) {
        return nullptr;
    } catch (std::exception &error) {
        PyErr_SetString(PyExc_RuntimeError, const_cast<char *>(error.what()));
        return nullptr;
    }
}

// Run ScriptActivateListenMode() on the interpreter's argument tuple.
// NTSC-U/C: 0x00148d50, PAL: 0x00149960
PyObject *PyInvokeActivateListenMode(PyObject *, PyObject *pArgs) {
    try {
        Py::Tuple args(pArgs);
        Py::Object result = ScriptActivateListenMode(args);
        return Py::new_reference_to(result);
    } catch (Py::Exception &) {
        return nullptr;
    } catch (std::exception &error) {
        PyErr_SetString(PyExc_RuntimeError, const_cast<char *>(error.what()));
        return nullptr;
    }
}

// Run ScriptSaveRnd() on the interpreter's argument tuple.
// NTSC-U/C: 0x00146750, PAL: 0x001472e0
PyObject *PyInvokeSaveRnd(PyObject *, PyObject *pArgs) {
    try {
        Py::Tuple args(pArgs);
        Py::Object result = ScriptSaveRnd(args);
        return Py::new_reference_to(result);
    } catch (Py::Exception &) {
        return nullptr;
    } catch (std::exception &error) {
        PyErr_SetString(PyExc_RuntimeError, const_cast<char *>(error.what()));
        return nullptr;
    }
}

// Run ScriptEmptyAlbumCaches() on the interpreter's argument tuple.
// NTSC-U/C: 0x003f69c0, PAL: 0x0042f1d0
PyObject *PyInvokeEmptyAlbumCaches(PyObject *, PyObject *pArgs) {
    try {
        Py::Tuple args(pArgs);
        Py::Object result = ScriptEmptyAlbumCaches(args);
        return Py::new_reference_to(result);
    } catch (Py::Exception &) {
        return nullptr;
    } catch (std::exception &error) {
        PyErr_SetString(PyExc_RuntimeError, const_cast<char *>(error.what()));
        return nullptr;
    }
}

// The script interface this file exports, registered in static initialisation.
// NTSC-U/C: 0x00147af8, PAL: 0x001486b0
const ScriptFunc kSaveRndFunc("save_rnd", PyInvokeSaveRnd);
// NTSC-U/C: 0x00148318, PAL: 0x00148f08
const ScriptFunc kAddJuiceFunc("add_juice", PyInvokeAddJuice);
// NTSC-U/C: 0x00148a20, PAL: 0x00149630
const ScriptFunc kAdvanceSectionFunc("advance_section", PyInvokeAdvanceSection);
// NTSC-U/C: 0x0014e5b8, PAL: 0x0014f280
const ScriptFunc kActivateListenModeFunc("activate_listen_mode", PyInvokeActivateListenMode);
const ScriptFunc kActivatePracticeModeFunc("activate_practice_mode", PyInvokeActivatePracticeMode);
const ScriptFunc kActivateAllAccessModeFunc("activate_all_access_mode",
                                            PyInvokeActivateAllAccessMode);
const ScriptFunc kEnableTeamFreqsFunc("enable_team_freqs", PyInvokeEnableTeamFreqs);
const ScriptFunc kEnablePowerupCheatsFunc("enable_powerup_cheats", PyInvokeEnablePowerupCheats);
const ScriptFunc kDoPowerupCheatFunc("do_powerup_cheat", PyInvokeDoPowerupCheat);
const ScriptFunc kDoBigGemModeCheatFunc("do_big_gem_mode_cheat", PyInvokeDoBigGemModeCheat);
const ScriptFunc kDoNoLatticeModeCheatFunc("do_no_lattice_mode_cheat",
                                           PyInvokeDoNoLatticeModeCheat);
const ScriptFunc kDoWinWithPointsCheatFunc("do_win_with_points_cheat",
                                           PyInvokeDoWinWithPointsCheat);
const ScriptFunc kDoEnableAllTracksCheatFunc("do_enable_all_tracks_cheat",
                                             PyInvokeDoEnableAllTracksCheat);
const ScriptFunc kDoArenaStateCycleCheatFunc("do_arena_state_cycle_cheat",
                                             PyInvokeDoArenaStateCycleCheat);
const ScriptFunc kDoExpansionPackToggleCheatFunc("do_expansion_pack_toggle_cheat",
                                                 PyInvokeDoExpansionPackToggleCheat);
const ScriptFunc kDoWinSequenceCheatFunc("do_win_sequence_cheat", PyInvokeDoWinSequenceCheat);
// NTSC-U/C: 0x003f6760, PAL: 0x0042ef48
const ScriptFunc kClearAlbumCacheFunc("clear_album_cache", PyInvokeEmptyAlbumCaches);

} // namespace
