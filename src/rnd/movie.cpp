#include "rnd/movie.h"

#include <list>

#include "os/failsink.h"
#include "os/hxstr.h"
#include "os/mem.h"
#include "rnd/filepath.h"
#include "rnd/manager.h"
#include "rnd/stream.h"
#include "rnd/tex.h"
#include "rndartt/apalette.h"

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
