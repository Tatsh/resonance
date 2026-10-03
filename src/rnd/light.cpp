#include "rnd/light.h"

#include "math/color.h"
#include "os/dbg.h"
#include "os/hxstr.h"
#include "os/mem.h"
#include "rnd/manager.h"
#include "rnd/object.h"
#include "rnd/stream.h"
#include "rnd/transformable.h"

namespace Rnd {

namespace {

// Revision Save() writes and the highest revision Load() accepts.
constexpr int kLightRevision = 1;

// First revision that stores the type word. A revision of 0 predates it.
constexpr int kLightTypeRevision = 1;

// The allocation tag every light block is billed to.
constexpr char kLightTag[] = "Rnd::Light";

// Defaults the constructor gives the cone and the reach. The angle is one ulp short of pi / 4.
constexpr float kDefaultConeAngle = 0.785398126f;
constexpr float kDefaultRange = 1000.0f;

// Grey level of the default ambient colour.
constexpr float kDefaultAmbientLevel = 0.1f;

// src/rnd/mat.cpp declares a helper of the same title file-locally for the same reason. Every
// component is a separate Print and Format pair, which is why the two literals "(r:" and "%.2f"
// survive apart in the read-only data.
void PrintColor(Dbg &sink, const Color &color) {
    sink.Print("(r:");
    sink.Format("%.2f", color.r);
    sink.Print(" g:");
    sink.Format("%.2f", color.g);
    sink.Print(" b:");
    sink.Format("%.2f", color.b);
    sink.Print(" a:");
    sink.Format("%.2f", color.a);
    sink.Print(")");
}

// NTSC-U/C: 0x005454a0, PAL: 0x00585218
Dbg &operator<<(Dbg &sink, Light::Type type) {
    switch (type) {
    case Light::kLightTypePoint:
        sink.Print("Point");
        break;
    case Light::kLightTypeDirectional:
        sink.Print("Directional");
        break;
    case Light::kLightTypeSpot:
        sink.Print("Spot");
        break;
    }
    return sink;
}

} // namespace

// NTSC-U/C: 0x00720bd0, PAL: 0x00764660
HxStr g_lightClassName("Light");

// NTSC-U/C: 0x005445c8, PAL: 0x00584340
const HxStr &Light::ClassName() const {
    return g_lightClassName;
}

// NTSC-U/C: 0x0053ffd8, PAL: 0x0057fcb0
Light::Light(const HxStr &name)
    : Object(name), mDiffuse{1.0f, 1.0f, 1.0f, 1.0f},
      mAmbient{kDefaultAmbientLevel, kDefaultAmbientLevel, kDefaultAmbientLevel, 1.0f},
      mSpecular{0.0f, 0.0f, 0.0f, 1.0f}, mInnerAngle(kDefaultConeAngle),
      mOuterAngle(kDefaultConeAngle), mRange(kDefaultRange), mConstantAtten(1.0f),
      mLinearAtten(0.0f), mQuadraticAtten(0.0f), mType(kLightTypeDirectional) {
}

// NTSC-U/C: 0x005445d8, PAL: 0x00584350
Light::~Light() {
    ReleaseAllRefs();
}

// NTSC-U/C: 0x00544400, PAL: 0x00584178
void Light::SetColors(const Color &ambient, const Color &diffuse, const Color &specular) {
    mAmbient = ambient;
    mDiffuse = diffuse;
    mSpecular = specular;
}

// NTSC-U/C: 0x00544420, PAL: 0x00584198
void Light::SetType(Type type) {
    mType = type;
}

// NTSC-U/C: 0x00544428, PAL: 0x005841a0
void Light::SetRange(float flRange) {
    mRange = flRange;
}

// NTSC-U/C: 0x00544430, PAL: 0x005841a8
void Light::SetAngles(float flInner, float flOuter) {
    mOuterAngle = flOuter;
    mInnerAngle = flInner;
}

// NTSC-U/C: 0x00544440, PAL: 0x005841b8
void Light::SetAttenuation(float flConstant, float flLinear, float flQuadratic) {
    mQuadraticAtten = flQuadratic;
    mConstantAtten = flConstant;
    mLinearAtten = flLinear;
}

// NTSC-U/C: 0x00544818, PAL: 0x00584590
void Light::SyncLight() {
}

// NTSC-U/C: 0x00540420, PAL: 0x005800f8
void Light::DumpText(Dbg &sink) {
    Object::DumpText(sink);
    Transformable::DumpText(sink);
    if (sink.mDumpLevel <= 0) {
        return;
    }

    sink.Print("[Light]\n");
    sink.Print("diffuse:");
    PrintColor(sink, mDiffuse);
    sink.Print(" ambient:");
    PrintColor(sink, mAmbient);
    sink.Print("\n");
    sink.Print("specular:");
    PrintColor(sink, mSpecular);
    sink.Print(" innerAng:");
    sink.Format("%.2f", mInnerAngle);
    sink.Print("\n");
    sink.Print("outerAng:");
    sink.Format("%.2f", mOuterAngle);
    sink.Print(" range:");
    sink.Format("%.2f", mRange);
    sink.Print("\n");
    sink.Print("constantAtten:");
    sink.Format("%.2f", mConstantAtten);
    sink.Print(" linearAtten:");
    sink.Format("%.2f", mLinearAtten);
    sink.Print(" quadraticAtten:");
    sink.Format("%.2f", mQuadraticAtten);
    sink.Print("\n");
    sink.Print("type:");
    sink << mType;
    sink.Print("\n");
}

// NTSC-U/C: 0x005408b8, PAL: 0x00580590
void Light::Save(Stream &stream) {
    const int nRevision = kLightRevision;
    stream.WriteLE(&nRevision, sizeof(nRevision));

    Transformable::Save(stream);

    // Every value below goes out through a local copy rather than from the member, which is what
    // places a copy on the stack ahead of each write. Rnd::TransAnim serialises its keys the same
    // way.
    float flValue = mDiffuse.r;
    stream.WriteLE(&flValue, sizeof(flValue));
    flValue = mDiffuse.g;
    stream.WriteLE(&flValue, sizeof(flValue));
    flValue = mDiffuse.b;
    stream.WriteLE(&flValue, sizeof(flValue));
    flValue = mDiffuse.a;
    stream.WriteLE(&flValue, sizeof(flValue));

    flValue = mAmbient.r;
    stream.WriteLE(&flValue, sizeof(flValue));
    flValue = mAmbient.g;
    stream.WriteLE(&flValue, sizeof(flValue));
    flValue = mAmbient.b;
    stream.WriteLE(&flValue, sizeof(flValue));
    flValue = mAmbient.a;
    stream.WriteLE(&flValue, sizeof(flValue));

    flValue = mSpecular.r;
    stream.WriteLE(&flValue, sizeof(flValue));
    flValue = mSpecular.g;
    stream.WriteLE(&flValue, sizeof(flValue));
    flValue = mSpecular.b;
    stream.WriteLE(&flValue, sizeof(flValue));
    flValue = mSpecular.a;
    stream.WriteLE(&flValue, sizeof(flValue));

    flValue = mInnerAngle;
    stream.WriteLE(&flValue, sizeof(flValue));
    flValue = mOuterAngle;
    stream.WriteLE(&flValue, sizeof(flValue));
    flValue = mRange;
    stream.WriteLE(&flValue, sizeof(flValue));
    flValue = mConstantAtten;
    stream.WriteLE(&flValue, sizeof(flValue));
    flValue = mLinearAtten;
    stream.WriteLE(&flValue, sizeof(flValue));
    flValue = mQuadraticAtten;
    stream.WriteLE(&flValue, sizeof(flValue));

    const int nType = mType;
    stream.WriteLE(&nType, sizeof(nType));
}

// NTSC-U/C: 0x00540be8, PAL: 0x005808c0
void Light::Load(Stream &stream) {
    int nRevision = 0;
    stream.ReadLE(&nRevision, sizeof(nRevision));
    if (nRevision > kLightRevision) {
        Rnd::TheDbg.Notify("Can't load new Light\n");
        return;
    }

    Transformable::Load(stream);

    stream.ReadLE(&mDiffuse.r, sizeof(float));
    stream.ReadLE(&mDiffuse.g, sizeof(float));
    stream.ReadLE(&mDiffuse.b, sizeof(float));
    stream.ReadLE(&mDiffuse.a, sizeof(float));
    stream.ReadLE(&mAmbient.r, sizeof(float));
    stream.ReadLE(&mAmbient.g, sizeof(float));
    stream.ReadLE(&mAmbient.b, sizeof(float));
    stream.ReadLE(&mAmbient.a, sizeof(float));
    stream.ReadLE(&mSpecular.r, sizeof(float));
    stream.ReadLE(&mSpecular.g, sizeof(float));
    stream.ReadLE(&mSpecular.b, sizeof(float));
    stream.ReadLE(&mSpecular.a, sizeof(float));

    stream.ReadLE(&mInnerAngle, sizeof(mInnerAngle));
    stream.ReadLE(&mOuterAngle, sizeof(mOuterAngle));
    stream.ReadLE(&mRange, sizeof(mRange));
    stream.ReadLE(&mConstantAtten, sizeof(mConstantAtten));
    stream.ReadLE(&mLinearAtten, sizeof(mLinearAtten));
    stream.ReadLE(&mQuadraticAtten, sizeof(mQuadraticAtten));

    if (nRevision >= kLightTypeRevision) {
        int nType = 0;
        stream.ReadLE(&nType, sizeof(nType));
        mType = static_cast<Type>(nType);
    }

    SyncLight();
}

// NTSC-U/C: 0x00545480, PAL: 0x005851f8
void Light::Replace(Object *pFrom, Object *pTo) {
    Transformable::Replace(pFrom, pTo);
}

// NTSC-U/C: 0x00545390, PAL: 0x00585108
void Light::Copy(const Object *pSource, unsigned nFlags) {
    const Light *pSourceLight = dynamic_cast<const Light *>(pSource);

    Transformable::Copy(pSource, nFlags);

    mDiffuse = pSourceLight->mDiffuse;
    mAmbient = pSourceLight->mAmbient;
    mSpecular = pSourceLight->mSpecular;
    mInnerAngle = pSourceLight->mInnerAngle;
    mOuterAngle = pSourceLight->mOuterAngle;
    mRange = pSourceLight->mRange;
    mConstantAtten = pSourceLight->mConstantAtten;
    mLinearAtten = pSourceLight->mLinearAtten;
    mQuadraticAtten = pSourceLight->mQuadraticAtten;
    mType = pSourceLight->mType;

    SyncLight();
}

// NTSC-U/C: 0x005448b8, PAL: 0x00584630
Light *NewLight(const HxStr &name) {
    return new Light(name);
}

// NTSC-U/C: 0x00720bc8, PAL: 0x00764658
Light *(*g_pfnNewLight)(const HxStr &name) = NewLight;

// NTSC-U/C: 0x005443c0, PAL: 0x00584138
void *Light::operator new(size_t nSize) {
    return AllocateTaggedMemory(nSize, kLightTag);
}

// NTSC-U/C: 0x005443e0, PAL: 0x00584158
void Light::operator delete(void *pBlock) {
    OperatorDeleteOverride(pBlock, kLightTag);
}

// NTSC-U/C: 0x00544490, PAL: 0x00584208
Light *NewLightThroughHook(const HxStr &name) {
    try {
        return g_pfnNewLight(name);
    } catch (...) {
        return nullptr; // The binary's handler returns null.
    }
}

// NTSC-U/C: 0x00544828, PAL: 0x005845a0
Object *CreateRegisteredLight(const HxStr &name) {
    try {
        return g_pfnNewLight(name);
    } catch (...) {
        return nullptr;
    }
}

// NTSC-U/C: 0x00544450, PAL: 0x005841c8
void RegisterLightClass() {
    g_pfnNewLight = NewLight;
    TheManager.RegisterClass(g_lightClassName, CreateRegisteredLight);
}

} // namespace Rnd
