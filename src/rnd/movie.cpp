#include "rnd/movie.h"

#include <list>
#include <string.h>

#include "os/dbg.h"
#include "os/genpath.h"
#include "os/hxstr.h"
#include "os/loadfile.h"
#include "os/log.h"
#include "os/mem.h"
#include "os/zone.h"
#include "rnd/amovieset.h"
#include "rnd/filepath.h"
#include "rnd/manager.h"
#include "rnd/stream.h"
#include "rnd/tex.h"
#include "rndartt/acanvas.h"
#include "rndartt/apalette.h"
#include "rndartt/arect.h"

namespace Rnd {

namespace {

const char *const kMovieTag = "Rnd::Movie";

// Save() writes revision 2. Load() rejects 3 and above, and reads a discarded byte below 2.
constexpr int kMovieRevision = 2;
constexpr int kMovieRejectedRevision = 3;
constexpr int kMoviePaddedRevision = 2;

const char *NameText(const Object *pObject) {
    return pObject->mName.mStr != nullptr ? pObject->mName.mStr : g_szEmptyString;
}

const char *TextOf(const HxStr &text) {
    return text.mStr != nullptr ? text.mStr : g_szEmptyString;
}

// OpenMovieFile() builds the path in a 0x100-byte stack buffer.
constexpr int kMaxPathLength = 0x100;
constexpr char kMovieExtension[] = ".mmv";

// SetFrameSelf() offsets the animation frame and converts it to stream ticks as
// frame * 1000 / 960, in double precision.
constexpr float kFrameTimeOffset = 7680.0f;
constexpr double kTicksNumerator = 1000.0;
constexpr double kTicksDenominator = 960.0;

// Every track texture is reconfigured as an 8 bit bitmap.
constexpr int kMovieBitsPerPixel = 8;

// The second SetPalette() argument OnChunk() passes, which no implementation reads.
constexpr int kSetPaletteReservedArg = -1;

// The last time SetFrameSelf() converted.
// NTSC-U/C: 0x0077a59c, PAL: 0x007be35c
float g_flMovieBeatCached = -9999.999f;

// SetFrameSelf() returns early while it is positive. It only ever stores zero, and the test never
// fires.
// NTSC-U/C: 0x0077a598, PAL: 0x007be358
int g_nMovieBeatCacheAge;

// Reconfigure a texture whose size or depth differs from its track's frame, logging the change.
// SetFrameSelf() and SetTrackTexture() both expand this with their own message.
inline void MatchTextureToTrack(Tex *pTex, const AMovieSet::Track &track, const char *pszMessage) {
    if (track.mWidth == pTex->mWidth && track.mHeight == pTex->mHeight &&
        pTex->mBitsPerPixel == kMovieBitsPerPixel) {
        return;
    }
    printf(
        pszMessage, pTex->mWidth, pTex->mHeight, pTex->mBitsPerPixel, track.mWidth, track.mHeight);
    pTex->SetBitmapConfig(
        track.mWidth, track.mHeight, kMovieBitsPerPixel, HxStr(""), pTex->mMipSelect, pTex->mFlags);
    pTex->ReloadBitmaps();
}

// NTSC-U/C: 0x005d15e0, PAL: 0x006135d0
Dbg &operator<<(Dbg &sink, const std::list<Movie::TrackTexture> &textures) {
    sink.Print("(size:")->Format("%u", textures.size())->Print(")");

    int nIndex = 0;
    for (const auto &entry : textures) {
        Dbg *pSink = sink.Print("\n")->Format("%d", nIndex)->Print("\t")->Print("(trackId:");
        pSink = pSink->Format("%d", entry.mTrackId)->Print(" tex:");
        if (entry.mTex != nullptr) {
            pSink->Format("\"%s\"", NameText(entry.mTex));
        } else {
            pSink->Print("no object");
        }
        pSink->Print(")");
        ++nIndex;
    }
    return sink;
}

// NTSC-U/C: 0x005d1790, PAL: 0x00613780
Stream &operator<<(Stream &stream, const std::list<Movie::TrackTexture> &textures) {
    const int nCount = textures.size();
    stream.WriteLE(&nCount, sizeof(nCount));

    for (const auto &entry : textures) {
        const int nTrackId = entry.mTrackId;
        stream.WriteLE(&nTrackId, sizeof(nTrackId));
        if (entry.mTex != nullptr) {
            const Object *pTex = entry.mTex;
            stream.Write(NameText(pTex), pTex->mName.mLen + 1);
        } else {
            const char chTerminator = '\0';
            stream.Write(&chTerminator, 1);
        }
    }
    return stream;
}

} // namespace

// NTSC-U/C: 0x005ce3d0, PAL: 0x00610330
Movie::Movie(const HxStr &name) : Object(name), mStream(nullptr), mTexturesMatched(0), mZone(-1) {
    memset(static_cast<void *>(&mPalette), 0, sizeof(mPalette)); // Yes, after constructing it.
}

// NTSC-U/C: 0x005ce930, PAL: 0x006108a8
Movie::~Movie() {
    CloseMovieFile();
    ReleaseAllRefs();
}

// NTSC-U/C: 0x005cebe0, PAL: 0x00610b68
void Movie::OpenMovieFile() {
    for (auto &entry : mTrackTextures) {
        if (entry.mTex != nullptr) {
            entry.mTex->AddRef(this);
        }
    }
    if (mFilename.mLen == 0) {
        return;
    }

    char szPath[kMaxPathLength];
    strcpy(szPath, TextOf(mFilename));
    ConvertNameToGenerated(szPath, kMovieExtension);
    if (FileTrueSize(szPath) == 0) {
        Rnd::TheDbg.Notify("Couldn't load movie file: %s\n", szPath);
        return;
    }

    mZone = ZoneGetCurrent();
    int nError;
    mStream = new AMovieSet(szPath, 0, &nError);
    if (nError != 0) {
        Rnd::TheDbg.Notify(
            "Couldn't load movie file %s: %s\n", szPath, g_apszMovieStreamErrors[-nError]);
        delete mStream;
        mStream = nullptr;
        return;
    }
    mTexturesMatched = 0;
    mStartPending = 1;
}

// NTSC-U/C: 0x005cef00, PAL: 0x00610e88
void Movie::CloseMovieFile() {
    for (auto &entry : mTrackTextures) {
        if (entry.mTex != nullptr) {
            entry.mTex->RemoveRef(this);
        }
    }
    if (mStream != nullptr) {
        delete mStream;
        mStream = nullptr;
    }
}

// NTSC-U/C: 0x005ced80, PAL: 0x00610d08
void Movie::Replace(Object *pFrom, Object *pTo) {
    Animatable::Replace(pFrom, pTo);
    for (auto it = mTrackTextures.begin(); it != mTrackTextures.end();) {
        if (pTo != nullptr) {
            if (it->mTex == pFrom) {
                if (pFrom != nullptr) {
                    pFrom->RemoveRef(this);
                }
                if (it->mTex != nullptr) {
                    it->mTex = dynamic_cast<Tex *>(pTo);
                }
                if (it->mTex != nullptr) {
                    it->mTex->AddRef(this);
                }
            }
        } else if (it->mTex == pFrom && mStream != nullptr) {
            // Yes, without an open stream the entry retains the departing texture.
            mStream->AssignHandler(it->mTrackId, nullptr, nullptr);
            it->mTex = nullptr;
        }

        // An entry left without a texture is dropped.
        if (it->mTex == nullptr) {
            it = mTrackTextures.erase(it);
        } else {
            ++it;
        }
    }
}

// NTSC-U/C: 0x005cef88, PAL: 0x00610f10
void Movie::SetFrameSelf(float flFrame) {
    const float flTime = flFrame + kFrameTimeOffset;
    if (mStream == nullptr) {
        return;
    }

    if (mTexturesMatched == 0 && mStream->mLoaded != 0) {
        for (auto &entry : mTrackTextures) {
            Tex *pTex = entry.mTex;
            const int nTrackId = entry.mTrackId;
            if (pTex == nullptr) {
                continue;
            }
            AttachTrack(nTrackId);
            MatchTextureToTrack(
                pTex, mStream->mTracks[nTrackId], "RNDMOVIE: SWITCHING FROM %dx%dx%d to %dx%dx8\n");
        }
        mTexturesMatched = 1;
    }

    if (flTime != g_flMovieBeatCached) {
        g_flMovieBeatCached = flTime;
        g_nMovieBeatCacheAge = 0;
    }
    if (g_nMovieBeatCacheAge > 0) {
        return;
    }

    const int nTick = static_cast<int>(
        static_cast<long long>(static_cast<double>(flTime) * kTicksNumerator / kTicksDenominator));
    if (mStartPending != 0) {
        mStream->mLoopTicks = nTick;
        mStartPending = 0;
    }
    if (mStream != nullptr) {
        mStream->Update(nTick, 0);
    }
}

// NTSC-U/C: 0x005d24d8, PAL: 0x00614508
void Movie::AttachTrack(int nTrackId) {
    if (mStream != nullptr) {
        mStream->AssignHandler(nTrackId, MasterTrackCallback, this);
    }
}

// NTSC-U/C: 0x005d2508, PAL: 0x00614538
void Movie::DetachTrack(int nTrackId) {
    if (mStream != nullptr) {
        mStream->AssignHandler(nTrackId, nullptr, nullptr);
    }
}

// NTSC-U/C: 0x005cf1f8, PAL: 0x006111a0
void Movie::SetTrackTexture(int nTrackId, Tex *pTex) {
    RemoveTrackTexture(nTrackId);
    if (pTex == nullptr) {
        return;
    }
    pTex->AddRef(this);
    const TrackTexture entry = {nTrackId, pTex};
    mTrackTextures.push_back(entry);
    if (mStream != nullptr) {
        MatchTextureToTrack(
            pTex, mStream->mTracks[nTrackId], "SetTrackTex, switching from %dx%dx%d to %dx%dx8\n");
    }
    AttachTrack(nTrackId);
}

// NTSC-U/C: 0x005cf3f8, PAL: 0x006113c0
void Movie::RemoveTrackTexture(int nTrackId) {
    for (auto it = mTrackTextures.begin(); it != mTrackTextures.end(); ++it) {
        if (it->mTrackId == nTrackId) {
            if (it->mTex != nullptr) {
                it->mTex->RemoveRef(this);
            }
            mTrackTextures.erase(it);
            break;
        }
    }
    DetachTrack(nTrackId);
}

// NTSC-U/C: 0x005cf4c0, PAL: 0x00611488
void Movie::OnChunk(AMovieChunkHdr *pHeader, void *pPayload) {
    Tex *pTex = nullptr;
    for (const auto &entry : mTrackTextures) {
        if (entry.mTrackId == pHeader->mTrackId) {
            pTex = entry.mTex;
            break;
        }
    }
    if (pTex == nullptr) {
        return;
    }

    const unsigned int nTag = pHeader->mTag;
    if (nTag == g_nPallTag) {
        const auto *pChunk = static_cast<AMovieSet::PaletteChunk *>(pPayload);
        mPalette.SetEntries(pChunk->mEntries, 0, pChunk->mCount);
        pTex->SetPalette(&mPalette, kSetPaletteReservedArg);
    } else if (nTag == g_nFramTag) {
        auto *pChunk = static_cast<AMovieSet::FrameChunk *>(pPayload);
        pChunk->mBitmap.mPixels = pChunk + 1;
        ACanvas *pCanvas = pTex->LockMipBitmap(0, 0, 0);
        if (pCanvas == nullptr) {
            return;
        }
        pChunk->mBitmap.mPalette = &mPalette;
        pCanvas->DrawBitmap(pChunk->mBitmap, pChunk->mX, pChunk->mY);
        pTex->UnlockMipBitmap();
    } else if (nTag == g_nBlakTag) {
        ACanvas *pCanvas = pTex->LockMipBitmap(0, 0, 0);
        if (pCanvas == nullptr) {
            return;
        }
        pCanvas->SetColor32(static_cast<AMovieSet::BlankChunk *>(pPayload)->mColor);
        const ARect rect = {0, 0, pCanvas->mBitmap.mWidth, pCanvas->mBitmap.mHeight};
        pCanvas->DrawRect(rect);
        pTex->UnlockMipBitmap();
    }
}

// NTSC-U/C: 0x005d2530, PAL: 0x00614560
void Movie::MasterTrackCallback(AMovieChunkHdr *pHeader, void *pPayload, void *pData) {
    static_cast<Movie *>(pData)->OnChunk(pHeader, pPayload);
}

// NTSC-U/C: 0x005d1a90, PAL: 0x00613a80
void Movie::ReadTrackTextures(Stream &stream, std::list<TrackTexture> &textures) {
    int nCount;
    stream.ReadLE(&nCount, sizeof(nCount));
    textures.resize(nCount);

    for (auto &entry : textures) {
        stream.ReadLE(&entry.mTrackId, sizeof(entry.mTrackId));

        HxStr name;
        stream.ReadString(name);
        entry.mTex = dynamic_cast<Tex *>(TheManager.Find(name));
    }
}

// NTSC-U/C: 0x005d21b0, PAL: 0x006141e0
void Movie::Reopen() {
    CloseMovieFile();
    OpenMovieFile();
}

// NTSC-U/C: 0x005d21e0, PAL: 0x00614210
void Movie::DumpText(Dbg &sink) {
    Object::DumpText(sink);
    Animatable::DumpText(sink);
    if (sink.mDumpLevel <= 0) {
        return;
    }

    sink.Print("[Movie]\n");
    sink.Print("file:");
    mFilename.Print(sink);
    (*sink.Print(" trackTextures:") << mTrackTextures).Print("\n");
}

// NTSC-U/C: 0x005d2280, PAL: 0x006142b0
void Movie::Save(Stream &stream) {
    const int nRevision = kMovieRevision;
    stream.WriteLE(&nRevision, sizeof(nRevision));
    Animatable::Save(stream);
    mFilename.Save(stream);
    stream << mTrackTextures;
}

// NTSC-U/C: 0x005d22f8, PAL: 0x00614328
void Movie::Load(Stream &stream) {
    int nRevision;
    stream.ReadLE(&nRevision, sizeof(nRevision));
    if (nRevision >= kMovieRejectedRevision) {
        Rnd::TheDbg.Notify("Can't load new Movie\n");
        return;
    }

    Animatable::Load(stream);
    CloseMovieFile();
    mFilename.Load(stream);
    if (nRevision < kMoviePaddedRevision) {
        char chDiscarded;
        stream.Read(&chDiscarded, 1);
    }
    ReadTrackTextures(stream, mTrackTextures);
    OpenMovieFile();
}

// NTSC-U/C: 0x005d2078, PAL: 0x006140a8
const HxStr &Movie::ClassName() const {
    return g_movieClassName;
}

// NTSC-U/C: 0x005d23c8, PAL: 0x006143f8
void Movie::Copy(const Object *pSource, unsigned nFlags) {
    const Movie *pMovie = dynamic_cast<const Movie *>(pSource);
    Animatable::Copy(pSource, nFlags);
    CloseMovieFile();
    mFilename = pMovie->mFilename;
    mTrackTextures = pMovie->mTrackTextures;
    OpenMovieFile();
}

// NTSC-U/C: 0x005d1eb8, PAL: 0x00613ee8
void *Movie::operator new(size_t nSize) {
    return AllocateTaggedMemory(nSize, kMovieTag);
}

// NTSC-U/C: 0x005d1ed8, PAL: 0x00613f08
void Movie::operator delete(void *pBlock) {
    OperatorDeleteOverride(pBlock, kMovieTag);
}

// NTSC-U/C: 0x005d2480, PAL: 0x006144b0
Tex *Movie::FindTrackTexture(int nTrackId) const {
    for (const auto &entry : mTrackTextures) {
        if (entry.mTrackId == nTrackId) {
            return entry.mTex;
        }
    }
    return nullptr;
}

// NTSC-U/C: 0x005d2058, PAL: 0x00614088
const HxStr &Movie::GetRelativeFilename() const {
    return mFilename.RelativeToRoot();
}

// NTSC-U/C: 0x005d2190, PAL: 0x006141c0
void Movie::SetFilename(const HxStr &name) {
    mFilename.SetFromRoot(name);
}

// NTSC-U/C: 0x005d20b8, PAL: 0x006140e8
Object *CreateRegisteredMovie(const HxStr &name) {
    return NewMovie(name);
}

// NTSC-U/C: 0x0077a590, PAL: 0x007be350
HxStr g_movieClassName("Movie");

} // namespace Rnd
