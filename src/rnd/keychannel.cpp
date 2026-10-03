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
    stream.Read(&key.mValue.r, sizeof(float));
    stream.Read(&key.mValue.g, sizeof(float));
    stream.Read(&key.mValue.b, sizeof(float));
    stream.Read(&key.mValue.a, sizeof(float));
    stream.Read(&key.mFrame, sizeof(key.mFrame));
    return stream;
}

// Each component reaches the stream as a stack copy rather than as the address of the member, so
// the original moved every float through a by-value parameter.
// NTSC-U/C: 0x004d9098, PAL: 0x005175b0
Stream &WriteColorKey(Stream &stream, const ColorKey &key) {
    stream.Write(&key.mValue.r, sizeof(float));
    stream.Write(&key.mValue.g, sizeof(float));
    stream.Write(&key.mValue.b, sizeof(float));
    stream.Write(&key.mValue.a, sizeof(float));
    stream.Write(&key.mFrame, sizeof(key.mFrame));
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

// NTSC-U/C: 0x004d8de8, PAL: 0x00517300
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

// NTSC-U/C: 0x004d8f08, PAL: 0x00517420
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

// NTSC-U/C: 0x004dd9b0, PAL: 0x0051bf68
Stream &ReadColorKeys(Stream &stream, std::list<ColorKey> &keys) {
    int nCount = 0;
    stream.Read(&nCount, sizeof(nCount));
    keys.resize(nCount);
    for (auto &key : keys) {
        ReadColorKey(stream, key);
    }
    return stream;
}

// NTSC-U/C: 0x004d9180, PAL: 0x00517698
Stream &WriteColorKeys(Stream &stream, const std::list<ColorKey> &keys) {
    const int nCount = keys.size();
    stream.Write(&nCount, sizeof(nCount));
    for (const auto &key : keys) {
        WriteColorKey(stream, key);
    }
    return stream;
}

// NTSC-U/C: 0x004d9238, PAL: 0x00517750
Stream &WriteFloatKeys(Stream &stream, const std::list<FloatKey> &keys) {
    const int nCount = keys.size();
    stream.Write(&nCount, sizeof(nCount));
    for (const auto &key : keys) {
        stream.Write(&key.mValue, sizeof(key.mValue));
        stream.Write(&key.mFrame, sizeof(key.mFrame));
    }
    return stream;
}

// NTSC-U/C: 0x004d98d8, PAL: 0x00517df0
Stream &ReadFloatKeys(Stream &stream, std::list<FloatKey> &keys) {
    int nCount = 0;
    stream.Read(&nCount, sizeof(nCount));
    keys.resize(nCount);
    for (auto &key : keys) {
        stream.Read(&key.mValue, sizeof(key.mValue));
        stream.Read(&key.mFrame, sizeof(key.mFrame));
    }
    return stream;
}

// NTSC-U/C: 0x004da9b0, PAL: 0x00518ec8
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

// NTSC-U/C: 0x004db3d0, PAL: 0x00519918
Stream &ReadVector3Keys(Stream &stream, std::list<Vector3Key> &keys) {
    int nCount = 0;
    stream.Read(&nCount, sizeof(nCount));
    keys.resize(nCount);
    for (auto &key : keys) {
        stream.Read(&key.mValue.x, sizeof(float));
        stream.Read(&key.mValue.y, sizeof(float));
        stream.Read(&key.mValue.z, sizeof(float));
        stream.Read(&key.mFrame, sizeof(key.mFrame));
    }
    return stream;
}

// NTSC-U/C: 0x004dac80, PAL: 0x00519198
Stream &WriteVector3Keys(Stream &stream, const std::list<Vector3Key> &keys) {
    const int nCount = keys.size();
    stream.Write(&nCount, sizeof(nCount));
    for (const auto &key : keys) {
        stream.Write(&key.mValue.x, sizeof(float));
        stream.Write(&key.mValue.y, sizeof(float));
        stream.Write(&key.mValue.z, sizeof(float));
        stream.Write(&key.mFrame, sizeof(key.mFrame));
    }
    return stream;
}

} // namespace Rnd
