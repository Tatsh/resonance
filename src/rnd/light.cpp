#include "rnd/light.h"

#include "math/color.h"
#include "os/failsink.h"
#include "os/hxstr.h"
#include "rnd/object.h"
#include "rnd/stream.h"
#include "rnd/transformable.h"

namespace Rnd {

namespace {

// Revision Save() writes and the highest revision Load() accepts.
constexpr int kLightRevision = 1;

// First revision that stores the type word. A revision of 0 predates it.
constexpr int kLightTypeRevision = 1;

// src/rnd/mat.cpp declares a helper of the same title file-locally for the same reason. Every
// component is a separate Print and Format pair, which is why the two literals "(r:" and "%.2f"
// survive apart in the read-only data.
void PrintColor(FailSink &sink, const Color &color) {
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

// 0x005454a0
FailSink &PrintLightType(FailSink &sink, LightType type) {
    switch (type) {
    case kLightTypePoint:
        sink.Print("Point");
        break;
    case kLightTypeDirectional:
        sink.Print("Directional");
        break;
    case kLightTypeSpot:
        sink.Print("Spot");
        break;
    }
    return sink;
}

} // namespace

// 0x00720bd0
HxStr g_lightClassName("Light");

// 0x005445c8
const HxStr &Light::ClassName() const {
    return g_lightClassName;
}

void Light::SetColors(const Color &ambient, const Color &diffuse, const Color &specular) {
    mAmbient = ambient;
    mDiffuse = diffuse;
    mSpecular = specular;
}

void Light::SetType(LightType type) {
    mType = type;
}

void Light::SetRange(float flRange) {
    mRange = flRange;
}

void Light::SetAngles(float flInner, float flOuter) {
    mOuterAngle = flOuter;
    mInnerAngle = flInner;
}

void Light::SetAttenuation(float flConstant, float flLinear, float flQuadratic) {
    mQuadraticAtten = flQuadratic;
    mConstantAtten = flConstant;
    mLinearAtten = flLinear;
}

void Light::ApplyUnknown() {
}

void Light::DumpText(FailSink &sink) {
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
    sink.Print("\n");
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
    PrintLightType(sink, mType);
    sink.Print("\n");
}

void Light::Save(Stream &stream) {
    const int nRevision = kLightRevision;
    stream.Write(&nRevision, sizeof(nRevision));

    Transformable::Save(stream);

    // Every value below goes out through a local copy rather than from the member, which is what
    // places a copy on the stack ahead of each write. Rnd::TransAnim serialises its keys the same
    // way.
    float flValue = mDiffuse.r;
    stream.Write(&flValue, sizeof(flValue));
    flValue = mDiffuse.g;
    stream.Write(&flValue, sizeof(flValue));
    flValue = mDiffuse.b;
    stream.Write(&flValue, sizeof(flValue));
    flValue = mDiffuse.a;
    stream.Write(&flValue, sizeof(flValue));

    flValue = mAmbient.r;
    stream.Write(&flValue, sizeof(flValue));
    flValue = mAmbient.g;
    stream.Write(&flValue, sizeof(flValue));
    flValue = mAmbient.b;
    stream.Write(&flValue, sizeof(flValue));
    flValue = mAmbient.a;
    stream.Write(&flValue, sizeof(flValue));

    flValue = mSpecular.r;
    stream.Write(&flValue, sizeof(flValue));
    flValue = mSpecular.g;
    stream.Write(&flValue, sizeof(flValue));
    flValue = mSpecular.b;
    stream.Write(&flValue, sizeof(flValue));
    flValue = mSpecular.a;
    stream.Write(&flValue, sizeof(flValue));

    flValue = mInnerAngle;
    stream.Write(&flValue, sizeof(flValue));
    flValue = mOuterAngle;
    stream.Write(&flValue, sizeof(flValue));
    flValue = mRange;
    stream.Write(&flValue, sizeof(flValue));
    flValue = mConstantAtten;
    stream.Write(&flValue, sizeof(flValue));
    flValue = mLinearAtten;
    stream.Write(&flValue, sizeof(flValue));
    flValue = mQuadraticAtten;
    stream.Write(&flValue, sizeof(flValue));

    const int nType = mType;
    stream.Write(&nType, sizeof(nType));
}

void Light::Load(Stream &stream) {
    int nRevision = 0;
    stream.Read(&nRevision, sizeof(nRevision));
    if (nRevision > kLightRevision) {
        g_failSink.Report("Can't load new Light\n");
        return;
    }

    Transformable::Load(stream);

    stream.Read(&mDiffuse.r, sizeof(float));
    stream.Read(&mDiffuse.g, sizeof(float));
    stream.Read(&mDiffuse.b, sizeof(float));
    stream.Read(&mDiffuse.a, sizeof(float));
    stream.Read(&mAmbient.r, sizeof(float));
    stream.Read(&mAmbient.g, sizeof(float));
    stream.Read(&mAmbient.b, sizeof(float));
    stream.Read(&mAmbient.a, sizeof(float));
    stream.Read(&mSpecular.r, sizeof(float));
    stream.Read(&mSpecular.g, sizeof(float));
    stream.Read(&mSpecular.b, sizeof(float));
    stream.Read(&mSpecular.a, sizeof(float));

    stream.Read(&mInnerAngle, sizeof(mInnerAngle));
    stream.Read(&mOuterAngle, sizeof(mOuterAngle));
    stream.Read(&mRange, sizeof(mRange));
    stream.Read(&mConstantAtten, sizeof(mConstantAtten));
    stream.Read(&mLinearAtten, sizeof(mLinearAtten));
    stream.Read(&mQuadraticAtten, sizeof(mQuadraticAtten));

    if (nRevision >= kLightTypeRevision) {
        int nType = 0;
        stream.Read(&nType, sizeof(nType));
        mType = static_cast<LightType>(nType);
    }

    ApplyUnknown();
}

void Light::Replace(Object *pFrom, Object *pTo) {
    Transformable::Replace(pFrom, pTo);
}

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

    ApplyUnknown();
}

// 0x005448b8
Light *NewLight(const HxStr &name) {
    return new Light(name);
}

// 0x00720bc8
Light *(*g_pfnNewLight)(const HxStr &name) = NewLight;

} // namespace Rnd
