#include "os/spew.h"

#include <algorithm>
#include <ctype.h>
#include <fstream>

namespace {

// Channel names NewChannel() treats specially. Any other name is a file to write.
constexpr char kChannelOff[] = "off";
constexpr char kChannelConsole[] = "console";
constexpr char kChannelDebug[] = "debug";

// Separator PrintConnections() writes between a file and its channel.
constexpr char kConnectionSeparator[] = " ";

// Lower-cases a string in place, through the C library's tolower().
inline void LowerCase(HxStr &text) {
    std::transform(text.mStr, text.mStr + text.mLen, text.mStr, tolower);
}

} // namespace

// 0x004b32b8
Spew::Spew() {
}

// 0x004b32e8
Spew::~Spew() {
    CloseChannels();
}

// 0x004b3528
void Spew::Register(std::ostream **ppStream, const char *pszFile) {
    HxStr name(pszFile);
    int nPos = name.ReverseFind('.');
    if (nPos != static_cast<int>(g_nHxStrNoPosition)) {
        name.Truncate(nPos);
    }
    nPos = name.ReverseFind('\\');
    if (nPos != static_cast<int>(g_nHxStrNoPosition)) {
        name.Erase(0, nPos + 1);
    }
    LowerCase(name);
    mConnections.push_back(Connection(name, ppStream, nullptr));
}

// 0x004b36b8
void Spew::Connect(const HxStr &file, const HxStr &channel) {
    Channel *pChannel = nullptr;
    HxStr channelName(channel);
    LowerCase(channelName);
    auto itChannel = mChannels.begin();
    for (; itChannel != mChannels.end(); ++itChannel) {
        if ((*itChannel)->mName == channelName) {
            pChannel = *itChannel;
            break;
        }
    }
    if (itChannel == mChannels.end()) {
        pChannel = NewChannel(channelName);
        mChannels.push_back(pChannel);
    }

    HxStr fileName(file);
    LowerCase(fileName);
    for (auto &connection : mConnections) {
        if (connection.mFile == fileName) {
            *connection.mppStream = pChannel->mStream;
            connection.mpChannel = pChannel;
            break;
        }
    }
}

// 0x004b3898
Spew::Channel *Spew::NewChannel(const HxStr &name) {
    std::ostream *pStream = nullptr;
    if (name == kChannelOff) {
        // A channel with no stream discards its output.
    } else if (name == kChannelConsole) {
        pStream = &std::cout;
    } else if (name == kChannelDebug) {
        // Yes, the binary gives this channel no stream either.
    } else {
        pStream = new std::ofstream(name.mStr != nullptr ? name.mStr : g_szEmptyString);
    }
    return new Channel(name, pStream);
}

// 0x004b3448
void Spew::CloseChannels() {
    for (Channel *pChannel : mChannels) {
        if (pChannel == nullptr) {
            continue;
        }
        if (pChannel->mStream != nullptr) {
            pChannel->mStream->flush();
            // Yes, the binary deletes the console channel's stream, which is cout.
            delete pChannel->mStream;
        }
        delete pChannel;
    }
    mChannels.erase(mChannels.begin(), mChannels.end());
}

// 0x004b4550
void Spew::PrintConnections(std::ostream &stream) {
    for (const auto &connection : mConnections) {
        stream << connection.mFile;
        if (connection.mpChannel != nullptr) {
            stream << kConnectionSeparator << connection.mpChannel->mName;
        }
        stream << std::endl;
    }
}
