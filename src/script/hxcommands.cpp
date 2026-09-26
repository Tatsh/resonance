#include <cstring>
#include <sstream>

#include "app/application.h"
#include "app/attachment.h"
#include "app/globals.h"
#include "game/gamemanagerimpl.h"
#include "game/gamer.h"
#include "game/grooveworld.h"
#include "game/localplayer.h"
#include "game/powerupcollection.h"
#include "game/scoretrackgraph.h"
#include "game/trackdata.h"
#include "gfx/vramtable.h"
#include "mid/mbt.h"
#include "os/filelog.h"
#include "os/formatstring.h"
#include "os/heap.h"
#include "os/hostmode.h"
#include "os/hxstr.h"
#include "os/log.h"
#include "os/mem.h"
#include "os/seccache.h"
#include "sch/cmdid.h"
#include "sch/command.h"
#include "sch/tickclock.h"
#include "script/cxx/config.h"
#include "script/cxx/int.h"
#include "script/cxx/nameerror.h"
#include "script/cxx/object.h"
#include "script/cxx/string.h"
#include "script/cxx/tuple.h"
#include "script/scriptcmd.h"
#include "script/testregistry.h"
#include "synth/midi_main.h"
#include "synth/ps2hardsynth.h"

namespace {

// Calls the memlog terminator has made, which names it. The name is inferred.
// 0x00676750
int g_nMemlogTermCalls = 0;

// Report the frequency root.
// 0x00506f50
Py::Object ScriptGetFreqRoot([[maybe_unused]] Py::Tuple args) {
    HxStr root = GetFreqRoot();
    Py::String text(root);
    return text;
}

// Run ScriptGetFreqRoot() on the interpreter's argument tuple.
// 0x00507138
PyObject *PyInvokeGetFreqRoot(PyObject *, PyObject *pArgs) {
    try {
        Py::Tuple args(pArgs);
        Py::Object result = ScriptGetFreqRoot(args);
        return Py::new_reference_to(result);
    } catch (Py::Exception &) {
        return nullptr;
    }
}

// Shut the memory, heap, and file loggers down.
//
// Dumps the sector cache and the heap log, logs the shutdown, closes the memory log and the
// file log, and writes the Python heap's statistics and contents to heapdump.txt.
// 0x00155dc0
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
// 0x00155f08
PyObject *PyInvokeMemlogTerm(PyObject *, PyObject *pArgs) {
    try {
        Py::Tuple args(pArgs);
        Py::Object result = ScriptMemlogTerm(args);
        return Py::new_reference_to(result);
    } catch (Py::Exception &) {
        return nullptr;
    }
}

// Stop the game.
//
// Takes no arguments.
// 0x0015e5f0
Py::Object ScriptStopGame(Py::Tuple args) {
    if (args.length() != 0) {
        throw Py::TypeError(HxStr(FormatString("requires 0 args")));
    }
    Application::shared()->GetWorld()->PostExitMode1();
    return Py::Object();
}

// Run ScriptStopGame() on the interpreter's argument tuple.
// 0x0015e718
PyObject *PyInvokeStopGame(PyObject *, PyObject *pArgs) {
    try {
        Py::Tuple args(pArgs);
        Py::Object result = ScriptStopGame(args);
        return Py::new_reference_to(result);
    } catch (Py::Exception &) {
        return nullptr;
    }
}

// Show a line of text on the display.
// 0x001532d8
Py::Object ScriptDisplayText(const Py::Tuple &args) {
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
// 0x00153590
PyObject *PyInvokeDisplayText(PyObject *, PyObject *pArgs) {
    try {
        Py::Tuple args(pArgs);
        Py::Object result = ScriptDisplayText(args);
        return Py::new_reference_to(result);
    } catch (Py::Exception &) {
        return nullptr;
    }
}

// Report a value on the screen.
//
// Shows the string form with a newline for fifty ticks.
// 0x001611f8
Py::Object ScriptTrace(const Py::Tuple &args) {
    if (args.length() != 1) {
        throw Py::TypeError(HxStr(FormatString("requires 1 arg")));
    }
    HxStr text = args.getItem(0).as_string();
    text += '\n';
    ShowReportedMessage(text, 50);
    return Py::Object();
}

// Run ScriptTrace() on the interpreter's argument tuple.
// 0x001614c0
PyObject *PyInvokeTrace(PyObject *, PyObject *pArgs) {
    try {
        Py::Tuple args(pArgs);
        Py::Object result = ScriptTrace(args);
        return Py::new_reference_to(result);
    } catch (Py::Exception &) {
        return nullptr;
    }
}

// Withdraw a scheduled command.
//
// The tuple carries the command handle.
// 0x00159a90
Py::Object ScriptCancelCmd(const Py::Tuple &args) {
    if (args.length() != 1) {
        throw Py::TypeError(HxStr(FormatString("requres 1 arg: cmdId")));
    }
    const long nId = Py::Int(args.getItem(0));
    CmdID id;
    id.mValue = static_cast<int>(nId);
    Application::shared()->GetSongClock()->Withdraw(id);
    return Py::Object();
}

// Run ScriptCancelCmd() on the interpreter's argument tuple.
// 0x00159f78
PyObject *PyInvokeCancelCmd(PyObject *, PyObject *pArgs) {
    try {
        Py::Tuple args(pArgs);
        Py::Object result = ScriptCancelCmd(args);
        return Py::new_reference_to(result);
    } catch (Py::Exception &) {
        return nullptr;
    }
}

// Freeze the juice.
//
// A non-zero value stops juice changes.
// 0x00154258
Py::Object ScriptFreezeJuice(const Py::Tuple &args) {
    if (args.length() != 1) {
        throw Py::TypeError(HxStr(FormatString("wrong # args for freeze_juice")));
    }
    const long nFreeze = Py::Int(args.getItem(0));
    GrooveWorld *pWorld = Application::shared()->GetWorld();
    if (pWorld != nullptr) {
        pWorld->mGamer->mUnknown1c = nFreeze != 0 ? 1 : 0;
    }
    return Py::Object();
}

// Run ScriptFreezeJuice() on the interpreter's argument tuple.
// 0x001544f8
PyObject *PyInvokeFreezeJuice(PyObject *, PyObject *pArgs) {
    try {
        Py::Tuple args(pArgs);
        Py::Object result = ScriptFreezeJuice(args);
        return Py::new_reference_to(result);
    } catch (Py::Exception &) {
        return nullptr;
    }
}

// Recreate the session from a recording.
//
// A second argument sets the playback flag.
// 0x0015bb00
Py::Object ScriptRecreate(const Py::Tuple &args) {
    if (args.length() <= 0) {
        throw Py::TypeError(HxStr(FormatString("requres arg: filename [ignore-autoexec]")));
    }
    Py::Object element = args.getItem(0);
    Py::String text(element);
    HxStr filename = text;
    Application::shared()->GetGameManager()->StartPlayback(filename, args.length() == 2);
    return Py::Object();
}

// Run ScriptRecreate() on the interpreter's argument tuple.
// 0x0015c080
PyObject *PyInvokeRecreate(PyObject *, PyObject *pArgs) {
    try {
        Py::Tuple args(pArgs);
        Py::Object result = ScriptRecreate(args);
        return Py::new_reference_to(result);
    } catch (Py::Exception &) {
        return nullptr;
    }
}

// Open a freestyle span.
//
// 0x00153a00
Py::Object ScriptEnableFreestyle(const Py::Tuple &args) {
    if (args.length() != 2) {
        throw Py::TypeError(HxStr(FormatString("wrong # args for enable_freestyle")));
    }
    const long nStartBar = Py::Int(args.getItem(0));
    const long nEndBar = Py::Int(args.getItem(1));
    Application::shared()->GetWorld()->mGamer->EnablePlayerFreestyle(static_cast<int>(nStartBar),
                                                                     static_cast<int>(nEndBar));
    return Py::Object();
}

// Run ScriptEnableFreestyle() on the interpreter's argument tuple.
// 0x00153de8
PyObject *PyInvokeEnableFreestyle(PyObject *, PyObject *pArgs) {
    try {
        Py::Tuple args(pArgs);
        Py::Object result = ScriptEnableFreestyle(args);
        return Py::new_reference_to(result);
    } catch (Py::Exception &) {
        return nullptr;
    }
}

// Select a powerup by index.
// 0x0015c998
Py::Object ScriptSelectPowerup(const Py::Tuple &args) {
    if (args.length() != 1) {
        throw Py::TypeError(HxStr(FormatString("wrong # args for select_powerup")));
    }
    const long nPowerup = Py::Int(args.getItem(0));
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
// 0x0015ccb8
PyObject *PyInvokeSelectPowerup(PyObject *, PyObject *pArgs) {
    try {
        Py::Tuple args(pArgs);
        Py::Object result = ScriptSelectPowerup(args);
        return Py::new_reference_to(result);
    } catch (Py::Exception &) {
        return nullptr;
    }
}

// Post script text to run at a song position.
//
// Returns the posted command's handle. The tuple carries the position and the text.
// 0x00159538
Py::Object ScriptPostScript(const Py::Tuple &args) {
    if (args.length() != 2) {
        throw Py::TypeError(HxStr(FormatString("requres 2 arg: tick, script")));
    }
    const long nTick = Py::Int(args.getItem(0));
    Py::Object element = args.getItem(1);
    Py::String text(element);
    HxStr script = text;
    IsFiniteMBT(static_cast<int>(nTick));
    Sch::Command *pCommand = NewScriptCmd(script);
    CmdID id;
    Application::shared()->GetSongClock()->PostAtSongTick(pCommand, nTick, id);
    Attachment::ReleaseIfSet(pCommand);
    return Py::Int(static_cast<long>(id.mValue));
}

// Run ScriptPostScript() on the interpreter's argument tuple.
// 0x00159d30
PyObject *PyInvokePostScript(PyObject *, PyObject *pArgs) {
    try {
        Py::Tuple args(pArgs);
        Py::Object result = ScriptPostScript(args);
        return Py::new_reference_to(result);
    } catch (Py::Exception &) {
        return nullptr;
    }
}

// Write the display buffer to a scrndump file.
// 0x0015c580
Py::Object ScriptScreenDump([[maybe_unused]] Py::Tuple args) {
    g_vramTable.Screendump("scrndump");
    return Py::Object();
}

// Run ScriptScreenDump() on the interpreter's argument tuple.
// 0x0015c688
PyObject *PyInvokeScreenDump(PyObject *, PyObject *pArgs) {
    try {
        Py::Tuple args(pArgs);
        Py::Object result = ScriptScreenDump(args);
        return Py::new_reference_to(result);
    } catch (Py::Exception &) {
        return nullptr;
    }
}

// Run a synthesiser command.
//
// Takes any positive argument count.
// 0x0015eb88
Py::Object ScriptSynthCmd(const Py::Tuple &args) {
    if (args.length() <= 0) {
        throw Py::TypeError(HxStr(FormatString("wrong # args for synth_info")));
    }
    const long nCommand = Py::Int(args.getItem(0));
    SynthCommand(static_cast<int>(nCommand));
    return Py::Object();
}

// Run ScriptSynthCmd() on the interpreter's argument tuple.
// 0x0015ee00
PyObject *PyInvokeSynthCmd(PyObject *, PyObject *pArgs) {
    try {
        Py::Tuple args(pArgs);
        Py::Object result = ScriptSynthCmd(args);
        return Py::new_reference_to(result);
    } catch (Py::Exception &) {
        return nullptr;
    }
}

// Set a track's MIDI volume.
//
// The name is score or bg, although both select the same path. The volume goes out as
// controller 7 on the track's channel.
// 0x00158060
Py::Object ScriptSetVolume(const Py::Tuple &args) {
    if (args.length() != 3) {
        throw Py::TypeError(HxStr(FormatString("requires 3 args")));
    }
    Py::Object element = args.getItem(0);
    Py::String text(element);
    HxStr name = text;
    const long nTrack = Py::Int(args.getItem(1));
    const long nVolume = Py::Int(args.getItem(2));
    if (name == "score" || name == "bg") {
        GrooveWorld *pWorld = Application::shared()->GetWorld();
        const int nChannel = pWorld->mTrackGraphs[nTrack]->mTrackData->mChannel;
        Application::shared()->GetSynth()->SendMidi(nChannel | 0xB0, 7, nVolume & 0xFF);
    }
    return Py::Object();
}

// Run ScriptSetVolume() on the interpreter's argument tuple.
// 0x00158660
PyObject *PyInvokeSetVolume(PyObject *, PyObject *pArgs) {
    try {
        Py::Tuple args(pArgs);
        Py::Object result = ScriptSetVolume(args);
        return Py::new_reference_to(result);
    } catch (Py::Exception &) {
        return nullptr;
    }
}

// Send MIDI messages.
//
// The tuple carries a device name, a command, and four values. Voice and fx send bank selects
// on every channel while the last value is 1, play sends a note on, stop a note off, and
// bank_swap moves the stream. The channel nibble reads 0 where it is consumed: voice and fx
// never reach play or stop with their values, so the status is always channel 0 there.
// 0x00157578
Py::Object ScriptMidi(const Py::Tuple &args) {
    if (args.length() != 6) {
        throw Py::TypeError(HxStr(FormatString("requires 6 args")));
    }
    Py::Object deviceElement = args.getItem(0);
    Py::String deviceText(deviceElement);
    HxStr device = deviceText;
    Py::Object commandElement = args.getItem(1);
    Py::String commandText(commandElement);
    HxStr command = commandText;
    const long nA = Py::Int(args.getItem(2));
    const long nB = Py::Int(args.getItem(3));
    const long nC = Py::Int(args.getItem(4));
    const long nD = Py::Int(args.getItem(5));
    int nChannel = 0;
    if (command == "voice") {
        nChannel = 0xE;
    } else if (command == "fx") {
        nChannel = 0xF;
    }
    if ((command == "voice" || command == "fx") && nD == 1) {
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
// 0x001588a8
PyObject *PyInvokeMidi(PyObject *, PyObject *pArgs) {
    try {
        Py::Tuple args(pArgs);
        Py::Object result = ScriptMidi(args);
        return Py::new_reference_to(result);
    } catch (Py::Exception &) {
        return nullptr;
    }
}

// Run a named self-test.
//
// Reports `ok` for a passing test and `not ok` otherwise. An unknown name throws NameError
// over the valid test list.
// 0x0015f270
Py::Object ScriptTest(const Py::Tuple &args) {
    if (args.length() <= 0) {
        throw Py::TypeError(HxStr(FormatString("requires 1 arg (name of test)")));
    }
    HxStr name = args.getItem(0).as_string();
    for (int i = 0; i < TestRegistry::sTestCount; ++i) {
        if (std::strcmp(TestRegistry::sTests[i].mName, name.mStr) == 0) {
            const int nPassed = TestRegistry::sTests[i].mFunc();
            return Py::String(HxStr(nPassed != 0 ? "ok" : "not ok"));
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
// 0x0015fa20
PyObject *PyInvokeTest(PyObject *, PyObject *pArgs) {
    try {
        Py::Tuple args(pArgs);
        Py::Object result = ScriptTest(args);
        return Py::new_reference_to(result);
    } catch (Py::Exception &) {
        return nullptr;
    }
}

} // namespace
