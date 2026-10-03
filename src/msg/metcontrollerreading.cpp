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

// NTSC-U/C: 0x00100f40, PAL: 0x00100f40
void MetControllerReading::Print(std::ostream &stream) const {
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

// NTSC-U/C: 0x00101060, PAL: 0x00101060
OBStream &operator<<(OBStream &stream, const MetControllerReading &reading) {
    int tag = reading.mTag;
    int padIndex = reading.mPadIndex;
    int button = reading.mButton;
    float value = reading.mValue;
    stream.WriteLE(&tag, sizeof(tag))
        .WriteLE(&padIndex, sizeof(padIndex))
        .WriteLE(&button, sizeof(button))
        .WriteLE(&value, sizeof(value));
    return stream;
}

// NTSC-U/C: 0x00101120, PAL: 0x00101120
IBStream &operator>>(IBStream &stream, MetControllerReading &reading) {
    int tag;
    stream.ReadLE(&tag, sizeof(tag))
        .ReadLE(&reading.mPadIndex, sizeof(reading.mPadIndex))
        .ReadLE(&reading.mButton, sizeof(reading.mButton))
        .ReadLE(&reading.mValue, sizeof(reading.mValue));
    reading.mTag = tag;
    return stream;
}
