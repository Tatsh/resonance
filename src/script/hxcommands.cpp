#include <cstring>
#include <exception>
#include <libscf.h>
#include <sstream>

#include "app/application.h"
#include "app/attachment.h"
#include "app/globals.h"
#include "app/scheduler.h"
#include "app/systemtime.h"
#include "game/gamemanagerimpl.h"
#include "game/gamer.h"
#include "game/grooveworld.h"
#include "game/localplayer.h"
#include "game/playmap.h"
#include "game/powerupcollection.h"
#include "game/scoretrackgraph.h"
#include "game/trackdata.h"
#include "gfx/vram.h"
#include "mid/mbt.h"
#include "os/filelog.h"
#include "os/formatstring.h"
#include "os/heap.h"
#include "os/hostmode.h"
#include "os/hxstr.h"
#include "os/log.h"
#include "os/mem.h"
#include "os/seccache.h"
#include "os/spew.h"
#include "os/zone.h"
#include "sch/cmdid.h"
#include "sch/command.h"
#include "sch/tempomap.h"
#include "sch/tickclock.h"
#include "script/cxx/config.h"
#include "script/cxx/int.h"
#include "script/cxx/nameerror.h"
#include "script/cxx/object.h"
#include "script/cxx/string.h"
#include "script/cxx/tuple.h"
#include "script/registercfunction.h"
#include "script/scriptcmd.h"
#include "script/testregistry.h"
#include "synth/midi_main.h"
#include "synth/ps2hardsynth.h"

namespace {

// The number of calls the memlog terminator has made. The name is inferred from the count.
// NTSC-U/C: 0x00676750, PAL: 0x006b75a8
int g_nMemlogTermCalls = 1;

// Report the frequency root.
// NTSC-U/C: 0x00506f50, PAL: 0x00545e30
Py::Object ScriptGetFreqRoot([[maybe_unused]] Py::Tuple args) {
    HxStr root = GetFreqRoot();
    Py::String text(root);
    return text;
}

// Run ScriptGetFreqRoot() on the interpreter's argument tuple.
// NTSC-U/C: 0x00507138, PAL: 0x00546038
PyObject *PyInvokeGetFreqRoot(PyObject *, PyObject *pArgs) {
    try {
        Py::Tuple args(pArgs);
        Py::Object result = ScriptGetFreqRoot(args);
        return Py::new_reference_to(result);
    } catch (Py::Exception &) {
        return nullptr;
    } catch (std::exception &error) {
        PyErr_SetString(PyExc_RuntimeError, const_cast<char *>(error.what()));
        return nullptr;
    }
}

// Record the watchdog's snapshot.
// NTSC-U/C: 0x00119048, PAL: 0x001195a8
// PyInvokeKillSch() expands this inline, and the out-of-line copy has no caller.
inline Py::Object ScriptKillSch([[maybe_unused]] const Py::Tuple &args) {
    Application::shared()->GetWatchdog()->Snapshot();
    return Py::Object();
}

// Run ScriptKillSch() on the interpreter's argument tuple.
// NTSC-U/C: 0x00117388, PAL: 0x00117830
PyObject *PyInvokeKillSch(PyObject *, PyObject *pArgs) {
    try {
        Py::Tuple args(pArgs);
        Py::Object result = ScriptKillSch(args);
        return Py::new_reference_to(result);
    } catch (Py::Exception &) {
        return nullptr;
    } catch (std::exception &error) {
        PyErr_SetString(PyExc_RuntimeError, const_cast<char *>(error.what()));
        return nullptr;
    }
}

// Shut the memory, heap, and file loggers down.
//
// Dumps the sector cache and the heap log, logs the shutdown, closes the memory log and the
// file log, and writes the Python heap's statistics and contents to heapdump.txt.
// NTSC-U/C: 0x00155dc0, PAL: 0x00157980
Py::Object ScriptMemlogTerm([[maybe_unused]] Py::Tuple args) {
    DumpSectorCache();
    DumpHeapMemoryLog(g_nMemlogTermCalls);
    ++g_nMemlogTermCalls;
    LogPrintf("Shutting down memory/heap/fileio loggers...\n");
    MemLogCloseAndContinue();
    FileLogStop();
    g_pPythonHeap->DumpStats();
    g_pPythonHeap->DumpToFile("heapdump.txt");
    return Py::Object();
}

// Run ScriptMemlogTerm() on the interpreter's argument tuple.
// NTSC-U/C: 0x00155f08, PAL: 0x00157ac8
PyObject *PyInvokeMemlogTerm(PyObject *, PyObject *pArgs) {
    try {
        Py::Tuple args(pArgs);
        Py::Object result = ScriptMemlogTerm(args);
        return Py::new_reference_to(result);
    } catch (Py::Exception &) {
        return nullptr;
    } catch (std::exception &error) {
        PyErr_SetString(PyExc_RuntimeError, const_cast<char *>(error.what()));
        return nullptr;
    }
}

#ifdef VIDEO_STANDARD_PAL
// Record the language that the script's one argument identifies.
//
// `french`, `german`, `italian`, and `spanish` select their language, and any other text selects
// English. The European disc's scripts call it at start-up.
// PAL: 0x00156d58
Py::Object ScriptSetLang(const Py::Tuple &args) {
    if (args.length() == 0) {
        throw Py::TypeError(HxStr("requires 1 arg: language"));
    }
    Py::Object element = args.getItem(0);
    Py::String text(element);
    HxStr language = text;
    int nLanguage = SCE_ENGLISH_LANGUAGE;
    if (language == "french") {
        nLanguage = SCE_FRENCH_LANGUAGE;
    } else if (language == "german") {
        nLanguage = SCE_GERMAN_LANGUAGE;
    } else if (language == "italian") {
        nLanguage = SCE_ITALIAN_LANGUAGE;
    } else if (language == "spanish") {
        nLanguage = SCE_SPANISH_LANGUAGE;
    }
    SetLanguage(nLanguage);
    return Py::Object();
}

// Run ScriptSetLang() on the interpreter's argument tuple.
// PAL: 0x001571d8
PyObject *PyInvokeSetLang(PyObject *, PyObject *pArgs) {
    try {
        Py::Tuple args(pArgs);
        Py::Object result = ScriptSetLang(args);
        return Py::new_reference_to(result);
    } catch (Py::Exception &) {
        return nullptr;
    } catch (std::exception &error) {
        PyErr_SetString(PyExc_RuntimeError, const_cast<char *>(error.what()));
        return nullptr;
    }
}

// Report the file-name suffix of the current language, empty for English.
// PAL: 0x00157388
Py::Object ScriptGetLanguageSuffix([[maybe_unused]] Py::Tuple args) {
    const char *pszSuffix;
    switch (GetLanguage()) {
    case SCE_FRENCH_LANGUAGE:
        pszSuffix = "_fre";
        break;
    case SCE_SPANISH_LANGUAGE:
        pszSuffix = "_spa";
        break;
    case SCE_GERMAN_LANGUAGE:
        pszSuffix = "_ger";
        break;
    case SCE_ITALIAN_LANGUAGE:
        pszSuffix = "_ita";
        break;
    default:
        pszSuffix = "";
        break;
    }
    return Py::String(pszSuffix);
}

// Run ScriptGetLanguageSuffix() on the interpreter's argument tuple.
// PAL: 0x00157590
PyObject *PyInvokeGetLanguageSuffix(PyObject *, PyObject *pArgs) {
    try {
        Py::Tuple args(pArgs);
        Py::Object result = ScriptGetLanguageSuffix(args);
        return Py::new_reference_to(result);
    } catch (Py::Exception &) {
        return nullptr;
    } catch (std::exception &error) {
        PyErr_SetString(PyExc_RuntimeError, const_cast<char *>(error.what()));
        return nullptr;
    }
}
#endif

// Stop the game.
//
// Takes no arguments.
// NTSC-U/C: 0x0015e5f0, PAL: 0x00160450
Py::Object ScriptStopGame(const Py::Tuple &args) {
    if (args.length() != 0) {
        throw Py::TypeError(HxStr("requires 0 args"));
    }
    Application::shared()->GetWorld()->PostFinish();
    return Py::Object();
}

// Run ScriptStopGame() on the interpreter's argument tuple.
// NTSC-U/C: 0x0015e718, PAL: 0x00160598
PyObject *PyInvokeStopGame(PyObject *, PyObject *pArgs) {
    try {
        Py::Tuple args(pArgs);
        Py::Object result = ScriptStopGame(args);
        return Py::new_reference_to(result);
    } catch (Py::Exception &) {
        return nullptr;
    } catch (std::exception &error) {
        PyErr_SetString(PyExc_RuntimeError, const_cast<char *>(error.what()));
        return nullptr;
    }
}

// Show a line of text on the display.
// NTSC-U/C: 0x001532d8, PAL: 0x00154128
Py::Object ScriptDisplayText(const Py::Tuple &args) {
    if (args.length() != 1) {
        throw Py::TypeError(HxStr("wrong # args for display_text"));
    }
    Py::Object element = args.getItem(0);
    Py::String text(element);
    HxStr message = text;
    GrooveWorld *pWorld = Application::shared()->GetWorld();
    if (pWorld != nullptr) {
        pWorld->DisplayText(message);
    }
    return Py::Object();
}

// Run ScriptDisplayText() on the interpreter's argument tuple.
// NTSC-U/C: 0x00153590, PAL: 0x00154420
PyObject *PyInvokeDisplayText(PyObject *, PyObject *pArgs) {
    try {
        Py::Tuple args(pArgs);
        Py::Object result = ScriptDisplayText(args);
        return Py::new_reference_to(result);
    } catch (Py::Exception &) {
        return nullptr;
    } catch (std::exception &error) {
        PyErr_SetString(PyExc_RuntimeError, const_cast<char *>(error.what()));
        return nullptr;
    }
}

// Report a value on the screen.
//
// Shows the string form with a newline for fifty ticks.
// NTSC-U/C: 0x001611f8, PAL: 0x00163158
Py::Object ScriptTrace(const Py::Tuple &args) {
    if (args.length() != 1) {
        throw Py::TypeError(HxStr("requires 1 arg"));
    }
    HxStr text = args.getItem(0).as_string();
    text += '\n';
    ShowReportedMessage(text, 50);
    return Py::Object();
}

// Run ScriptTrace() on the interpreter's argument tuple.
// NTSC-U/C: 0x001614c0, PAL: 0x00163490
PyObject *PyInvokeTrace(PyObject *, PyObject *pArgs) {
    try {
        Py::Tuple args(pArgs);
        Py::Object result = ScriptTrace(args);
        return Py::new_reference_to(result);
    } catch (Py::Exception &) {
        return nullptr;
    } catch (std::exception &error) {
        PyErr_SetString(PyExc_RuntimeError, const_cast<char *>(error.what()));
        return nullptr;
    }
}

// Withdraw a scheduled command.
//
// The tuple includes the command handle.
// NTSC-U/C: 0x00159a90, PAL: 0x0015b7b0
Py::Object ScriptCancelCmd(const Py::Tuple &args) {
    if (args.length() != 1) {
        throw Py::TypeError(HxStr("requres 1 arg: cmdId"));
    }
    const int nId = Py::Int(args.getItem(0));
    CmdID id;
    id.mValue = static_cast<int>(nId);
    Application::shared()->GetSongClock()->Withdraw(id);
    return Py::Object();
}

// Run ScriptCancelCmd() on the interpreter's argument tuple.
// NTSC-U/C: 0x00159f78, PAL: 0x0015bcb8
PyObject *PyInvokeCancelCmd(PyObject *, PyObject *pArgs) {
    try {
        Py::Tuple args(pArgs);
        Py::Object result = ScriptCancelCmd(args);
        return Py::new_reference_to(result);
    } catch (Py::Exception &) {
        return nullptr;
    } catch (std::exception &error) {
        PyErr_SetString(PyExc_RuntimeError, const_cast<char *>(error.what()));
        return nullptr;
    }
}

// Freeze the juice.
//
// A non-zero value stops juice changes.
// NTSC-U/C: 0x00154258, PAL: 0x00155108
Py::Object ScriptFreezeJuice(const Py::Tuple &args) {
    if (args.length() != 1) {
        throw Py::TypeError(HxStr("wrong # args for freeze_juice"));
    }
    const int nFreeze = Py::Int(args.getItem(0));
    GrooveWorld *pWorld = Application::shared()->GetWorld();
    if (pWorld != nullptr) {
        pWorld->mGamer->mJuiceFrozen = nFreeze != 0 ? 1 : 0;
    }
    return Py::Object();
}

// Run ScriptFreezeJuice() on the interpreter's argument tuple.
// NTSC-U/C: 0x001544f8, PAL: 0x001553c8
PyObject *PyInvokeFreezeJuice(PyObject *, PyObject *pArgs) {
    try {
        Py::Tuple args(pArgs);
        Py::Object result = ScriptFreezeJuice(args);
        return Py::new_reference_to(result);
    } catch (Py::Exception &) {
        return nullptr;
    } catch (std::exception &error) {
        PyErr_SetString(PyExc_RuntimeError, const_cast<char *>(error.what()));
        return nullptr;
    }
}

// Recreate the session from a recording.
//
// A second argument sets the playback flag.
// NTSC-U/C: 0x0015bb00, PAL: 0x0015d8a0
Py::Object ScriptRecreate(const Py::Tuple &args) {
    if (args.length() <= 0) {
        throw Py::TypeError(HxStr("requres arg: filename [ignore-autoexec]"));
    }
    Py::Object element = args.getItem(0);
    Py::String text(element);
    HxStr filename = text;
    Application::shared()->GetGameManager()->StartPlayback(filename, args.length() == 2);
    return Py::Object();
}

// Run ScriptRecreate() on the interpreter's argument tuple.
// NTSC-U/C: 0x0015c080, PAL: 0x0015de60
PyObject *PyInvokeRecreate(PyObject *, PyObject *pArgs) {
    try {
        Py::Tuple args(pArgs);
        Py::Object result = ScriptRecreate(args);
        return Py::new_reference_to(result);
    } catch (Py::Exception &) {
        return nullptr;
    } catch (std::exception &error) {
        PyErr_SetString(PyExc_RuntimeError, const_cast<char *>(error.what()));
        return nullptr;
    }
}

// Open a freestyle span.
//
// NTSC-U/C: 0x00153a00, PAL: 0x00154890
Py::Object ScriptEnableFreestyle(const Py::Tuple &args) {
    if (args.length() != 2) {
        throw Py::TypeError(HxStr("wrong # args for enable_freestyle"));
    }
    const int nStartBar = Py::Int(args.getItem(0));
    const int nEndBar = Py::Int(args.getItem(1));
    Application::shared()->GetWorld()->mGamer->EnablePlayerFreestyle(static_cast<int>(nStartBar),
                                                                     static_cast<int>(nEndBar));
    return Py::Object();
}

// Run ScriptEnableFreestyle() on the interpreter's argument tuple.
// NTSC-U/C: 0x00153de8, PAL: 0x00154c98
PyObject *PyInvokeEnableFreestyle(PyObject *, PyObject *pArgs) {
    try {
        Py::Tuple args(pArgs);
        Py::Object result = ScriptEnableFreestyle(args);
        return Py::new_reference_to(result);
    } catch (Py::Exception &) {
        return nullptr;
    } catch (std::exception &error) {
        PyErr_SetString(PyExc_RuntimeError, const_cast<char *>(error.what()));
        return nullptr;
    }
}

// Select a powerup by index.
// NTSC-U/C: 0x0015c998, PAL: 0x0015e778
Py::Object ScriptSelectPowerup(const Py::Tuple &args) {
    if (args.length() != 1) {
        throw Py::TypeError(HxStr("wrong # args for select_powerup"));
    }
    const int nPowerup = Py::Int(args.getItem(0));
    GrooveWorld *pWorld = Application::shared()->GetWorld();
    if (pWorld == nullptr) {
        return Py::Object();
    }
    LocalPlayer *pPlayer = dynamic_cast<LocalPlayer *>(pWorld->mLocalPlayers[0]);
    if (pPlayer == nullptr) {
        return Py::Object();
    }
    PowerupCollection *pCollection = dynamic_cast<PowerupCollection *>(pPlayer->mCollection);
    if (pCollection == nullptr) {
        return Py::Object();
    }
    pCollection->Select(static_cast<int>(nPowerup));
    return Py::Object();
}

// Run ScriptSelectPowerup() on the interpreter's argument tuple.
// NTSC-U/C: 0x0015ccb8, PAL: 0x0015eab8
PyObject *PyInvokeSelectPowerup(PyObject *, PyObject *pArgs) {
    try {
        Py::Tuple args(pArgs);
        Py::Object result = ScriptSelectPowerup(args);
        return Py::new_reference_to(result);
    } catch (Py::Exception &) {
        return nullptr;
    } catch (std::exception &error) {
        PyErr_SetString(PyExc_RuntimeError, const_cast<char *>(error.what()));
        return nullptr;
    }
}

// Post script text to run at a song position.
//
// Returns the posted command's handle. The tuple includes the position and the text.
// NTSC-U/C: 0x00159538, PAL: 0x0015b218
Py::Object ScriptPostScript(const Py::Tuple &args) {
    if (args.length() != 2) {
        throw Py::TypeError(HxStr("requres 2 arg: tick, script"));
    }
    const int nTick = Py::Int(args.getItem(0));
    Py::Object element = args.getItem(1);
    Py::String text(element);
    HxStr script = text;
    IsFiniteMBT(static_cast<int>(nTick));
    Sch::Command *pCommand = NewScriptCmd(script);
    CmdID id;
    Application::shared()->GetSongClock()->PostAtSongTick(pCommand, nTick, id);
    Attachment::ReleaseIfSet(pCommand);
    return Py::Int(static_cast<long long>(id.mValue));
}

// Run ScriptPostScript() on the interpreter's argument tuple.
// NTSC-U/C: 0x00159d30, PAL: 0x0015ba70
PyObject *PyInvokePostScript(PyObject *, PyObject *pArgs) {
    try {
        Py::Tuple args(pArgs);
        Py::Object result = ScriptPostScript(args);
        return Py::new_reference_to(result);
    } catch (Py::Exception &) {
        return nullptr;
    } catch (std::exception &error) {
        PyErr_SetString(PyExc_RuntimeError, const_cast<char *>(error.what()));
        return nullptr;
    }
}

// Write the display buffer to a scrndump file.
// NTSC-U/C: 0x0015c580, PAL: 0x0015e360
Py::Object ScriptScreenDump([[maybe_unused]] Py::Tuple args) {
    g_vramTable.Screendump("scrndump");
    return Py::Object();
}

// Run ScriptScreenDump() on the interpreter's argument tuple.
// NTSC-U/C: 0x0015c688, PAL: 0x0015e468
PyObject *PyInvokeScreenDump(PyObject *, PyObject *pArgs) {
    try {
        Py::Tuple args(pArgs);
        Py::Object result = ScriptScreenDump(args);
        return Py::new_reference_to(result);
    } catch (Py::Exception &) {
        return nullptr;
    } catch (std::exception &error) {
        PyErr_SetString(PyExc_RuntimeError, const_cast<char *>(error.what()));
        return nullptr;
    }
}

// Run a synthesiser command.
//
// Takes any positive argument count.
// NTSC-U/C: 0x0015eb88, PAL: 0x00160a08
Py::Object ScriptSynthCmd(const Py::Tuple &args) {
    if (args.length() <= 0) {
        throw Py::TypeError(HxStr("wrong # args for synth_info"));
    }
    const int nCommand = Py::Int(args.getItem(0));
    SynthCommand(static_cast<int>(nCommand));
    return Py::Object();
}

// Run ScriptSynthCmd() on the interpreter's argument tuple.
// NTSC-U/C: 0x0015ee00, PAL: 0x00160ca0
PyObject *PyInvokeSynthCmd(PyObject *, PyObject *pArgs) {
    try {
        Py::Tuple args(pArgs);
        Py::Object result = ScriptSynthCmd(args);
        return Py::new_reference_to(result);
    } catch (Py::Exception &) {
        return nullptr;
    } catch (std::exception &error) {
        PyErr_SetString(PyExc_RuntimeError, const_cast<char *>(error.what()));
        return nullptr;
    }
}

// Set a track's MIDI volume.
//
// The name is score or bg, although both select the same path. The volume goes out as
// controller 7 on the track's channel.
// NTSC-U/C: 0x00158060, PAL: 0x00159cf0
Py::Object ScriptSetVolume(const Py::Tuple &args) {
    if (args.length() != 3) {
        throw Py::TypeError(HxStr("requires 3 args"));
    }
    Py::Object element = args.getItem(0);
    Py::String text(element);
    HxStr name = text;
    const int nTrack = Py::Int(args.getItem(1));
    const int nVolume = Py::Int(args.getItem(2));
    if (name == "score" || name == "bg") {
        GrooveWorld *pWorld = Application::shared()->GetWorld();
        const int nChannel = pWorld->mTrackGraphs[nTrack]->mTrackData->mChannel;
        Application::shared()->GetSynth()->SendMidi(nChannel | 0xB0, 7, nVolume & 0xFF);
    }
    return Py::Object();
}

// Run ScriptSetVolume() on the interpreter's argument tuple.
// NTSC-U/C: 0x00158660, PAL: 0x0015a340
PyObject *PyInvokeSetVolume(PyObject *, PyObject *pArgs) {
    try {
        Py::Tuple args(pArgs);
        Py::Object result = ScriptSetVolume(args);
        return Py::new_reference_to(result);
    } catch (Py::Exception &) {
        return nullptr;
    } catch (std::exception &error) {
        PyErr_SetString(PyExc_RuntimeError, const_cast<char *>(error.what()));
        return nullptr;
    }
}

// Send MIDI messages.
//
// The tuple includes a device name, a command, and four values. Voice and fx select their
// channels, then any command with a last value of 1 sends bank selects on every channel.
// Otherwise play sends a note on, stop a note off, and bank_swap moves the stream. The
// channel nibble reads 0 where it is consumed, because voice and fx never arrive at play or
// stop with their values.
// NTSC-U/C: 0x00157578, PAL: 0x001591a8
Py::Object ScriptMidi(const Py::Tuple &args) {
    if (args.length() != 6) {
        throw Py::TypeError(HxStr("requires 6 args"));
    }
    Py::Object deviceElement = args.getItem(0);
    Py::String deviceText(deviceElement);
    HxStr device = deviceText;
    Py::Object commandElement = args.getItem(1);
    Py::String commandText(commandElement);
    HxStr command = commandText;
    const int nA = Py::Int(args.getItem(2));
    const int nB = Py::Int(args.getItem(3));
    const int nC = Py::Int(args.getItem(4));
    const int nD = Py::Int(args.getItem(5));
    int nChannel = 0;
    if (command == "voice") {
        nChannel = 0xE;
    } else if (command == "fx") {
        nChannel = 0xF;
    }
    if (nD == 1) {
        for (int nCh = 0; nCh < 15; ++nCh) {
            const int nStatus = (nCh - 0x50) & 0xFF;
            Application::shared()->GetSynth()->SendMidi(nStatus, 0, 0);
            Application::shared()->GetSynth()->SendMidi(nStatus, 0x20, nC & 0xFF);
        }
    }
    if (command == "play") {
        Application::shared()->GetSynth()->SendMidi(nChannel | 0x90, nA & 0xFF, nB & 0xFF);
    }
    if (command == "stop") {
        Application::shared()->GetSynth()->SendMidi(nChannel | 0x80, nA & 0xFF, 0);
    }
    if (command == "bank_swap") {
        SetSynthStreamBar(static_cast<int>(nA));
    }
    return Py::Object();
}

// Run ScriptMidi() on the interpreter's argument tuple.
// NTSC-U/C: 0x001588a8, PAL: 0x0015a588
PyObject *PyInvokeMidi(PyObject *, PyObject *pArgs) {
    try {
        Py::Tuple args(pArgs);
        Py::Object result = ScriptMidi(args);
        return Py::new_reference_to(result);
    } catch (Py::Exception &) {
        return nullptr;
    } catch (std::exception &error) {
        PyErr_SetString(PyExc_RuntimeError, const_cast<char *>(error.what()));
        return nullptr;
    }
}

// Run a named self-test.
//
// Reports `ok` for a passing test and `not ok` otherwise. An unknown name throws NameError
// over the valid test list.
// NTSC-U/C: 0x0015f270, PAL: 0x00161110
Py::Object ScriptTest(const Py::Tuple &args) {
    if (args.length() <= 0) {
        throw Py::TypeError(HxStr("requires 1 arg (name of test)"));
    }
    HxStr name = args.getItem(0).as_string();
    for (int i = 0; i < TestRegistry::sTestCount; ++i) {
        if (std::strcmp(TestRegistry::sTests[i].mName, name.mStr) == 0) {
            const int nPassed = TestRegistry::sTests[i].mFunc();
            return Py::String(nPassed != 0 ? "ok" : "not ok");
        }
    }
    std::ostringstream report;
    report << "wrong arg: test \"" << name << "\" does not exist\n";
    report << "valid tests are:";
    for (int i = 0; i < TestRegistry::sTestCount; ++i) {
        report << "\n  " << TestRegistry::sTests[i].mName;
    }
    throw Py::NameError(HxStr(report.str().c_str()));
}

// Run ScriptTest() on the interpreter's argument tuple.
// NTSC-U/C: 0x0015fa20, PAL: 0x00161940
PyObject *PyInvokeTest(PyObject *, PyObject *pArgs) {
    try {
        Py::Tuple args(pArgs);
        Py::Object result = ScriptTest(args);
        return Py::new_reference_to(result);
    } catch (Py::Exception &) {
        return nullptr;
    } catch (std::exception &error) {
        PyErr_SetString(PyExc_RuntimeError, const_cast<char *>(error.what()));
        return nullptr;
    }
}

// Report the spew connections, and optionally connect a channel.
//
// Without arguments reports the connections. With a file and a channel, connects them first.
// The report is built before the arguments are checked.
// NTSC-U/C: 0x0015d950, PAL: 0x0015f750
Py::Object ScriptSpew(const Py::Tuple &args) {
    Spew &shared = Spew::shared();
    std::ostringstream report;
    report << "Spew Connections:\n";
    shared.PrintConnections(report);
    Py::Object result = Py::String(report.str().c_str());
    if (args.length() == 0) {
        return result;
    }
    if (args.length() != 2) {
        throw Py::TypeError(HxStr("requires arguments: HxStr, HxStr"));
    }
    Py::Object fileElement = args.getItem(0);
    Py::String fileText(fileElement);
    HxStr file = fileText;
    Py::Object channelElement = args.getItem(1);
    Py::String channelText(channelElement);
    HxStr channel = channelText;
    shared.Connect(file, channel);
    return result;
}

// Run ScriptSpew() on the interpreter's argument tuple.
// NTSC-U/C: 0x0015e180, PAL: 0x0015ffe0
PyObject *PyInvokeSpew(PyObject *, PyObject *pArgs) {
    try {
        Py::Tuple args(pArgs);
        Py::Object result = ScriptSpew(args);
        return Py::new_reference_to(result);
    } catch (Py::Exception &) {
        return nullptr;
    } catch (std::exception &error) {
        PyErr_SetString(PyExc_RuntimeError, const_cast<char *>(error.what()));
        return nullptr;
    }
}

// Ticks per measure, four quarter notes at 480 ticks each.
constexpr int kTicksPerMeasure = 1920;

// Drive the song clock.
//
// Pause and start halt and release it, step advances it by milliseconds, tempo reports or sets
// the microseconds per quarter, tick reports the song position, and song_bar reports the
// play map's section there.
// NTSC-U/C: 0x00150d88, PAL: 0x00151ad8
Py::Object ScriptClock(Py::Tuple args) {
    if (args.length() == 0) {
        throw Py::TypeError(HxStr("requires 1st arg: pause, start, step, tempo, tick"));
    }
    Py::Object element = args.getItem(0);
    Py::String text(element);
    HxStr command = text;
    Sch::Scheduler *pWatchdog = Application::shared()->GetWatchdog();
    if (command == "pause") {
        pWatchdog->mClock.Pause();
        return Py::Object();
    }
    if (command == "start") {
        pWatchdog->mClock.Resume();
        return Py::Object();
    }
    if (command == "step") {
        if (args.length() != 2) {
            throw Py::TypeError(HxStr("requires 2nd arg: milliseconds"));
        }
        const int nMs = Py::Int(args.getItem(1));
        pWatchdog->mClock.Advance(static_cast<int>(nMs));
        return Py::Object();
    }
    if (command == "tempo") {
        Sch::TickClock *pClock = Application::shared()->GetSongClock();
        if (args.length() == 1) {
            return Py::Int(static_cast<long long>(pClock->mTempoMap->mMicrosecondsPerQuarter));
        }
        const int nTempo = Py::Int(args.getItem(1));
        const int nTick = pClock->SongTick();
        pClock->mTempoMap->SetTempo(static_cast<int>(nTempo), nTick);
        return Py::Object();
    }
    if (command == "tick") {
        Sch::TickClock *pClock = Application::shared()->GetSongClock();
        return Py::Int(static_cast<long long>(pClock->SongTick()));
    }
    if (command == "song_bar") {
        Sch::TickClock *pClock = Application::shared()->GetSongClock();
        const int nBar = pClock->SongTick() / kTicksPerMeasure;
        return Py::Int(static_cast<long long>(Application::shared()->GetPlayMap()->MapBar(nBar)));
    }
    throw Py::TypeError(HxStr("requires 1st arg: pause, start, step, tempo, tick"));
}

// Run ScriptClock() on the interpreter's argument tuple.
// NTSC-U/C: 0x00151b20, PAL: 0x00152900
PyObject *PyInvokeClock(PyObject *, PyObject *pArgs) {
    try {
        Py::Tuple args(pArgs);
        Py::Object result = ScriptClock(args);
        return Py::new_reference_to(result);
    } catch (Py::Exception &) {
        return nullptr;
    } catch (std::exception &error) {
        PyErr_SetString(PyExc_RuntimeError, const_cast<char *>(error.what()));
        return nullptr;
    }
}

// Dump the zone table.
// NTSC-U/C: 0x00163a10, PAL: 0x00165ac0
// PyInvokeZoneDump() expands this inline, and the out-of-line copy has no caller.
inline Py::Object ScriptZoneDump([[maybe_unused]] const Py::Tuple &args) {
    ZoneDump();
    return Py::Object();
}

// Run ScriptZoneDump() on the interpreter's argument tuple.
// NTSC-U/C: 0x00163660, PAL: 0x00165710
PyObject *PyInvokeZoneDump(PyObject *, PyObject *pArgs) {
    try {
        Py::Tuple args(pArgs);
        Py::Object result = ScriptZoneDump(args);
        return Py::new_reference_to(result);
    } catch (Py::Exception &) {
        return nullptr;
    } catch (std::exception &error) {
        PyErr_SetString(PyExc_RuntimeError, const_cast<char *>(error.what()));
        return nullptr;
    }
}

// Start a recording capture.
// NTSC-U/C: 0x0015c420, PAL: 0x0015e200
// PyInvokeCapture() expands this inline, and the out-of-line copy has no caller.
inline Py::Object ScriptCapture([[maybe_unused]] const Py::Tuple &args) {
    Application::shared()->GetGameManager()->StartRecording();
    return Py::Object();
}

// Run ScriptCapture() on the interpreter's argument tuple.
// NTSC-U/C: 0x0015bdf8, PAL: 0x0015dbd8
PyObject *PyInvokeCapture(PyObject *, PyObject *pArgs) {
    try {
        Py::Tuple args(pArgs);
        Py::Object result = ScriptCapture(args);
        return Py::new_reference_to(result);
    } catch (Py::Exception &) {
        return nullptr;
    } catch (std::exception &error) {
        PyErr_SetString(PyExc_RuntimeError, const_cast<char *>(error.what()));
        return nullptr;
    }
}

// Silence every channel.
// NTSC-U/C: 0x00159410, PAL: 0x0015b0f0
// PyInvokeStopAllMidi() expands this inline, and the out-of-line copy has no caller.
inline Py::Object ScriptStopAllMidi([[maybe_unused]] const Py::Tuple &args) {
    if (Application::shared()->GetSynth() != nullptr) {
        // Yes, the binary fetches the synthesiser a second time.
        Application::shared()->GetSynth()->AllNotesOff();
    }
    return Py::Object();
}

// Run ScriptStopAllMidi() on the interpreter's argument tuple.
// NTSC-U/C: 0x00158af0, PAL: 0x0015a7d0
PyObject *PyInvokeStopAllMidi(PyObject *, PyObject *pArgs) {
    try {
        Py::Tuple args(pArgs);
        Py::Object result = ScriptStopAllMidi(args);
        return Py::new_reference_to(result);
    } catch (Py::Exception &) {
        return nullptr;
    } catch (std::exception &error) {
        PyErr_SetString(PyExc_RuntimeError, const_cast<char *>(error.what()));
        return nullptr;
    }
}

// Win with five hundred points.
// NTSC-U/C: 0x00150cc0, PAL: 0x00151a10
// PyInvokeCheatWin() expands this inline, and the out-of-line copy has no caller.
inline Py::Object ScriptCheatWin([[maybe_unused]] const Py::Tuple &args) {
    GrooveWorld *pWorld = Application::shared()->GetWorld();
    if (pWorld != nullptr) {
        pWorld->mGamer->EndWithScore(500);
    }
    return Py::Object();
}

// Run ScriptCheatWin() on the interpreter's argument tuple.
// NTSC-U/C: 0x001508f8, PAL: 0x00151648
PyObject *PyInvokeCheatWin(PyObject *, PyObject *pArgs) {
    try {
        Py::Tuple args(pArgs);
        Py::Object result = ScriptCheatWin(args);
        return Py::new_reference_to(result);
    } catch (Py::Exception &) {
        return nullptr;
    } catch (std::exception &error) {
        PyErr_SetString(PyExc_RuntimeError, const_cast<char *>(error.what()));
        return nullptr;
    }
}

// The script interface this file exports, registered in static initialisation.
// NTSC-U/C: 0x00118960, PAL: 0x00118e78
const RegisterCFunction kKillschFunc("killsch", PyInvokeKillSch);
// NTSC-U/C: 0x00150b80, PAL: 0x001518d0
const RegisterCFunction kCheatWinFunc("cheat_win", PyInvokeCheatWin);
// NTSC-U/C: 0x00153030, PAL: 0x00153e80
const RegisterCFunction kClockFunc("clock", PyInvokeClock);
// NTSC-U/C: 0x001537d8, PAL: 0x00154668
const RegisterCFunction kDisplayTextFunc("display_text", PyInvokeDisplayText);
// NTSC-U/C: 0x00154030, PAL: 0x00154ee0
const RegisterCFunction kEnableFreestyleFunc("enable_freestyle", PyInvokeEnableFreestyle);
// NTSC-U/C: 0x00154740, PAL: 0x00155610
const RegisterCFunction kFreezeJuiceFunc("freeze_juice", PyInvokeFreezeJuice);
// NTSC-U/C: 0x00157418, PAL: 0x00159048
const RegisterCFunction kMemlogTermFunc("memlog_term", PyInvokeMemlogTerm);
#ifdef VIDEO_STANDARD_PAL
// PAL: 0x00157740
const RegisterCFunction kSetLangFunc("set_lang", PyInvokeSetLang);
const RegisterCFunction kGetLanguageSuffixFunc("get_language_suffix", PyInvokeGetLanguageSuffix);
#endif
// NTSC-U/C: 0x00159248, PAL: 0x0015af28
const RegisterCFunction kSetVolumeFunc("set_volume", PyInvokeSetVolume);
const RegisterCFunction kMidiFunc("midi", PyInvokeMidi);
const RegisterCFunction kStopAllMidiFunc("stop_all_midi", PyInvokeStopAllMidi);
// NTSC-U/C: 0x0015a1c0, PAL: 0x0015bf00
const RegisterCFunction kPostScriptFunc("post_script", PyInvokePostScript);
const RegisterCFunction kCancelCmdFunc("cancel_cmd", PyInvokeCancelCmd);
// NTSC-U/C: 0x0015c2c8, PAL: 0x0015e0a8
const RegisterCFunction kCaptureFunc("capture", PyInvokeCapture);
const RegisterCFunction kRecreateFunc("recreate", PyInvokeRecreate);
// NTSC-U/C: 0x0015c838, PAL: 0x0015e618
const RegisterCFunction kScreenDumpFunc("screen_dump", PyInvokeScreenDump);
// NTSC-U/C: 0x0015d6d8, PAL: 0x0015f4d8
const RegisterCFunction kSelectPowerupFunc("select_powerup", PyInvokeSelectPowerup);
// NTSC-U/C: 0x0015e3c8, PAL: 0x00160228
const RegisterCFunction kSpewFunc("spew", PyInvokeSpew);
// NTSC-U/C: 0x0015e960, PAL: 0x001607e0
const RegisterCFunction kStopGameFunc("stop_game", PyInvokeStopGame);
// NTSC-U/C: 0x0015f048, PAL: 0x00160ee8
const RegisterCFunction kSynthCmdFunc("synth_cmd", PyInvokeSynthCmd);
// NTSC-U/C: 0x0015fc68, PAL: 0x00161b88
const RegisterCFunction kTestFunc("test", PyInvokeTest);
// NTSC-U/C: 0x00162a68, PAL: 0x00164aa8
const RegisterCFunction kTraceFunc("trace", PyInvokeTrace);
// NTSC-U/C: 0x001638d0, PAL: 0x00165980
const RegisterCFunction kZoneDumpFunc("zone_dump", PyInvokeZoneDump);
// NTSC-U/C: 0x0050cbc0, PAL: 0x0054c058
const RegisterCFunction kGetFreqRootFunc("get_freq_root", PyInvokeGetFreqRoot);

} // namespace
