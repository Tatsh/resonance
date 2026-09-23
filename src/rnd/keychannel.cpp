#include "rnd/keychannel.h"

#include <list>

#include "math/color.h"
#include "math/vector3.h"
#include "os/failsink.h"
#include "rnd/stream.h"

namespace Rnd {

namespace {

// 0x004d8c88
FailSink &DumpColorKey(FailSink &sink, const ColorKey &key) {
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

// 0x004d9658
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
// 0x004d9098
Stream &WriteColorKey(Stream &stream, const ColorKey &key) {
    stream.Write(&key.mValue.r, sizeof(float));
    stream.Write(&key.mValue.g, sizeof(float));
    stream.Write(&key.mValue.b, sizeof(float));
    stream.Write(&key.mValue.a, sizeof(float));
    stream.Write(&key.mFrame, sizeof(key.mFrame));
    return stream;
}

// 0x004da880
FailSink &DumpVector3Key(FailSink &sink, const Vector3Key &key) {
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

// 0x004d8de8
FailSink &DumpColorKeys(FailSink &sink, const std::list<ColorKey> &keys) {
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

// 0x004d8f08
FailSink &DumpFloatKeys(FailSink &sink, const std::list<FloatKey> &keys) {
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

// 0x004dd9b0
Stream &ReadColorKeys(Stream &stream, std::list<ColorKey> &keys) {
    int nCount = 0;
    stream.Read(&nCount, sizeof(nCount));
    keys.resize(nCount);
    for (auto &key : keys) {
        ReadColorKey(stream, key);
    }
    return stream;
}

// 0x004d9180
Stream &WriteColorKeys(Stream &stream, const std::list<ColorKey> &keys) {
    const int nCount = keys.size();
    stream.Write(&nCount, sizeof(nCount));
    for (const auto &key : keys) {
        WriteColorKey(stream, key);
    }
    return stream;
}

// 0x004d9238
Stream &WriteFloatKeys(Stream &stream, const std::list<FloatKey> &keys) {
    const int nCount = keys.size();
    stream.Write(&nCount, sizeof(nCount));
    for (const auto &key : keys) {
        stream.Write(&key.mValue, sizeof(key.mValue));
        stream.Write(&key.mFrame, sizeof(key.mFrame));
    }
    return stream;
}

// 0x004d98d8
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

// 0x004da9b0
FailSink &DumpVector3Keys(FailSink &sink, const std::list<Vector3Key> &keys) {
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

// 0x004db3d0
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

// 0x004dac80
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
