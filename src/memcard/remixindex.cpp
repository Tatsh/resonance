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

// 0x00136150
void RemixIndexElement::Dump() {
    std::cout << kDumpBanner << std::endl;
    std::cout << kLevelNameLabel << LevelName << std::endl;
    std::cout << kRemixNameLabel << RemixName << std::endl;
    std::cout << kFileNameLabel << FileName << std::endl;
    std::cout << kGameOKLabel << GameOK << std::endl;
    std::cout << kVersionLabel << Version << std::endl;
    std::cout << kAlbumNumLabel << AlbumNum << std::endl;
}

// 0x00136278
void RemixIndexElement::Save(OBStream &stream) {
    // Yes, the binary writes the format constant rather than Version.
    int nFormat = kRemixIndexElementFormat;
    stream.Write(&nFormat, sizeof(nFormat));
    stream.WriteBytes(LevelName, sizeof(LevelName));
    stream.WriteBytes(RemixName, sizeof(RemixName));
    stream.WriteBytes(FileName, sizeof(FileName));
    char gameOK = GameOK;
    stream.WriteBytes(&gameOK, sizeof(gameOK));
    SaveHxStr(stream, dateTime);
    int nCount = static_cast<int>(appearances.size());
    stream.Write(&nCount, sizeof(nCount));
    for (std::vector<FreqAppearance>::iterator it = appearances.begin(); it != appearances.end();
         ++it) {
        it->Save(stream);
    }
    int nAlbum = AlbumNum;
    stream.Write(&nAlbum, sizeof(nAlbum));
}

// 0x00136448
void RemixIndexElement::Load(IBStream &stream) {
    stream.Read(&Version, sizeof(Version));
    if (Version > kNamelessVersion) {
        stream.ReadBytes(LevelName, sizeof(LevelName));
        stream.ReadBytes(RemixName, sizeof(RemixName));
        stream.ReadBytes(FileName, sizeof(FileName));
        stream.ReadBytes(&GameOK, sizeof(GameOK));
        LoadHxStr(stream, dateTime);
        int nCount;
        stream.Read(&nCount, sizeof(nCount));
        appearances.resize(nCount, FreqAppearance());
        for (std::vector<FreqAppearance>::iterator it = appearances.begin();
             it != appearances.end();
             ++it) {
            it->Load(stream);
        }
    }
    if (Version >= kAlbumVersion) {
        stream.Read(&AlbumNum, sizeof(AlbumNum));
    }
}

// 0x001366e0
void RemixIndex::ReadFromStream(IBStream &stream) {
    stream.Read(&version, sizeof(version));
    int nCount;
    stream.Read(&nCount, sizeof(nCount));
    elements.clear();
    for (int nIndex = 0; nIndex < nCount; ++nIndex) {
        // Yes, the binary copies the fresh element's uninitialised names before Load() fills them.
        elements.push_back(RemixIndexElement());
        elements[nIndex].Load(stream);
    }
}

// 0x001397f0
void RemixIndexElement::Reset() {
    memset(LevelName, 0, sizeof(LevelName));
    memset(RemixName, 0, sizeof(RemixName));
    memset(FileName, 0, sizeof(FileName));
    GameOK = 0;
    Version = kRemixIndexElementFormat;
    dateTime = kEmptyDate;
}

// 0x00139858
void RemixIndex::WriteToStream(OBStream &stream) {
    int nVersion = version;
    stream.Write(&nVersion, sizeof(nVersion));
    int nCount = static_cast<int>(elements.size());
    stream.Write(&nCount, sizeof(nCount));
    for (std::vector<RemixIndexElement>::iterator it = elements.begin(); it != elements.end();
         ++it) {
        it->Save(stream);
    }
}

// 0x00139920
void RemixIndex::DumpElements() {
    for (std::vector<RemixIndexElement>::size_type nIndex = 0; nIndex < elements.size(); ++nIndex) {
        elements[nIndex].Dump();
    }
}
