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
#include "script/scripthost.h"

namespace {

// Tracks the enable-all-tracks cheat frees, one SetFreeUntil() each.
constexpr int kCheatTrackCount = 8;

// Template the powerup cheat runs.
constexpr int kPowerupCheatTemplate = 207;

// Add juice to the gamer.
//
// The error names the enable_freestyle command, whose check this repeats.
// 0x00147e40
Py::Object ScriptAddJuice(const Py::Tuple &args) {
    if (args.length() != 1) {
        throw Py::TypeError(HxStr(FormatString("wrong # args for enable_freestyle")));
    }
    const long nAmount = Py::Int(args.getItem(0));
    Application::shared()->GetWorld()->mGamer->AddJuice(static_cast<int>(nAmount));
    return Py::Object();
}

// Move the gamer to a song position.
// 0x00148540
Py::Object ScriptAdvanceSection(const Py::Tuple &args) {
    if (args.length() != 1) {
        throw Py::TypeError(HxStr(FormatString("wrong # args for advance_section")));
    }
    const long nPosition = Py::Int(args.getItem(0));
    Mid::MBT position(static_cast<int>(nPosition));
    Application::shared()->GetWorld()->mGamer->AdvanceAt(position);
    return Py::Object();
}

// End the game with a score.
// 0x0014a200
Py::Object ScriptWinWithPointsCheat(const Py::Tuple &args) {
    PlayActivateSound();
    if (args.length() != 1) {
        throw Py::TypeError(HxStr(FormatString("wrong # args for do_win_with_points_cheat")));
    }
    const long nScore = Py::Int(args.getItem(0));
    Application::shared()->GetWorld()->mGamer->EndWithScore(static_cast<int>(nScore));
    return Py::Object();
}

// Free every track of its requirements.
//
// Each track stays free until bar -1, which never arrives, so every track is enabled. Jam
// sessions keep their requirements.
// 0x0014a6d8
Py::Object ScriptEnableAllTracksCheat([[maybe_unused]] const Py::Tuple &args) {
    PlayActivateSound();
    GrooveWorld *pWorld = Application::shared()->GetWorld();
    if (pWorld != nullptr && Application::shared()->GetPlayMode() != kPlayModeJam) {
        if (pWorld->mGamer->mEnableMgr != nullptr) {
            for (int nTrack = 0; nTrack < kCheatTrackCount; ++nTrack) {
                pWorld->mGamer->mEnableMgr->SetFreeUntil(nTrack, 0, -1);
            }
        }
        pWorld->mGamer->mUnknown98 = 1;
    }
    return Py::Object();
}

// Enter listen mode.
//
// Ends the game scoreless and arms the cheat flag, everywhere but in a jam session.
// 0x00148c68
Py::Object ScriptActivateListenMode([[maybe_unused]] const Py::Tuple &args) {
    PlayActivateSound();
    GrooveWorld *pWorld = Application::shared()->GetWorld();
    if (pWorld != nullptr && Application::shared()->GetPlayMode() != kPlayModeJam) {
        pWorld->mGamer->EndWithScore(0);
        pWorld->mGamer->mUnknown98 = 1;
    }
    return Py::Object();
}

// Save the live scene to tunnel/livegame.rnd.
// 0x00146590
Py::Object ScriptSaveRnd([[maybe_unused]] const Py::Tuple &args) {
    Rnd::FilePath::SetRoot(MakeFreqPath(HxStr("tunnel")));
    Rnd::g_manager.SaveFile(MakeFreqPath(HxStr("tunnel/livegame.rnd")));
    return Py::Object();
}

// Empty the album caches.
//
// The binary expands the three steps of ClearAlbumCache() inline; calling it repeats them
// without duplicating the body, which met/albumcache.h records.
// 0x003f6860
Py::Object ScriptEmptyAlbumCaches([[maybe_unused]] const Py::Tuple &args) {
    ClearAlbumCache();
    return Py::Object();
}

// Enter practice mode.
// 0x00148f98
PyObject *PyInvokeActivatePracticeMode(PyObject *, PyObject *) {
    try {
        PlayActivateSound();
        GrooveWorld *pWorld = Application::shared()->GetWorld();
        if (pWorld != nullptr) {
            pWorld->mGamer->mUnknown1c = 1;
            pWorld->mGamer->mUnknown98 = 1;
        }
        return Py::new_reference_to(Py::Object());
    } catch (Py::Exception &) {
        return nullptr;
    }
}

// Unlock every stage.
// 0x00149230
PyObject *PyInvokeActivateAllAccessMode(PyObject *, PyObject *) {
    try {
        MetRenderer *pRenderer =
            dynamic_cast<MetRenderer *>(Application::shared()->GetMetaWorld()->GetRenderer());
        if (pRenderer != nullptr) {
            pRenderer->UnlockAllStages();
        }
        return Py::new_reference_to(Py::Object());
    } catch (Py::Exception &) {
        return nullptr;
    }
}

// Offer the team identities.
//
// Only from the logo screen, where the cheat is entered.
// 0x001494f8
PyObject *PyInvokeEnableTeamFreqs(PyObject *, PyObject *) {
    try {
        MetRenderer *pRenderer =
            dynamic_cast<MetRenderer *>(Application::shared()->GetMetaWorld()->GetRenderer());
        if (pRenderer->IsLogoScreenActive() != 0) {
            PlayActivateSound();
            GlobalSettings::shared()->mTeamFreqUnlocked = 1;
        }
        return Py::new_reference_to(Py::Object());
    } catch (Py::Exception &) {
        return nullptr;
    }
}

// Grant a powerup through its script template.
//
// Only from the logo screen, where the cheat is entered.
// 0x001497d8
PyObject *PyInvokeEnablePowerupCheats(PyObject *, PyObject *) {
    try {
        MetRenderer *pRenderer =
            dynamic_cast<MetRenderer *>(Application::shared()->GetMetaWorld()->GetRenderer());
        if (pRenderer->IsLogoScreenActive() != 0) {
            PlayActivateSound();
            CallScriptTemplate(kPowerupCheatTemplate);
        }
        return Py::new_reference_to(Py::Object());
    } catch (Py::Exception &) {
        return nullptr;
    }
}

// Play the powerup cheat sound.
// 0x00149ab0
PyObject *PyInvokeDoPowerupCheat(PyObject *, PyObject *) {
    try {
        PlayActivateSound();
        return Py::new_reference_to(Py::Object());
    } catch (Py::Exception &) {
        return nullptr;
    }
}

// Play the big-gem cheat sound.
// 0x00149d20
PyObject *PyInvokeDoBigGemModeCheat(PyObject *, PyObject *) {
    try {
        PlayActivateSound();
        return Py::new_reference_to(Py::Object());
    } catch (Py::Exception &) {
        return nullptr;
    }
}

// Play the no-lattice cheat sound.
// 0x00149f90
PyObject *PyInvokeDoNoLatticeModeCheat(PyObject *, PyObject *) {
    try {
        PlayActivateSound();
        return Py::new_reference_to(Py::Object());
    } catch (Py::Exception &) {
        return nullptr;
    }
}

// Play the arena-cycle cheat sound.
// 0x0014aa58
PyObject *PyInvokeDoArenaStateCycleCheat(PyObject *, PyObject *) {
    try {
        PlayActivateSound();
        return Py::new_reference_to(Py::Object());
    } catch (Py::Exception &) {
        return nullptr;
    }
}

// Flip the expansion-pack flag.
//
// Only from the logo screen, where the cheat is entered.
// 0x0014acc8
PyObject *PyInvokeDoExpansionPackToggleCheat(PyObject *, PyObject *) {
    try {
        MetRenderer *pRenderer =
            dynamic_cast<MetRenderer *>(Application::shared()->GetMetaWorld()->GetRenderer());
        if (pRenderer->IsLogoScreenActive() != 0) {
            PlayActivateSound();
            GlobalSettings::shared()->mGameOptions.mUnknown04 ^= 1;
        }
        return Py::new_reference_to(Py::Object());
    } catch (Py::Exception &) {
        return nullptr;
    }
}

// Flag the win sequence to run.
// 0x0014afa8
PyObject *PyInvokeDoWinSequenceCheat(PyObject *, PyObject *) {
    try {
        SetDoWinSequence(1);
        return Py::new_reference_to(Py::Object());
    } catch (Py::Exception &) {
        return nullptr;
    }
}

// Run ScriptAddJuice() on the interpreter's argument tuple.
// 0x001480d0
PyObject *PyInvokeAddJuice(PyObject *, PyObject *pArgs) {
    try {
        Py::Tuple args(pArgs);
        Py::Object result = ScriptAddJuice(args);
        return Py::new_reference_to(result);
    } catch (Py::Exception &) {
        return nullptr;
    }
}

// Run ScriptAdvanceSection() on the interpreter's argument tuple.
// 0x001487d8
PyObject *PyInvokeAdvanceSection(PyObject *, PyObject *pArgs) {
    try {
        Py::Tuple args(pArgs);
        Py::Object result = ScriptAdvanceSection(args);
        return Py::new_reference_to(result);
    } catch (Py::Exception &) {
        return nullptr;
    }
}

// Run ScriptWinWithPointsCheat() on the interpreter's argument tuple.
// 0x0014a490
PyObject *PyInvokeDoWinWithPointsCheat(PyObject *, PyObject *pArgs) {
    try {
        Py::Tuple args(pArgs);
        Py::Object result = ScriptWinWithPointsCheat(args);
        return Py::new_reference_to(result);
    } catch (Py::Exception &) {
        return nullptr;
    }
}

// Run ScriptEnableAllTracksCheat() on the interpreter's argument tuple.
// 0x0014a810
PyObject *PyInvokeDoEnableAllTracksCheat(PyObject *, PyObject *pArgs) {
    try {
        Py::Tuple args(pArgs);
        Py::Object result = ScriptEnableAllTracksCheat(args);
        return Py::new_reference_to(result);
    } catch (Py::Exception &) {
        return nullptr;
    }
}

// Run ScriptActivateListenMode() on the interpreter's argument tuple.
// 0x00148d50
PyObject *PyInvokeActivateListenMode(PyObject *, PyObject *pArgs) {
    try {
        Py::Tuple args(pArgs);
        Py::Object result = ScriptActivateListenMode(args);
        return Py::new_reference_to(result);
    } catch (Py::Exception &) {
        return nullptr;
    }
}

// Run ScriptSaveRnd() on the interpreter's argument tuple.
// 0x00146750
PyObject *PyInvokeSaveRnd(PyObject *, PyObject *pArgs) {
    try {
        Py::Tuple args(pArgs);
        Py::Object result = ScriptSaveRnd(args);
        return Py::new_reference_to(result);
    } catch (Py::Exception &) {
        return nullptr;
    }
}

// Run ScriptEmptyAlbumCaches() on the interpreter's argument tuple.
// 0x003f69c0
PyObject *PyInvokeEmptyAlbumCaches(PyObject *, PyObject *pArgs) {
    try {
        Py::Tuple args(pArgs);
        Py::Object result = ScriptEmptyAlbumCaches(args);
        return Py::new_reference_to(result);
    } catch (Py::Exception &) {
        return nullptr;
    }
}

} // namespace
