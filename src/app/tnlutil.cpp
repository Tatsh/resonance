#include "app/tnlutil.h"

#include "math/color.h"
#include "os/hxstr.h"

namespace {

// The lane floor components TnlLaneColorFromName() scales, and the purple and null blends.
constexpr float kLaneComponent = 0.5f;
constexpr float kLanePurpleRed = 0.325f;
constexpr float kLaneNullBlue = 0.25f;

// The components TnlColorFromName() and TnlDimColorFromName() give purple and the dim primaries.
constexpr float kPurpleRed = 0.65f;
constexpr float kDimComponent = 0.7f;
constexpr float kDimPurpleRed = 0.5f;

enum ColorIndex {
    kColorIndexNull = 0,
    kColorIndexGreen = 1,
    kColorIndexRed = 2,
    kColorIndexYellow = 3,
    kColorIndexPurple = 4,
};

inline Color MakeColor(float flR, float flG, float flB) {
    return Color{flR, flG, flB, 1.0f};
}

} // namespace

// 0x006e42a4
int g_nAppTunnelDisplayMode;

// 0x006e42b0
float g_flTunnelBrightness;

// 0x00437e60
Color TnlColorFromName(HxStr name) {
    if (name == "green") {
        return MakeColor(0.0f, 1.0f, 0.0f);
    }
    if (name == "red") {
        return MakeColor(1.0f, 0.0f, 0.0f);
    }
    if (name == "yellow") {
        return MakeColor(1.0f, 1.0f, 0.0f);
    }
    if (name == "purple") {
        return MakeColor(kPurpleRed, 0.0f, 1.0f);
    }
    if (name == "null") {
        return MakeColor(1.0f, 1.0f, 1.0f);
    }
    return MakeColor(0.0f, 1.0f, 1.0f);
}

// 0x00437fa8
Color TnlDimColorFromName(HxStr name) {
    if (name == "green") {
        return MakeColor(0.0f, kDimComponent, 0.0f);
    }
    if (name == "red") {
        return MakeColor(kDimComponent, 0.0f, 0.0f);
    }
    if (name == "yellow") {
        return MakeColor(kDimComponent, kDimComponent, 0.0f);
    }
    if (name == "purple") {
        return MakeColor(kDimPurpleRed, 0.0f, kDimComponent);
    }
    if (name == "null") {
        return MakeColor(kDimComponent, kDimComponent, kDimComponent);
    }
    return MakeColor(0.0f, 1.0f, 1.0f);
}

// 0x00438138
Color TnlLaneColorFromName(HxStr name) {
    Color base;
    if (name == "green") {
        base = MakeColor(0.0f, kLaneComponent, 0.0f);
    } else if (name == "red") {
        base = MakeColor(kLaneComponent, 0.0f, 0.0f);
    } else if (name == "yellow") {
        base = MakeColor(kLaneComponent, kLaneComponent, 0.0f);
    } else if (name == "purple") {
        base = MakeColor(kLanePurpleRed, 0.0f, kLaneComponent);
    } else if (name == "null") {
        base = MakeColor(0.0f, 0.0f, kLaneNullBlue);
    } else {
        return MakeColor(1.0f, 1.0f, 1.0f);
    }
    Color result;
    ScaleColor(base, g_flTunnelBrightness, result);
    return result;
}

// 0x00454770
int TnlColorIndexFromName(HxStr name) {
    if (name == "null") {
        return kColorIndexNull;
    }
    if (name == "green") {
        return kColorIndexGreen;
    }
    if (name == "purple") {
        return kColorIndexPurple;
    }
    if (name == "red") {
        return kColorIndexRed;
    }
    if (name == "yellow") {
        return kColorIndexYellow;
    }
    return kColorIndexNull;
}
