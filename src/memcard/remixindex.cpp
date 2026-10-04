#include "memcard/remixindex.h"

#include <cstring>
#include <iostream>
#include <vector>

#include "game/freqappearance.h"
#include "msg/hxstrtransfer.h"
#include "os/hxstr.h"
#include "stream/ibstream.h"
#include "stream/obstream.h"

namespace {

// The banner and the field labels Dump() writes.
static const char *const kDumpBanner = "********** RemixIndex element **********";
static const char *const kLevelNameLabel = " LevelName = ";
static const char *const kRemixNameLabel = " RemixName = ";
static const char *const kFileNameLabel = " FileName  = ";
static const char *const kGameOKLabel = " GameOK    = ";
static const char *const kVersionLabel = " Version   = ";
static const char *const kAlbumNumLabel = " AlbumNum  = ";

// Reset() stores this date, and Load() reads the names only from a version above it.
static const char *const kEmptyDate = "";
constexpr int kNamelessVersion = 0;
// The first version whose records carry AlbumNum.
constexpr int kAlbumVersion = 2;

} // namespace

void RemixIndexElement::Dump() {
    std::cout << kDumpBanner << std::endl;
    std::cout << kLevelNameLabel << LevelName << std::endl;
    std::cout << kRemixNameLabel << RemixName << std::endl;
    std::cout << kFileNameLabel << FileName << std::endl;
    std::cout << kGameOKLabel << GameOK << std::endl;
    std::cout << kVersionLabel << Version << std::endl;
    std::cout << kAlbumNumLabel << AlbumNum << std::endl;
}

void RemixIndexElement::Save(OBStream &stream) {
    // Yes, the binary writes the format constant rather than Version.
    int nFormat = kRemixIndexElementFormat;
    stream.WriteLE(&nFormat, sizeof(nFormat));
    stream.Write(LevelName, sizeof(LevelName));
    stream.Write(RemixName, sizeof(RemixName));
    stream.Write(FileName, sizeof(FileName));
    char gameOK = GameOK;
    stream.Write(&gameOK, sizeof(gameOK));
    SaveHxStr(stream, dateTime);
    int nCount = static_cast<int>(appearances.size());
    stream.WriteLE(&nCount, sizeof(nCount));
    for (std::vector<FreqAppearance>::iterator it = appearances.begin(); it != appearances.end();
         ++it) {
        it->Save(stream);
    }
    int nAlbum = AlbumNum;
    stream.WriteLE(&nAlbum, sizeof(nAlbum));
}

void RemixIndexElement::Load(IBStream &stream) {
    stream.ReadLE(&Version, sizeof(Version));
    if (Version > kNamelessVersion) {
        stream.Read(LevelName, sizeof(LevelName));
        stream.Read(RemixName, sizeof(RemixName));
        stream.Read(FileName, sizeof(FileName));
        stream.Read(&GameOK, sizeof(GameOK));
        LoadHxStr(stream, dateTime);
        int nCount;
        stream.ReadLE(&nCount, sizeof(nCount));
        appearances.resize(nCount, FreqAppearance());
        for (std::vector<FreqAppearance>::iterator it = appearances.begin();
             it != appearances.end();
             ++it) {
            it->Load(stream);
        }
    }
    if (Version >= kAlbumVersion) {
        stream.ReadLE(&AlbumNum, sizeof(AlbumNum));
    }
}

void RemixIndex::ReadFromStream(IBStream &stream) {
    stream.ReadLE(&version, sizeof(version));
    int nCount;
    stream.ReadLE(&nCount, sizeof(nCount));
    elements.clear();
    for (int nIndex = 0; nIndex < nCount; ++nIndex) {
        // Yes, the binary copies the fresh element's uninitialised names, GameOK, and Version
        // before Load() fills them.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
        elements.push_back(RemixIndexElement());
#pragma GCC diagnostic pop
        elements[nIndex].Load(stream);
    }
}

void RemixIndexElement::Reset() {
    memset(LevelName, 0, sizeof(LevelName));
    memset(RemixName, 0, sizeof(RemixName));
    memset(FileName, 0, sizeof(FileName));
    GameOK = 0;
    Version = kRemixIndexElementFormat;
    dateTime = kEmptyDate;
}

void RemixIndex::WriteToStream(OBStream &stream) {
    int nVersion = version;
    stream.WriteLE(&nVersion, sizeof(nVersion));
    int nCount = static_cast<int>(elements.size());
    stream.WriteLE(&nCount, sizeof(nCount));
    for (std::vector<RemixIndexElement>::iterator it = elements.begin(); it != elements.end();
         ++it) {
        it->Save(stream);
    }
}

void RemixIndex::DumpElements() {
    for (std::vector<RemixIndexElement>::size_type nIndex = 0; nIndex < elements.size(); ++nIndex) {
        elements[nIndex].Dump();
    }
}
