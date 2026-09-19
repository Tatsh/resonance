#include "rnd/light.h"

#include "math/color.h"

namespace Rnd {

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

} // namespace Rnd
