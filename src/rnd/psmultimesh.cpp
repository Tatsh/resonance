#include "rnd/psmultimesh.h"

#include <algorithm>
#include <list>

#include "gfx/gfxdevice.h"
#include "gfx/renderstats.h"
#include "math/transform.h"
#include "os/hxstr.h"
#include "rnd/mat.h"
#include "rnd/mesh.h"
#include "rnd/psmat.h"
#include "rnd/psmesh.h"

namespace Rnd {

namespace {

// Quadwords reserved ahead of one material pass, the same budget Rnd::PsMesh::DrawSelf() reserves.
constexpr int kGifReserveQuadwords = 32;

// VU1 data quadwords an instance batch may share with the face run, and the most one UNPACK may
// carry. The transforms of a batch fill whatever the face run leaves under both.
constexpr int kVu1InstanceQuadwordBudget = 0x14d;
constexpr int kInstanceUnpackQuadwordLimit = 0xfe;

// One instance transform is four quadword rows.
constexpr int kQuadwordsPerTransform = 4;

// The batch header quadword that carries the instance count ahead of the transforms.
constexpr int kBatchHeaderQuadwords = 1;

// VIFcode fields, as Rnd::PsMesh assembles them.
constexpr unsigned kVifCmdMsCal = 0x14;
constexpr unsigned kVifCmdMsCnt = 0x17;
constexpr unsigned kVifCmdUnpackV4_32 = 0x6c;
constexpr int kVifCmdShift = 24;
constexpr int kVifNumShift = 16;
constexpr unsigned kVifUnpackFlg = 0x8000;

// Microprogram entry point of the instanced face path, called by the first batch. Every later
// batch continues the program instead.
constexpr unsigned kVu1InstanceEntry = 0x406;

inline unsigned MakeVifCode(unsigned nCmd, int nNum, unsigned nImmediate) {
    return (nCmd << kVifCmdShift) | (static_cast<unsigned>(nNum) << kVifNumShift) | nImmediate;
}

// Two 32-bit words ride in each half of a packet quadword, the first in the low bits.
inline unsigned long long PackWordPair(unsigned nLow, unsigned nHigh) {
    return nLow | (static_cast<unsigned long long>(nHigh) << 32);
}

// The packet buffer is untyped quadwords, and a transform is copied into it row by row.
inline const GifQuadword *AsQuadwords(const void *pRecord) {
    return static_cast<const GifQuadword *>(pRecord);
}

} // namespace

// 0x005b5c28
MultiMesh *NewPsMultiMesh(const HxStr &name) {
    // The binary allocates the 0x38 bytes the object occupies.
    return new PsMultiMesh(name);
}

// 0x005b2fd8
PsMultiMesh::PsMultiMesh(const HxStr &name) : MultiMesh(name) {
}

// 0x005b5a40
PsMultiMesh::~PsMultiMesh() {
}

// 0x005b2ed0
int PsMultiMesh::DrawSelf() {
    ++g_renderStats.mnMeshDraws;
    if (g_gfxDevice.mnUseVu1 == 0) {
        return MultiMesh::DrawSelf();
    }

    if (mTransforms.empty()) {
        return 1;
    }
    if (mMesh == nullptr) {
        return 1;
    }
    if (mMesh->mFacesOwner->mFaces.size() == 0) {
        return 1;
    }

    Mat *pMat = mMesh->mMat;
    int nPass = 0;
    int nMorePasses = 0;
    do {
        g_gfxDevice.ReserveGifSpace(kGifReserveQuadwords);
        PsMesh::SelectDepthRegsForPass(*mMesh, nPass);
        if (pMat != nullptr) {
            // The narrowing is what psmat.h records as undecidable. Every material on this target
            // is a PsMat, because GfxDevice::Init() installs that creator.
            nMorePasses = static_cast<PsMat *>(pMat)->Select();
        } else {
            PsMat::SelectDefault();
        }
        SubmitInstanceGifPackets();
        ++nPass;
    } while (nMorePasses != 0);
    return 1;
}

// 0x005b2c60
void PsMultiMesh::SubmitInstanceGifPackets() {
    const int nInstances = static_cast<int>(mTransforms.size());
    g_renderStats.mnTriangles += nInstances * static_cast<int>(mMesh->mFacesOwner->mFaces.size());

    g_gfxDevice.CloseGifTag(1);
    g_gfxDevice.SwapGifWrite();
    // The same narrowing DrawSelf() makes for the material. Every mesh on this target is a PsMesh.
    const int nFaceQuadwords = static_cast<PsMesh *>(mMesh)->EmitMultiMeshFaceRun();
    const int nBatchLimit =
        std::min(kVu1InstanceQuadwordBudget - nFaceQuadwords, kInstanceUnpackQuadwordLimit) /
        kQuadwordsPerTransform;

    int bFirstBatch = 1;
    std::list<Transform>::iterator it = mTransforms.begin();
    while (it != mTransforms.end()) {
        GifQuadword *pHeader = g_gfxDevice.mpWrite;
        pHeader[0] = GifQuadword{0, 0};
        pHeader[1] = GifQuadword{0, 0};
        GifQuadword *pWrite = pHeader + 2;

        int nBatch = 0;
        while (nBatch < nBatchLimit && it != mTransforms.end()) {
            const GifQuadword *pRows = AsQuadwords(&*it);
            for (int i = 0; i < kQuadwordsPerTransform; ++i) {
                pWrite[i] = pRows[i];
            }
            pWrite += kQuadwordsPerTransform;
            ++nBatch;
            ++it;
        }

        pHeader[0].mHi =
            PackWordPair(0,
                         MakeVifCode(kVifCmdUnpackV4_32,
                                     nBatch * kQuadwordsPerTransform + kBatchHeaderQuadwords,
                                     kVifUnpackFlg | static_cast<unsigned>(nFaceQuadwords)));
        g_gfxDevice.mpWrite = pWrite;
        pHeader[1].mLo = static_cast<unsigned>(nBatch);
        g_gfxDevice.FlushReservedGif();

        GifQuadword *pCall = g_gfxDevice.mpWrite;
        g_gfxDevice.mpWrite = pCall + 1;
        pCall->mHi = 0;
        pCall->mLo = bFirstBatch != 0 ? MakeVifCode(kVifCmdMsCal, 0, kVu1InstanceEntry) :
                                        MakeVifCode(kVifCmdMsCnt, 0, 0);
        bFirstBatch = 0;
        g_gfxDevice.FlushGifPacket(0, 0);
    }
}

} // namespace Rnd
