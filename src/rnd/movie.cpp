#include "rnd/movie.h"

#include <list>
#include <string.h>

#include "os/failsink.h"
#include "os/genpath.h"
#include "os/hxstr.h"
#include "os/loadfile.h"
#include "os/log.h"
#include "os/mem.h"
#include "os/zone.h"
#include "rnd/filepath.h"
#include "rnd/manager.h"
#include "rnd/moviestream.h"
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
constexpr int kSetPaletteUnknownArg = -1;

// 0x0077a59c. The last time SetFrameSelf() converted.
float g_flMovieBeatCached;

// 0x0077a598. SetFrameSelf() returns early while it is positive. It only ever stores zero, and
// the test never fires.
int g_nMovieBeatCacheAge;

// Reconfigure a texture whose size or depth differs from its track's frame, logging the change.
// SetFrameSelf() and SetTrackTexture() both expand this with their own message.
inline void
MatchTextureToTrack(Tex *pTex, const MovieStream::Track &track, const char *pszMessage) {
    if (track.mWidth == pTex->mWidth && track.mHeight == pTex->mHeight &&
        pTex->mBitsPerPixel == kMovieBitsPerPixel) {
        return;
    }
    LogPrintf(
        pszMessage, pTex->mWidth, pTex->mHeight, pTex->mBitsPerPixel, track.mWidth, track.mHeight);
    pTex->SetBitmapConfig(
        track.mWidth, track.mHeight, kMovieBitsPerPixel, HxStr(""), pTex->mMipSelect, pTex->mFlags);
    pTex->ReloadBitmaps();
}

// 0x005d15e0
FailSink &operator<<(FailSink &sink, const std::list<Movie::TrackTexture> &textures) {
    sink.Print("(size:")->Format("%u", textures.size())->Print(")");

    int nIndex = 0;
    for (const auto &entry : textures) {
        FailSink *pSink = sink.Print("\n")->Format("%d", nIndex)->Print("\t")->Print("(trackId:");
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

// 0x005d1790
Stream &operator<<(Stream &stream, const std::list<Movie::TrackTexture> &textures) {
    const int nCount = textures.size();
    stream.Write(&nCount, sizeof(nCount));

    for (const auto &entry : textures) {
        const int nTrackId = entry.mTrackId;
        stream.Write(&nTrackId, sizeof(nTrackId));
        if (entry.mTex != nullptr) {
            const Object *pTex = entry.mTex;
            stream.WriteBytes(NameText(pTex), pTex->mName.mLen + 1);
        } else {
            const char chTerminator = '\0';
            stream.WriteBytes(&chTerminator, 1);
        }
    }
    return stream;
}

} // namespace

// 0x005ce3d0
Movie::Movie(const HxStr &name) : Object(name), mStream(nullptr), mTexturesMatched(0), mZone(-1) {
    memset(static_cast<void *>(&mPalette), 0, sizeof(mPalette)); // Yes, after constructing it.
}

// 0x005ce930
Movie::~Movie() {
    CloseMovieFile();
    ReleaseAllRefs();
}

// 0x005cebe0
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
    BuildBitmapCacheFileName(szPath, kMovieExtension);
    if (GetUncompressedFileLength(szPath) == 0) {
        g_failSink.Report("Couldn't load movie file: %s\n", szPath);
        return;
    }

    mZone = ZoneGetCurrent();
    int nError;
    mStream = new MovieStream(szPath, 0, &nError);
    if (nError != 0) {
        g_failSink.Report(
            "Couldn't load movie file %s: %s\n", szPath, g_apszMovieStreamErrors[-nError]);
        delete mStream;
        mStream = nullptr;
        return;
    }
    mTexturesMatched = 0;
    mStartPending = 1;
}

// 0x005cef00
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

// 0x005ced80
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
        } else if (it->mTex == pFrom) {
            DetachTrack(it->mTrackId);
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

// 0x005cef88
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

// 0x005d24d8
void Movie::AttachTrack(int nTrackId) {
    if (mStream != nullptr) {
        mStream->SetTrackHandler(nTrackId, ChunkHandler, this);
    }
}

// 0x005d2508
void Movie::DetachTrack(int nTrackId) {
    if (mStream != nullptr) {
        mStream->SetTrackHandler(nTrackId, nullptr, nullptr);
    }
}

// 0x005cf1f8
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

// 0x005cf3f8
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

// 0x005cf4c0
void Movie::OnChunk(MovieStream::ChunkHeader *pHeader, void *pPayload) {
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
        const auto *pChunk = static_cast<MovieStream::PaletteChunk *>(pPayload);
        mPalette.SetEntries(pChunk->mEntries, 0, pChunk->mCount);
        pTex->SetPalette(&mPalette, kSetPaletteUnknownArg);
    } else if (nTag == g_nFramTag) {
        auto *pChunk = static_cast<MovieStream::FrameChunk *>(pPayload);
        pChunk->mBitmap.mPixels = pChunk + 1;
        ACanvas *pCanvas = pTex->LockMipBitmap(0, 0, 0);
        if (pCanvas == nullptr) {
            return;
        }
        pChunk->mBitmap.mPalette = &mPalette;
        pCanvas->Blit(pChunk->mBitmap, pChunk->mX, pChunk->mY);
        pTex->UnlockMipBitmap();
    } else if (nTag == g_nBlakTag) {
        ACanvas *pCanvas = pTex->LockMipBitmap(0, 0, 0);
        if (pCanvas == nullptr) {
            return;
        }
        pCanvas->SetColor32(static_cast<MovieStream::BlankChunk *>(pPayload)->mColor);
        const ARect rect = {0, 0, pCanvas->mBitmap.mWidth, pCanvas->mBitmap.mHeight};
        pCanvas->FillRect(rect);
        pTex->UnlockMipBitmap();
    }
}

// 0x005d2530
void Movie::ChunkHandler(MovieStream::ChunkHeader *pHeader, void *pPayload, void *pData) {
    static_cast<Movie *>(pData)->OnChunk(pHeader, pPayload);
}

// 0x005d1a90
void Movie::ReadTrackTextures(Stream &stream, std::list<TrackTexture> &textures) {
    int nCount;
    stream.Read(&nCount, sizeof(nCount));
    textures.resize(nCount);

    for (auto &entry : textures) {
        stream.Read(&entry.mTrackId, sizeof(entry.mTrackId));

        HxStr name;
        stream.ReadString(name);
        entry.mTex = dynamic_cast<Tex *>(g_manager.Find(name));
    }
}

// 0x005d21b0
void Movie::Reopen() {
    CloseMovieFile();
    OpenMovieFile();
}

// 0x005d21e0
void Movie::DumpText(FailSink &sink) {
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

// 0x005d2280
void Movie::Save(Stream &stream) {
    const int nRevision = kMovieRevision;
    stream.Write(&nRevision, sizeof(nRevision));
    Animatable::Save(stream);
    mFilename.Save(stream);
    stream << mTrackTextures;
}

// 0x005d22f8
void Movie::Load(Stream &stream) {
    int nRevision;
    stream.Read(&nRevision, sizeof(nRevision));
    if (nRevision >= kMovieRejectedRevision) {
        g_failSink.Report("Can't load new Movie\n");
        return;
    }

    Animatable::Load(stream);
    CloseMovieFile();
    mFilename.Load(stream);
    if (nRevision < kMoviePaddedRevision) {
        char chDiscarded;
        stream.ReadBytes(&chDiscarded, 1);
    }
    ReadTrackTextures(stream, mTrackTextures);
    OpenMovieFile();
}

// 0x005d2078
const HxStr &Movie::ClassName() const {
    return g_movieClassName;
}

// 0x005d23c8
void Movie::Copy(const Object *pSource, unsigned nFlags) {
    const Movie *pMovie = dynamic_cast<const Movie *>(pSource);
    Animatable::Copy(pSource, nFlags);
    CloseMovieFile();
    mFilename = pMovie->mFilename;
    mTrackTextures = pMovie->mTrackTextures;
    OpenMovieFile();
}

// 0x005d1eb8
void *Movie::operator new(size_t nSize) {
    return AllocateTaggedMemory(nSize, kMovieTag);
}

// 0x005d1ed8
void Movie::operator delete(void *pBlock) {
    FreeTaggedMemory(pBlock, kMovieTag);
}

// 0x005d2480
Tex *Movie::FindTrackTexture(int nTrackId) const {
    for (const auto &entry : mTrackTextures) {
        if (entry.mTrackId == nTrackId) {
            return entry.mTex;
        }
    }
    return nullptr;
}

// 0x005d2058
const HxStr &Movie::GetRelativeFilename() const {
    return mFilename.RelativeToRoot();
}

// 0x005d2190
void Movie::SetFilename(const HxStr &name) {
    mFilename.SetFromRoot(name);
}

// 0x005d20b8
Object *CreateRegisteredMovie(const HxStr &name) {
    return NewMovie(name);
}

// 0x0077a590
HxStr g_movieClassName("Movie");

} // namespace Rnd
