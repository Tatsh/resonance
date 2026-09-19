#include "rnd/mat.h"

#include <vector>

#include "os/failsink.h"
#include "os/hxstr.h"
#include "rnd/manager.h"
#include "rnd/stream.h"
#include "rnd/tex.h"
#include "rndartt/apalette.h"

namespace Rnd {

namespace {

constexpr char kNoObject[] = "no object";

// 0x004d2e68
// The same printer serves the material blend and the stage blend.
FailSink &PrintBlendMode(FailSink &sink, Mat::BlendMode nBlend) {
    switch (nBlend) {
    case Mat::kBlendModeDest:
        sink.Print("Dest");
        break;
    case Mat::kBlendModeSrc:
        sink.Print("Src");
        break;
    case Mat::kBlendModeAdd:
        sink.Print("Add");
        break;
    case Mat::kBlendModeMultiply:
        sink.Print("Multiply");
        break;
    case Mat::kBlendModeMultiply2:
        sink.Print("Multiply2");
        break;
    case Mat::kBlendModeSrcAlpha:
        sink.Print("SrcAlpha");
        break;
    case Mat::kBlendModeSrcAlphaAdd:
        sink.Print("SrcAlphaAdd");
        break;
    case Mat::kBlendModeSrcAdd:
        sink.Print("SrcAdd");
        break;
    case Mat::kBlendModeInvSrcAlpha:
        sink.Print("InvSrcAlpha");
        break;
    case Mat::kBlendModeDestAlpha:
        sink.Print("DestAlpha");
        break;
    case Mat::kBlendModeInvDestAlpha:
        sink.Print("InvDestAlpha");
        break;
    case Mat::kBlendModeSrcAlphaOpaque:
        sink.Print("SrcAlphaOpaque");
        break;
    case Mat::kBlendModeSrcAlphaCutout:
        sink.Print("SrcAlphaCutout");
        break;
    }
    return sink;
}

// 0x004dd0f0
FailSink &PrintCullMode(FailSink &sink, Mat::CullMode nCull) {
    switch (nCull) {
    case Mat::kCullModeCw:
        sink.Print("CW");
        break;
    case Mat::kCullModeCcw:
        sink.Print("CCW");
        break;
    case Mat::kCullModeNone:
        sink.Print("NoCull");
        break;
    }
    return sink;
}

// 0x004dd188
FailSink &PrintGenMode(FailSink &sink, Mat::Stage::GenMode nGenMode) {
    switch (nGenMode) {
    case Mat::Stage::kGenModeFixed:
        sink.Print("Fixed");
        break;
    case Mat::Stage::kGenModeSphere:
        sink.Print("Sphere");
        break;
    case Mat::Stage::kGenModePlanar:
        sink.Print("Planar");
        break;
    case Mat::Stage::kGenModeOrthoCube:
        sink.Print("OrthoCube");
        break;
    case Mat::Stage::kGenModeLocalCube:
        sink.Print("LocalCube");
        break;
    }
    return sink;
}

// 0x004dd240
FailSink &PrintWrapMode(FailSink &sink, Mat::Stage::WrapMode nWrap) {
    switch (nWrap) {
    case Mat::Stage::kWrapModeClamp:
        sink.Print("Clamp");
        break;
    case Mat::Stage::kWrapModeRepeat:
        sink.Print("Repeat");
        break;
    case Mat::Stage::kWrapModeMirror:
        sink.Print("Mirror");
        break;
    }
    return sink;
}

const char *NameText(const Object *pObject) {
    return pObject->mName.mStr != nullptr ? pObject->mName.mStr : "";
}

void PrintObjectRef(FailSink &sink, const Object *pObject) {
    if (pObject == nullptr) {
        sink.Print(kNoObject);
        return;
    }
    sink.Format("\"%s\"", NameText(pObject));
}

void WriteObjectRef(Stream &stream, const Object *pObject) {
    if (pObject == nullptr) {
        const char chTerminator = '\0';
        stream.WriteBytes(&chTerminator, 1);
        return;
    }
    stream.WriteBytes(NameText(pObject), pObject->mName.mLen + 1);
}

template <class T>
void ReadObjectRef(Stream &stream, T *&refOut) {
    HxStr name(nullptr);
    stream.ReadString(name);
    refOut = dynamic_cast<T *>(g_manager.Find(name));
}

void PrintColor(FailSink &sink, const Color &color) {
    sink.Format("(r:%.2f", color.r);
    sink.Format(" g:%.2f", color.g);
    sink.Format(" b:%.2f", color.b);
    sink.Format(" a:%.2f", color.a);
    sink.Print(")");
}

void PrintVector3(FailSink &sink, const Vector3 &v) {
    sink.Format("(x:%.2f", v.x);
    sink.Format(" y:%.2f", v.y);
    sink.Format(" z:%.2f", v.z);
    sink.Print(")");
}

void PrintBool(FailSink &sink, int nValue) {
    sink.Print(nValue != 0 ? "true" : "false");
}

// 0x004dd020
void InitStageDefaults(Mat::Stage &stage) {
    stage.mBlend = Mat::kBlendModeMultiply;
    stage.mCoordIndex = 0;
    stage.mGenMode = Mat::Stage::kGenModeFixed;
    stage.mXfm.mBasisX.x = 1.0f;
    stage.mXfm.mBasisX.y = 0.0f;
    stage.mXfm.mBasisX.z = 0.0f;
    stage.mXfm.mBasisX.w = 1.0f;
    stage.mXfm.mBasisY.x = 0.0f;
    stage.mXfm.mBasisY.y = 1.0f;
    stage.mXfm.mBasisY.z = 0.0f;
    stage.mXfm.mBasisY.w = 1.0f;
    stage.mXfm.mBasisZ.x = 0.0f;
    stage.mXfm.mBasisZ.y = 0.0f;
    stage.mXfm.mBasisZ.z = 1.0f;
    stage.mXfm.mBasisZ.w = 1.0f;
    stage.mXfm.mTranslation.x = 0.0f;
    stage.mXfm.mTranslation.y = 0.0f;
    stage.mXfm.mTranslation.z = 0.0f;
    stage.mXfm.mTranslation.w = 1.0f;
    stage.mUseXfm = 0;
    stage.mWrap = Mat::Stage::kWrapModeRepeat;
    stage.mTex = nullptr;
    stage.mMat = nullptr;
}

// 0x004d2270
FailSink &DumpStage(FailSink &sink, const Mat::Stage &stage) {
    sink.Print("\n\tblend:");
    PrintBlendMode(sink, stage.mBlend);
    sink.Print(" coordIndex:");
    sink.Format("%d", stage.mCoordIndex);
    sink.Print(" genMode:");
    PrintGenMode(sink, stage.mGenMode);
    sink.Print(" xfm:");
    PrintVector3(sink, stage.mXfm.mBasisX);
    PrintVector3(sink, stage.mXfm.mBasisY);
    PrintVector3(sink, stage.mXfm.mBasisZ);
    PrintVector3(sink, stage.mXfm.mTranslation);
    sink.Print("useXfm:");
    PrintBool(sink, stage.mUseXfm);
    sink.Print(" wrap:");
    PrintWrapMode(sink, stage.mWrap);
    // Yes, the dump writes the material reference before the texture reference even though the
    // texture is the earlier member.
    sink.Print(" mat:");
    PrintObjectRef(sink, stage.mMat);
    sink.Print(" tex:");
    PrintObjectRef(sink, stage.mTex);
    return sink;
}

// 0x004d75c8
// The vector dump opens with the element count and then writes the index of every stage on its
// own line, the same shape the mesh vector dumps use.
FailSink &DumpStageVector(FailSink &sink, const std::vector<Mat::Stage> &stages) {
    sink.Print("(size:");
    sink.Format("%u", stages.size());
    sink.Print(")");
    for (unsigned nIndex = 0; nIndex < stages.size(); ++nIndex) {
        sink.Print("\n");
        sink.Format("%d", nIndex);
        sink.Print("\t");
        DumpStage(sink, stages[nIndex]);
    }
    return sink;
}

// 0x004d26f8
Stream &WriteStage(Stream &stream, const Mat::Stage &stage) {
    stream.Write(&stage.mBlend, sizeof(stage.mBlend));
    stream.Write(&stage.mCoordIndex, sizeof(stage.mCoordIndex));
    stream.Write(&stage.mGenMode, sizeof(stage.mGenMode));
    stream.Write(&stage.mXfm.mBasisX.x, sizeof(float));
    stream.Write(&stage.mXfm.mBasisX.y, sizeof(float));
    stream.Write(&stage.mXfm.mBasisX.z, sizeof(float));
    stream.Write(&stage.mXfm.mBasisY.x, sizeof(float));
    stream.Write(&stage.mXfm.mBasisY.y, sizeof(float));
    stream.Write(&stage.mXfm.mBasisY.z, sizeof(float));
    stream.Write(&stage.mXfm.mBasisZ.x, sizeof(float));
    stream.Write(&stage.mXfm.mBasisZ.y, sizeof(float));
    stream.Write(&stage.mXfm.mBasisZ.z, sizeof(float));
    stream.Write(&stage.mXfm.mTranslation.x, sizeof(float));
    stream.Write(&stage.mXfm.mTranslation.y, sizeof(float));
    stream.Write(&stage.mXfm.mTranslation.z, sizeof(float));
    const char chUseXfm = static_cast<char>(stage.mUseXfm);
    stream.WriteBytes(&chUseXfm, sizeof(chUseXfm));
    stream.Write(&stage.mWrap, sizeof(stage.mWrap));
    WriteObjectRef(stream, stage.mTex);
    WriteObjectRef(stream, stage.mMat);
    return stream;
}

// 0x004dd860
Stream &WriteStageVector(Stream &stream, const std::vector<Mat::Stage> &stages) {
    const int nCount = static_cast<int>(stages.size());
    stream.Write(&nCount, sizeof(nCount));
    for (const auto &stage : stages) {
        WriteStage(stream, stage);
    }
    return stream;
}

// 0x004d76d8
// The stage record is read back in the order WriteStage() writes it.
Stream &ReadStageVector(Stream &stream, std::vector<Mat::Stage> &stages) {
    int nCount = 0;
    stream.Read(&nCount, sizeof(nCount));
    stages.resize(nCount);
    for (auto &stage : stages) {
        InitStageDefaults(stage);
        stream.Read(&stage.mBlend, sizeof(stage.mBlend));
        stream.Read(&stage.mCoordIndex, sizeof(stage.mCoordIndex));
        stream.Read(&stage.mGenMode, sizeof(stage.mGenMode));
        stream.Read(&stage.mXfm.mBasisX.x, sizeof(float));
        stream.Read(&stage.mXfm.mBasisX.y, sizeof(float));
        stream.Read(&stage.mXfm.mBasisX.z, sizeof(float));
        stream.Read(&stage.mXfm.mBasisY.x, sizeof(float));
        stream.Read(&stage.mXfm.mBasisY.y, sizeof(float));
        stream.Read(&stage.mXfm.mBasisY.z, sizeof(float));
        stream.Read(&stage.mXfm.mBasisZ.x, sizeof(float));
        stream.Read(&stage.mXfm.mBasisZ.y, sizeof(float));
        stream.Read(&stage.mXfm.mBasisZ.z, sizeof(float));
        stream.Read(&stage.mXfm.mTranslation.x, sizeof(float));
        stream.Read(&stage.mXfm.mTranslation.y, sizeof(float));
        stream.Read(&stage.mXfm.mTranslation.z, sizeof(float));
        char chUseXfm = 0;
        stream.ReadBytes(&chUseXfm, sizeof(chUseXfm));
        stage.mUseXfm = chUseXfm != 0;
        stream.Read(&stage.mWrap, sizeof(stage.mWrap));
        ReadObjectRef(stream, stage.mTex);
        ReadObjectRef(stream, stage.mMat);
    }
    return stream;
}

// The blend mode replaced a source factor and a destination factor in version 3. A pair the table
// does not recognise has no mapping, and the blend mode stays as it was.
Mat::BlendMode MapLegacyBlendFactors(Mat::BlendMode current, int nSrc, int nDst) {
    if (nSrc == 0 && nDst == 1) {
        return Mat::kBlendModeDest;
    }
    if (nSrc == 1 && nDst == 0) {
        return Mat::kBlendModeSrc;
    }
    if (nSrc == 1 && nDst == 1) {
        return Mat::kBlendModeAdd;
    }
    if (nSrc == 2 && nDst == 1) {
        return Mat::kBlendModeSrcAdd;
    }
    if (nSrc == 4 && nDst == 1) {
        return Mat::kBlendModeSrcAlphaAdd;
    }
    if (nSrc == 6 && nDst == 0) {
        return Mat::kBlendModeMultiply;
    }
    if (nSrc == 6 && nDst == 2) {
        return Mat::kBlendModeMultiply2;
    }
    if (nSrc == 4 && nDst == 5) {
        return Mat::kBlendModeSrcAlpha;
    }
    if (nSrc == 5 && nDst == 4) {
        return Mat::kBlendModeInvSrcAlpha;
    }
    if (nSrc == 8 && nDst == 9) {
        return Mat::kBlendModeDestAlpha;
    }
    if (nSrc == 9 && nDst == 8) {
        return Mat::kBlendModeInvDestAlpha;
    }
    return current;
}

} // namespace

// 0x00700420
HxStr g_matClassName("Mat");

// 0x00894e2c
int g_nRndMatLoadVersion;

// 0x004d0ea8
Mat::Mat(const HxStr &name)
    : Object(name), mBlend(kBlendModeSrcAlpha), mEnable(1), mVertAmbient(0), mVertDiffuse(0),
      mVertSpecular(0), mVertEmissive(0), mVertAlpha(0), mNormalize(0), mCull(kCullModeCw),
      mMultiPass(0), mFlat(0) {
    mEmissive.r = 0.0f;
    mEmissive.g = 0.0f;
    mEmissive.b = 0.0f;
    mEmissive.a = 1.0f;
    mAmbient.r = 1.0f;
    mAmbient.g = 1.0f;
    mAmbient.b = 1.0f;
    mAmbient.a = 1.0f;
    mDiffuse.r = 1.0f;
    mDiffuse.g = 1.0f;
    mDiffuse.b = 1.0f;
    mDiffuse.a = 1.0f;
    mSpecular.r = 0.0f;
    mSpecular.g = 0.0f;
    mSpecular.b = 0.0f;
    mSpecular.a = 1.0f;
}

// 0x004dcd20
void Mat::RemoveStageTexRefs() {
    for (auto &stage : mStages) {
        if (stage.mTex != nullptr) {
            stage.mTex->RemoveRef(this);
        }
    }
}

// 0x004dbb10
Mat::~Mat() {
    // The stage textures are the only references a material takes.
    RemoveStageTexRefs();
}

// 0x004d0f78
void Mat::DumpText(FailSink &sink) {
    Object::DumpText(sink);
    if (sink.mDumpLevel <= 0) {
        return;
    }

    sink.Print("[Mat]\n");
    sink.Print("stages:");
    DumpStageVector(sink, mStages);
    sink.Print("blend:");
    PrintBlendMode(sink, mBlend);
    sink.Print(" enable:");
    PrintBool(sink, mEnable);
    sink.Print("\n");

    sink.Print("ambient:");
    PrintColor(sink, mAmbient);
    sink.Print("diffuse:");
    PrintColor(sink, mDiffuse);
    sink.Print("specular:");
    PrintColor(sink, mSpecular);
    sink.Print(" emissive:");
    PrintColor(sink, mEmissive);
    sink.Print("\n");

    sink.Print("vertAmbient:");
    PrintBool(sink, mVertAmbient);
    sink.Print(" vertDiffuse:");
    PrintBool(sink, mVertDiffuse);
    sink.Print(" vertSpecular:");
    PrintBool(sink, mVertSpecular);
    sink.Print("\n");

    sink.Print("vertEmissive:");
    PrintBool(sink, mVertEmissive);
    sink.Print(" vertAlpha:");
    PrintBool(sink, mVertAlpha);
    sink.Print("\n");

    sink.Print("cull:");
    PrintCullMode(sink, mCull);
    sink.Print(" multiPass:");
    sink.Format("%d", mMultiPass);
    sink.Print(" normalize:");
    PrintBool(sink, mNormalize);
    sink.Print(" flat:");
    PrintBool(sink, mFlat);
    sink.Print("\n");
}

// 0x004d1638
void Mat::Save(Stream &stream) {
    const int nVersion = kSerialVersion;
    stream.Write(&nVersion, sizeof(nVersion));

    WriteStageVector(stream, mStages);
    stream.Write(&mBlend, sizeof(mBlend));

    stream.Write(&mAmbient.r, sizeof(float));
    stream.Write(&mAmbient.g, sizeof(float));
    stream.Write(&mAmbient.b, sizeof(float));
    stream.Write(&mAmbient.a, sizeof(float));
    stream.Write(&mDiffuse.r, sizeof(float));
    stream.Write(&mDiffuse.g, sizeof(float));
    stream.Write(&mDiffuse.b, sizeof(float));
    stream.Write(&mDiffuse.a, sizeof(float));
    stream.Write(&mSpecular.r, sizeof(float));
    stream.Write(&mSpecular.g, sizeof(float));
    stream.Write(&mSpecular.b, sizeof(float));
    stream.Write(&mSpecular.a, sizeof(float));
    stream.Write(&mEmissive.r, sizeof(float));
    stream.Write(&mEmissive.g, sizeof(float));
    stream.Write(&mEmissive.b, sizeof(float));
    stream.Write(&mEmissive.a, sizeof(float));

    const char achFlags[] = {static_cast<char>(mEnable),
                             static_cast<char>(mVertAmbient),
                             static_cast<char>(mVertDiffuse),
                             static_cast<char>(mVertSpecular),
                             static_cast<char>(mVertEmissive),
                             static_cast<char>(mVertAlpha)};
    for (const auto chFlag : achFlags) {
        stream.WriteBytes(&chFlag, sizeof(chFlag));
    }

    stream.Write(&mCull, sizeof(mCull));
    stream.Write(&mMultiPass, sizeof(mMultiPass));
    const char chNormalize = static_cast<char>(mNormalize);
    stream.WriteBytes(&chNormalize, sizeof(chNormalize));
    const char chFlat = static_cast<char>(mFlat);
    stream.WriteBytes(&chFlat, sizeof(chFlat));
}

// 0x004dcc20
void Mat::Replace(Object *pFrom, Object *pTo) {
    for (auto &stage : mStages) {
        if (stage.mTex != pFrom) {
            continue;
        }
        if (pFrom != nullptr) {
            pFrom->RemoveRef(this);
        }
        if (stage.mTex != nullptr) {
            stage.mTex = dynamic_cast<Tex *>(pTo);
        }
        if (stage.mTex != nullptr) {
            stage.mTex->AddRef(this);
        }
    }
}

// 0x004dbc70
const HxStr &Mat::ClassName() const {
    return g_matClassName;
}

// 0x004dce18
void Mat::Copy(const Object *pSource, unsigned nFlags) {
    const Mat *pMat = dynamic_cast<const Mat *>(pSource);

    for (auto &stage : mStages) {
        if (stage.mTex != nullptr) {
            stage.mTex->RemoveRef(this);
        }
    }

    mBlend = pMat->mBlend;
    mEmissive = pMat->mEmissive;
    mStages = pMat->mStages;
    mAmbient = pMat->mAmbient;
    mDiffuse = pMat->mDiffuse;
    mSpecular = pMat->mSpecular;
    mEnable = pMat->mEnable;
    mVertAmbient = pMat->mVertAmbient;
    mVertDiffuse = pMat->mVertDiffuse;
    mVertSpecular = pMat->mVertSpecular;
    mVertEmissive = pMat->mVertEmissive;
    mVertAlpha = pMat->mVertAlpha;
    mCull = pMat->mCull;
    mNormalize = pMat->mNormalize;

    Refresh();
}

// 0x004d1a90
void Mat::Load(Stream &stream) {
    stream.Read(&g_nRndMatLoadVersion, sizeof(g_nRndMatLoadVersion));
    if (g_nRndMatLoadVersion > kSerialVersion) {
        g_failSink.Report("Can't load new Mat\n");
        g_failSink.mAbortProc();
        return;
    }

    for (auto &stage : mStages) {
        if (stage.mTex != nullptr) {
            stage.mTex->RemoveRef(this);
        }
    }
    ReadStageVector(stream, mStages);

    if (g_nRndMatLoadVersion >= 3) {
        stream.Read(&mBlend, sizeof(mBlend));
    } else {
        int nSrcFactor = 0;
        int nDstFactor = 0;
        stream.Read(&nSrcFactor, sizeof(nSrcFactor));
        stream.Read(&nDstFactor, sizeof(nDstFactor));
        mBlend = MapLegacyBlendFactors(mBlend, nSrcFactor, nDstFactor);
    }

    stream.Read(&mAmbient.r, sizeof(float));
    stream.Read(&mAmbient.g, sizeof(float));
    stream.Read(&mAmbient.b, sizeof(float));
    stream.Read(&mAmbient.a, sizeof(float));
    stream.Read(&mDiffuse.r, sizeof(float));
    stream.Read(&mDiffuse.g, sizeof(float));
    stream.Read(&mDiffuse.b, sizeof(float));
    stream.Read(&mDiffuse.a, sizeof(float));
    stream.Read(&mSpecular.r, sizeof(float));
    stream.Read(&mSpecular.g, sizeof(float));
    stream.Read(&mSpecular.b, sizeof(float));
    stream.Read(&mSpecular.a, sizeof(float));
    stream.Read(&mEmissive.r, sizeof(float));
    stream.Read(&mEmissive.g, sizeof(float));
    stream.Read(&mEmissive.b, sizeof(float));
    stream.Read(&mEmissive.a, sizeof(float));

    if (g_nRndMatLoadVersion < 6) {
        // The two alpha values used to sit after the colours rather than inside them.
        float flSpecularAlpha = 0.0f;
        float flDiffuseAlpha = 0.0f;
        stream.Read(&flSpecularAlpha, sizeof(flSpecularAlpha));
        stream.Read(&flDiffuseAlpha, sizeof(flDiffuseAlpha));
        mDiffuse.a = flDiffuseAlpha;
        mSpecular.a = flSpecularAlpha;
    }

    char chFlag = 0;
    stream.ReadBytes(&chFlag, sizeof(chFlag));
    mEnable = chFlag != 0;
    stream.ReadBytes(&chFlag, sizeof(chFlag));
    mVertAmbient = chFlag != 0;
    stream.ReadBytes(&chFlag, sizeof(chFlag));
    mVertDiffuse = chFlag != 0;
    stream.ReadBytes(&chFlag, sizeof(chFlag));
    mVertSpecular = chFlag != 0;
    stream.ReadBytes(&chFlag, sizeof(chFlag));
    mVertEmissive = chFlag != 0;
    stream.ReadBytes(&chFlag, sizeof(chFlag));
    mVertAlpha = chFlag != 0;

    stream.Read(&mCull, sizeof(mCull));
    if (g_nRndMatLoadVersion >= 7) {
        stream.Read(&mMultiPass, sizeof(mMultiPass));
    } else if (g_nRndMatLoadVersion >= 2) {
        stream.ReadBytes(&chFlag, sizeof(chFlag));
        mMultiPass = chFlag != 0;
    }
    if (g_nRndMatLoadVersion >= 4) {
        stream.ReadBytes(&chFlag, sizeof(chFlag));
        mNormalize = chFlag != 0;
    }
    if (g_nRndMatLoadVersion >= 5) {
        stream.ReadBytes(&chFlag, sizeof(chFlag));
        mFlat = chFlag != 0;
    }
}

// 0x004db958
void Mat::SyncMat() {
}

// 0x004db970
void Mat::SetAmbient(const Color &color) {
    mAmbient = color;
}

// 0x004db980
void Mat::SetDiffuse(const Color &color) {
    mDiffuse.r = color.r;
    mDiffuse.g = color.g;
    mDiffuse.b = color.b;
}

// 0x004db9a0
void Mat::SetEmissive(const Color &color) {
    mEmissive = color;
}

// 0x004db9b0
void Mat::SetAlpha(float flAlpha) {
    mDiffuse.a = flAlpha;
}

// 0x004db9b8
void Mat::SetSpecular(const Color &color, float flAlpha) {
    mSpecular.r = color.r;
    mSpecular.g = color.g;
    mSpecular.b = color.b;
    mSpecular.a = flAlpha; // Yes, the binary discards color.a and stores the argument instead.
}

// 0x004dcbc0
void Mat::SetLighting(int nEnable,
                      int nVertAmbient,
                      int nVertDiffuse,
                      int nVertSpecular,
                      int nVertEmissive,
                      int nVertAlpha,
                      int nNormalize) {
    mNormalize = nNormalize;
    mEnable = nEnable;
    mVertAmbient = nVertAmbient;
    mVertDiffuse = nVertDiffuse;
    mVertSpecular = nVertSpecular;
    mVertEmissive = nVertEmissive;
    mVertAlpha = nVertAlpha;
}

// 0x004db960
void Mat::SetMultiPass(int nMultiPass) {
    mMultiPass = nMultiPass;
}

// 0x004db968
void Mat::SetFlat(int nFlat) {
    mFlat = nFlat;
}

} // namespace Rnd
