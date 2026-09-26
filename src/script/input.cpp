#include <cstring>
#include <sstream>
#include <string>

#include "app/application.h"
#include "app/globals.h"
#include "game/enablemgr.h"
#include "game/gamemanagerimpl.h"
#include "game/gamer.h"
#include "game/grooveworld.h"
#include "game/inputmap.h"
#include "os/formatstring.h"
#include "os/hxstr.h"
#include "script/cxx/config.h"
#include "script/cxx/int.h"
#include "script/cxx/object.h"
#include "script/cxx/string.h"
#include "script/cxx/tuple.h"
#include "script/cxx/typeerror.h"

namespace {

// Device tags, matching the kTagNone, kTagMouse, kTagJoystick, and kTagKeyboard word values.
constexpr int kDeviceNone = 0x6e6f6e65;     // 'none'
constexpr int kDeviceMouse = 0x6d6f7573;    // 'mous'
constexpr int kDeviceJoystick = 0x6a6f7920; // 'joy '
constexpr int kDeviceKeyboard = 0x6b657920; // 'key '

// The pitch-riff event, whose binding carries an action argument.
constexpr int kEventPitchRiff = 0x72706368; // 'rpch'

// Parsed device token. The image threads a stack record with the tag at +0x0 and error bits at
// +0x1a; a failed parse throws before the caller reads anything back, so the record is a tag
// here.
struct InputDeviceDesc {
    int mTag;
};

// Parsed event token. The image holds the action argument at +0x8 of the same stack record
// shape as the device token.
struct InputEventDesc {
    int mTag;
    int mExtra;
};

// Parse a device name into its tag.
//
// Accepts none, mouse, joy, and key. An empty string or anything else throws TypeError. The
// image extracts the word through an istrstream and tests the stream state.
// 0x00154968
void InputParseDevice(const HxStr &text, InputDeviceDesc *pDesc) {
    std::istringstream stream(text.mStr != nullptr ? text.mStr : "");
    std::string word;
    if (!(stream >> word)) {
        throw Py::TypeError(HxStr(FormatString("cannot parse Device Type from:")));
    }
    if (word == "none") {
        pDesc->mTag = kDeviceNone;
    } else if (word == "mouse") {
        pDesc->mTag = kDeviceMouse;
    } else if (word == "joy") {
        pDesc->mTag = kDeviceJoystick;
    } else if (word == "key") {
        pDesc->mTag = kDeviceKeyboard;
    } else {
        throw Py::TypeError(HxStr(FormatString("cannot parse Device Type from:")));
    }
}

// Parse an event name into its four-character tag.
//
// Reads four characters; a short read throws TypeError.
// 0x00154b30
void InputParseEvent(const HxStr &text, InputEventDesc *pDesc) {
    std::istringstream stream(text.mStr != nullptr ? text.mStr : "");
    char tag[4];
    if (!(stream >> tag[0] >> tag[1] >> tag[2] >> tag[3])) {
        throw Py::TypeError(HxStr(FormatString("bad Event Type")));
    }
    std::memcpy(&pDesc->mTag, tag, sizeof(tag));
}

// Bind a control, or toggle one slot and action.
//
// The tuple carries the command, the slot, and the event text. The add command further carries
// the action argument of a pitch-riff event, the device text, the port, and the button; other
// events leave the argument zero. Disable and enable switch the slot and action off and on.
// 0x00154d00
Py::Object ScriptInput(const Py::Tuple &args) {
    InputMap *pMap = InputMap::shared();
    if (args.length() < 3) {
        throw Py::TypeError(HxStr(FormatString("wrong # args for input")));
    }
    Py::Object commandElement = args.getItem(0);
    Py::String commandText(commandElement);
    HxStr command = commandText;
    const long nSlot = Py::Int(args.getItem(1));
    Py::Object eventElement = args.getItem(2);
    Py::String eventString(eventElement);
    HxStr eventName = eventString;
    InputEventDesc event{};
    InputParseEvent(eventName, &event);
    if (command == "add") {
        if (event.mTag == kEventPitchRiff) {
            const long nExtra = Py::Int(args.getItem(3));
            event.mExtra = static_cast<int>(nExtra);
        }
        Py::Object deviceElement = args.getItem(4);
        Py::String deviceText(deviceElement);
        HxStr deviceName = deviceText;
        InputDeviceDesc device{};
        InputParseDevice(deviceName, &device);
        const long nPort = Py::Int(args.getItem(5));
        const long nButton = Py::Int(args.getItem(6));
        pMap->AddBinding(device.mTag,
                         static_cast<int>(nPort),
                         static_cast<int>(nButton),
                         static_cast<int>(nSlot),
                         event.mTag,
                         event.mExtra);
        return Py::Object();
    }
    if (command == "disable") {
        pMap->SetEnabled(static_cast<int>(nSlot), event.mTag, 0);
        return Py::Object();
    }
    if (command == "enable") {
        pMap->SetEnabled(static_cast<int>(nSlot), event.mTag, 1);
        return Py::Object();
    }
    throw Py::TypeError(HxStr(FormatString("bad first arg to input")));
}

// Run ScriptInput() on the interpreter's argument tuple.
// 0x00155950
PyObject *PyInvokeInput(PyObject *, PyObject *pArgs) {
    try {
        Py::Tuple args(pArgs);
        Py::Object result = ScriptInput(args);
        return Py::new_reference_to(result);
    } catch (Py::Exception &) {
        return nullptr;
    }
}

// Enable or disable one track outside jam.
//
// The tuple carries the command and the track. In a jam session, or without an enable policy,
// it does nothing. Enable frees the track from bar zero through bar -1, which never arrives,
// so the track stays enabled.
// 0x00162c90
Py::Object ScriptTrackCtrl(const Py::Tuple &args) {
    if (args.length() != 2) {
        throw Py::TypeError(HxStr(FormatString("wrong # args for input")));
    }
    Py::Object commandElement = args.getItem(0);
    Py::String commandText(commandElement);
    HxStr command = commandText;
    const long nTrack = Py::Int(args.getItem(1));
    GrooveWorld *pWorld = Application::shared()->GetWorld();
    if (pWorld == nullptr) {
        return Py::Object();
    }
    if (Application::shared()->GetPlayMode() == kPlayModeJam) {
        return Py::Object();
    }
    EnableMgr *pPolicy = pWorld->mGamer->mEnableMgr;
    if (pPolicy == nullptr) {
        return Py::Object();
    }
    if (command == "enable") {
        pPolicy->SetFreeUntil(static_cast<int>(nTrack), 0, -1);
        return Py::Object();
    }
    if (command == "disable") {
        pPolicy->DisableTrack(static_cast<int>(nTrack));
        return Py::Object();
    }
    return Py::Object();
}

// Run ScriptTrackCtrl() on the interpreter's argument tuple.
// 0x001631f0
PyObject *PyInvokeTrackCtrl(PyObject *, PyObject *pArgs) {
    try {
        Py::Tuple args(pArgs);
        Py::Object result = ScriptTrackCtrl(args);
        return Py::new_reference_to(result);
    } catch (Py::Exception &) {
        return nullptr;
    }
}

} // namespace
