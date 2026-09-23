#include "msg/metcontrollerreading.h"

#include <iostream>

#include "stream/ibstream.h"
#include "stream/obstream.h"

namespace {

// The four device tags, each the value the compiler gives the four-character literal.
constexpr int kTagJoystick = 0x6a6f7920; // 'joy '
constexpr int kTagKeyboard = 0x6b657920; // 'key '
constexpr int kTagMouse = 0x6d6f7573;    // 'mous'
constexpr int kTagNone = 0x6e6f6e65;     // 'none'

} // namespace

// 0x00100f40
void MetControllerReading::Print(std::ostream &stream) {
    switch (mTag) {
    case kTagKeyboard:
        stream << "key";
        break;
    case kTagMouse:
        stream << "mouse";
        break;
    case kTagNone:
        stream << "none";
        break;
    case kTagJoystick:
        stream << "joy";
        break;
    }
    stream << '.' << mPadIndex << '.' << mButton << ':' << mValue;
}

// 0x00101060
OBStream &operator<<(OBStream &stream, const MetControllerReading &reading) {
    int tag = reading.mTag;
    int padIndex = reading.mPadIndex;
    int button = reading.mButton;
    float value = reading.mValue;
    stream.Write(&tag, sizeof(tag))
        .Write(&padIndex, sizeof(padIndex))
        .Write(&button, sizeof(button))
        .Write(&value, sizeof(value));
    return stream;
}

// 0x00101120
IBStream &operator>>(IBStream &stream, MetControllerReading &reading) {
    int tag;
    stream.Read(&tag, sizeof(tag))
        .Read(&reading.mPadIndex, sizeof(reading.mPadIndex))
        .Read(&reading.mButton, sizeof(reading.mButton))
        .Read(&reading.mValue, sizeof(reading.mValue));
    reading.mTag = tag;
    return stream;
}
