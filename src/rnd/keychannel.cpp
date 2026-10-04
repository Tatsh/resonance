#include "rnd/keychannel.h"

#include <list>

#include "math/color.h"
#include "math/vector3.h"
#include "os/dbg.h"
#include "rnd/stream.h"

namespace Rnd {

namespace {

// NTSC-U/C: 0x004d8c88, PAL: 0x005171a0
Dbg &DumpColorKey(Dbg &sink, const ColorKey &key) {
    sink.Print("(frame:");
    sink.Format("%.2f", key.mFrame);
    sink.Print(" value:");
    sink.Print("(r:");
    sink.Format("%.2f", key.mValue.r);
    sink.Print(" g:");
    sink.Format("%.2f", key.mValue.g);
    sink.Print(" b:");
    sink.Format("%.2f", key.mValue.b);
    sink.Print(" a:");
    sink.Format("%.2f", key.mValue.a);
    sink.Print(")");
    sink.Print(")");
    return sink;
}

// NTSC-U/C: 0x004d9658, PAL: 0x00517b70
Stream &ReadColorKey(Stream &stream, ColorKey &key) {
    stream.ReadLE(&key.mValue.r, sizeof(float));
    stream.ReadLE(&key.mValue.g, sizeof(float));
    stream.ReadLE(&key.mValue.b, sizeof(float));
    stream.ReadLE(&key.mValue.a, sizeof(float));
    stream.ReadLE(&key.mFrame, sizeof(key.mFrame));
    return stream;
}

// Each component reaches the stream as a stack copy rather than as the address of the member, so
// the original moved every float through a by-value parameter.
// NTSC-U/C: 0x004d9098, PAL: 0x005175b0
Stream &WriteColorKey(Stream &stream, const ColorKey &key) {
    stream.WriteLE(&key.mValue.r, sizeof(float));
    stream.WriteLE(&key.mValue.g, sizeof(float));
    stream.WriteLE(&key.mValue.b, sizeof(float));
    stream.WriteLE(&key.mValue.a, sizeof(float));
    stream.WriteLE(&key.mFrame, sizeof(key.mFrame));
    return stream;
}

// NTSC-U/C: 0x004da880, PAL: 0x00518d98
Dbg &DumpVector3Key(Dbg &sink, const Vector3Key &key) {
    sink.Print("(frame:");
    sink.Format("%.2f", key.mFrame);
    sink.Print(" value:");
    sink.Print("(x:");
    sink.Format("%.2f", key.mValue.x);
    sink.Print(" y:");
    sink.Format("%.2f", key.mValue.y);
    sink.Print(" z:");
    sink.Format("%.2f", key.mValue.z);
    sink.Print(")");
    sink.Print(")");
    return sink;
}

} // namespace

Dbg &DumpColorKeys(Dbg &sink, const std::list<ColorKey> &keys) {
    sink.Print("(size:");
    sink.Format("%u", keys.size());
    sink.Print(")");

    unsigned nIndex = 0;
    for (const auto &key : keys) {
        sink.Print("\n");
        sink.Format("%d", nIndex);
        sink.Print("\t");
        ++nIndex;
        DumpColorKey(sink, key);
    }
    return sink;
}

Dbg &DumpFloatKeys(Dbg &sink, const std::list<FloatKey> &keys) {
    sink.Print("(size:");
    sink.Format("%u", keys.size());
    sink.Print(")");

    unsigned nIndex = 0;
    for (const auto &key : keys) {
        sink.Print("\n");
        sink.Format("%d", nIndex);
        sink.Print("\t");
        ++nIndex;
        sink.Print("(frame:");
        sink.Format("%.2f", key.mFrame);
        sink.Print(" value:");
        sink.Format("%.2f", key.mValue);
        sink.Print(")");
    }
    return sink;
}

Stream &ReadColorKeys(Stream &stream, std::list<ColorKey> &keys) {
    int nCount = 0;
    stream.ReadLE(&nCount, sizeof(nCount));
    keys.resize(nCount);
    for (auto &key : keys) {
        ReadColorKey(stream, key);
    }
    return stream;
}

Stream &WriteColorKeys(Stream &stream, const std::list<ColorKey> &keys) {
    const int nCount = keys.size();
    stream.WriteLE(&nCount, sizeof(nCount));
    for (const auto &key : keys) {
        WriteColorKey(stream, key);
    }
    return stream;
}

Stream &WriteFloatKeys(Stream &stream, const std::list<FloatKey> &keys) {
    const int nCount = keys.size();
    stream.WriteLE(&nCount, sizeof(nCount));
    for (const auto &key : keys) {
        stream.WriteLE(&key.mValue, sizeof(key.mValue));
        stream.WriteLE(&key.mFrame, sizeof(key.mFrame));
    }
    return stream;
}

Stream &ReadFloatKeys(Stream &stream, std::list<FloatKey> &keys) {
    int nCount = 0;
    stream.ReadLE(&nCount, sizeof(nCount));
    keys.resize(nCount);
    for (auto &key : keys) {
        stream.ReadLE(&key.mValue, sizeof(key.mValue));
        stream.ReadLE(&key.mFrame, sizeof(key.mFrame));
    }
    return stream;
}

Dbg &DumpVector3Keys(Dbg &sink, const std::list<Vector3Key> &keys) {
    sink.Print("(size:");
    sink.Format("%u", keys.size());
    sink.Print(")");

    unsigned nIndex = 0;
    for (const auto &key : keys) {
        sink.Print("\n");
        sink.Format("%d", nIndex);
        sink.Print("\t");
        ++nIndex;
        DumpVector3Key(sink, key);
    }
    return sink;
}

Stream &ReadVector3Keys(Stream &stream, std::list<Vector3Key> &keys) {
    int nCount = 0;
    stream.ReadLE(&nCount, sizeof(nCount));
    keys.resize(nCount);
    for (auto &key : keys) {
        stream.ReadLE(&key.mValue.x, sizeof(float));
        stream.ReadLE(&key.mValue.y, sizeof(float));
        stream.ReadLE(&key.mValue.z, sizeof(float));
        stream.ReadLE(&key.mFrame, sizeof(key.mFrame));
    }
    return stream;
}

Stream &operator<<(Stream &stream, const std::list<Vector3Key> &keys) {
    const int nCount = keys.size();
    stream.WriteLE(&nCount, sizeof(nCount));
    for (const auto &key : keys) {
        stream.WriteLE(&key.mValue.x, sizeof(float));
        stream.WriteLE(&key.mValue.y, sizeof(float));
        stream.WriteLE(&key.mValue.z, sizeof(float));
        stream.WriteLE(&key.mFrame, sizeof(key.mFrame));
    }
    return stream;
}

} // namespace Rnd
