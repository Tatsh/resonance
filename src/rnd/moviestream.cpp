#include "rnd/moviestream.h"

#include <cstdint>
#include <list>
#include <string.h>

#include "os/async.h"
#include "os/circbuff.h"
#include "os/loadfile.h"
#include "os/log.h"
#include "os/zone.h"
#include "rnd/movieasynccallback.h"
#include "rnd/moviestreamingasynccallback.h"
#include "rndartt/abitmap.h"
#include "synth/midi_main.h"

namespace {

// A four character code as the literal pool spells it, read as the word a chunk header compares.
inline unsigned int FourCc(const char *pszCode) {
    unsigned int nCode;
    memcpy(&nCode, pszCode, sizeof(nCode));
    return nCode;
}

// The chunk types, built by the unit's static initialiser from the literals at 0x0082eb90.
// 0x007578f0 through 0x00757930.
const unsigned int g_nMovsTag = FourCc("MOVS");
const unsigned int g_nMovtTag = FourCc("MOVT");
const unsigned int g_nPallTag = FourCc("PALL");
const unsigned int g_nFramTag = FourCc("FRAM");
const unsigned int g_nBlakTag = FourCc("BLAK");
const unsigned int g_nLoopTag = FourCc("LOOP");
const unsigned int g_nSndhTag = FourCc("SNDH");
const unsigned int g_nSndbTag = FourCc("SNDB");
const unsigned int g_nSndpTag = FourCc("SNDP");

// 0x00757938. Streams whose first read has not yet completed.
std::list<Rnd::MovieStream *> g_pendingStreams;

// 0x00757940 and 0x00757948.
MovieAsyncCallback g_movieAsyncCallback;
MovieStreamingAsyncCallback g_movieStreamingAsyncCallback;

// 0x0075794c. Non-zero while a streaming read is outstanding; only one may be.
int g_nStreamingReadPending;

// 0x007579a4. The stream the streaming callback commits to.
Rnd::MovieStream *g_pStreamingMovie;

// Sizes the stream reads with. A streaming file's first read and a rewind read 0x8000 bytes, a
// top-up needs more than 0x400 bytes free and is rounded down to whole quadwords, and the ring
// buffer of a streaming file stops 0x80e8 bytes short of the zone's end.
constexpr int kFirstReadBytes = 0x8000;
constexpr int kMinReadBytes = 0x400;
constexpr int kReadAlignMask = ~0xf;
constexpr int kStreamingBufferReserve = 0x80e8;
constexpr int kMinStreamingZoneBytes = 0x80000;

constexpr char kStreamingZoneName[] = "movieStreamBuff";

// The errors ParseHeader() and the constructor report.
constexpr int kErrorOutOfMemory = -1;
constexpr int kErrorNoFile = -2;
constexpr int kErrorBadHeader = -6;

// Bit 0 of a frame or palette chunk's flags word records that its colours have been swapped.
constexpr int kChunkSwapped = 1;

// The alpha byte of a palette entry, halved with rounding for the GS range.
constexpr unsigned int kColorMask = 0x00ffffff;
constexpr int kAlphaShift = 24;

// The payload of a PALL chunk: an entry count, a flags word, and the entries.
struct PaletteChunk {
    int mCount;
    int mFlags;
    unsigned int mEntries[1];
};

// The payload of a FRAM chunk: two words, a flags word, and the frame's bitmap.
struct FrameChunk {
    int mUnknown00[2];
    int mFlags;
    ABitmap mBitmap;
};

} // namespace

namespace Rnd {

// 0x0057f7b8
MovieStream::MovieStream(const char *pszPath, int bStreaming, int *pnError) {
    *pnError = 0;
    mBuffer = nullptr;
    mCircBuff = nullptr;
    mLoaded = 0;
    mAsyncHandle = 0;
    mFile = -1;
    mSoundHold = 0;
    memset(mHandlers, 0, sizeof(mHandlers));
    memset(mHandlerData, 0, sizeof(mHandlerData));

    mFileLength = GetUncompressedFileLength(pszPath);
    if (mFileLength <= 0) {
        *pnError = kErrorNoFile;
        return;
    }

    mStreaming = bStreaming;
    if (bStreaming != 0) {
        g_nStreamingReadPending = 0;
        g_pStreamingMovie = this;

        const int nPreviousZone = ZoneGetCurrent();
        const int nZone = FindZoneByName(kStreamingZoneName);
        if (nZone == -1) {
            Fatal("ERROR - Streaming movies require zone 'movieStreamBuff': %s\n", pszPath);
        }
        ZoneSetCurrent(nZone);
        ZoneReset();
        mBufferSize = ZoneGetAvail(kMinStreamingZoneBytes);
        if (mBufferSize < kMinStreamingZoneBytes) {
            Fatal("ERROR - Streaming movies require zone of at least 512K: %s\n", pszPath);
        }
        mBuffer = static_cast<char *>(ZoneAlloc(mBufferSize));
        ZoneSetCurrent(nPreviousZone);

        mFile = FileOpen(pszPath, 0);
        if (mFile < 0) {
            Fatal("ERROR - Couldn't open movie file %s for streaming\n", pszPath);
        }
        mAsyncHandle =
            AsyncSubmitRequest(mFile, mBuffer, kFirstReadBytes, 0, &g_movieAsyncCallback, 0);
        g_pendingStreams.push_back(this);
        mFileOffset = kFirstReadBytes;
        return;
    }

    // The whole file is read into a block of its uncompressed length. mBuffer stays null until
    // ParseHeader() records the block the read filled.
    mBufferSize = mFileLength;
    char *pBlock = static_cast<char *>(ZoneAlloc(mFileLength));
    if (pBlock == nullptr) {
        *pnError = kErrorOutOfMemory;
        return;
    }
    mAsyncHandle =
        AsyncLoadFileByPath(pszPath, pBlock, GetStoredFileLength(pszPath), &g_movieAsyncCallback);
    g_pendingStreams.push_back(this);
}

// 0x00580858
MovieStream::~MovieStream() {
    if (mFile >= 0) {
        FileClose(mFile);
    }
    if (mAsyncHandle != 0) {
        AsyncCancelRequest(mAsyncHandle);
    }
    delete mCircBuff;
}

// 0x005808e8
inline int MovieStream::RequestRead(int nBytes) {
    if (mStreaming == 0 || mLoaded == 0 || g_nStreamingReadPending != 0) {
        return 0;
    }
    const int nFree = mCircBuff->FreeSpace();
    if (nFree < nBytes) {
        nBytes = nFree;
    }
    if (nBytes <= kMinReadBytes) {
        return 0;
    }
    nBytes = mCircBuff->ContiguousWriteSize(nBytes);
    if (mFileOffset + nBytes > mFileLength) {
        const int nLeft = mFileLength - mFileOffset;
        if (nLeft > 0) {
            nBytes = nLeft;
        } else {
            FileSeek(mFile, mDataStart, kFileSeekSet);
            mFileOffset = mDataStart;
        }
    }
    nBytes &= kReadAlignMask;
    if (nBytes == 0) {
        return 0;
    }
    mAsyncHandle =
        AsyncSubmitRequest(mFile, mCircBuff->mWrite, nBytes, 0, &g_movieStreamingAsyncCallback, 0);
    g_nStreamingReadPending = 1;
    return nBytes;
}

// 0x0057fac8
void MovieStream::Update(int nTick, int nReadSize) {
    if (mSoundHold > 0) {
        --mSoundHold;
    }
    if (mLoaded == 0) {
        return;
    }

    if (nTick < mLastTick) {
        if (mStreaming == 0) {
            mCircBuff->mRead = mCircBuff->mBuff;
        } else {
            mCircBuff->mWrite = mCircBuff->mBuff;
            mCircBuff->mRead = mCircBuff->mBuff;
            FileSeek(mFile, mDataStart, kFileSeekSet);
            mFileOffset = mDataStart;
            const int nRead = FileRead(mFile, mCircBuff->mWrite, kFirstReadBytes);
            mFileOffset += nRead;
            mCircBuff->AdvanceWrite(nRead);
        }
        mLoopTicks = 0;
    }
    mLastTick = nTick;

    if (mStreaming != 0 && nReadSize > 0) {
        RequestRead(nReadSize); // Yes, the count queued is discarded.
    }

    bool bVideoDispatched = false;
    while (mCircBuff->mRead != mCircBuff->mWrite &&
           mCircBuff->IsClearOfWrite(mCircBuff->mRead, sizeof(ChunkHeader))) {
        char *pChunk = mCircBuff->mRead;
        ChunkHeader *pHeader = reinterpret_cast<ChunkHeader *>(pChunk);

        // A header that straddles the end of the buffer is completed by copying the bytes that
        // wrapped to the far side of mWrap.
        if (static_cast<unsigned>(mCircBuff->mWrap - pChunk) < sizeof(ChunkHeader)) {
            memcpy(mCircBuff->mWrap, mCircBuff->mBuff, sizeof(ChunkHeader));
        }
        if (!mCircBuff->IsClearOfWrite(pChunk, pHeader->mSize + sizeof(ChunkHeader))) {
            return;
        }
        if (nTick < mLoopTicks + pHeader->mTicks) {
            return;
        }

        const int nChunkBytes = pHeader->mSize + sizeof(ChunkHeader);
        if (pChunk + nChunkBytes > mCircBuff->mWrap) {
            memcpy(mCircBuff->mWrap, mCircBuff->mBuff, nChunkBytes - (mCircBuff->mWrap - pChunk));
        }

        const unsigned int nTag = pHeader->mTag;
        if (nTag == g_nPallTag || nTag == g_nFramTag || nTag == g_nBlakTag) {
            if (nTag == g_nFramTag) {
                FrameChunk *pFrame = reinterpret_cast<FrameChunk *>(pHeader + 1);
                if ((pFrame->mFlags & kChunkSwapped) == 0) {
                    if (g_nSkipColorSwap == 0) {
                        pFrame->mBitmap.SwapRedBlue();
                    }
                    pFrame->mFlags |= kChunkSwapped;
                }
            } else if (nTag == g_nPallTag) {
                PaletteChunk *pPalette = reinterpret_cast<PaletteChunk *>(pHeader + 1);
                if ((pPalette->mFlags & kChunkSwapped) == 0) {
                    if (g_nSkipColorSwap == 0) {
                        ABitmap::SwapRedBlue32(
                            reinterpret_cast<unsigned char *>(pPalette->mEntries),
                            pPalette->mCount);
                    }
                    for (int i = 0; i < pPalette->mCount; ++i) {
                        const unsigned int nEntry = pPalette->mEntries[i];
                        pPalette->mEntries[i] =
                            (nEntry & kColorMask) |
                            ((((nEntry >> kAlphaShift) + 1) >> 1) << kAlphaShift);
                    }
                    pPalette->mFlags |= kChunkSwapped;
                }
            }
            if (bVideoDispatched) {
                return;
            }
            bVideoDispatched = true;
            const ChunkHandler pfnHandler = mHandlers[pHeader->mTrackId];
            if (pfnHandler != nullptr) {
                pfnHandler(pHeader, pHeader + 1, mHandlerData[pHeader->mTrackId]);
            }
        } else if (nTag == g_nSndhTag || nTag == g_nSndbTag || nTag == g_nSndpTag) {
            if (mSoundHold > 0) {
                return;
            }
            mSoundHold = 1;
            const ChunkHandler pfnHandler = mHandlers[pHeader->mTrackId];
            if (pfnHandler != nullptr) {
                pfnHandler(pHeader, pHeader + 1, mHandlerData[pHeader->mTrackId]);
            }
        } else if (nTag == g_nLoopTag) {
            mLoopTicks += pHeader->mTicks;
        } else {
            // Yes, the binary reads a word from the address the code's first character gives.
            const char *pszTag = FourCcToString(pHeader);
            Fatal("ERROR - UNKNOWN MOVIE CHUNK TYPE: %x, pChunkHdr: %p\npCircBuff: %p, pWrapPtr: "
                  "%p, fileLen: %d\npChunkHdr->ticks: $%x, time: $%x\n",
                  *reinterpret_cast<const int *>(static_cast<std::intptr_t>(pszTag[0])),
                  pHeader,
                  mCircBuff,
                  mCircBuff->mWrap,
                  mFileLength,
                  pHeader->mTicks,
                  nTick);
        }

        if (mStreaming == 0 && pHeader->mTag == g_nLoopTag) {
            mCircBuff->mRead = mCircBuff->mBuff;
        } else {
            mCircBuff->AdvanceRead(nChunkBytes);
        }
    }
}

// 0x0057ffd0
int MovieStream::ParseHeader(char *pBuffer, int nBytes) {
    mBuffer = pBuffer;
    ChunkHeader *pHeader = reinterpret_cast<ChunkHeader *>(pBuffer);
    if (pHeader->mTag != g_nMovsTag) {
        return kErrorBadHeader;
    }
    char *pChunk = pBuffer + pHeader->mSize + sizeof(ChunkHeader);
    for (pHeader = reinterpret_cast<ChunkHeader *>(pChunk); pHeader->mTag == g_nMovtTag;
         pHeader = reinterpret_cast<ChunkHeader *>(pChunk)) {
        memcpy(mTracks[pHeader->mTrackId].mUnknown00, pHeader + 1, sizeof(Track));
        pChunk += pHeader->mSize + sizeof(ChunkHeader);
    }

    mDataStart = pChunk - mBuffer;
    int nRingBytes = mBufferSize - mDataStart;
    if (mStreaming != 0) {
        nRingBytes -= kStreamingBufferReserve;
    }
    nRingBytes = (nRingBytes + 0xf) & kReadAlignMask;
    mCircBuff = new CircBuff(pChunk, nRingBytes);
    if (mCircBuff == nullptr) {
        return kErrorOutOfMemory;
    }
    mCircBuff->AdvanceWrite(nBytes - mDataStart);
    mAsyncHandle = 0;
    mLastTick = 0;
    mLoopTicks = 0;
    return 0;
}

// 0x005808d0
void MovieStream::SetTrackHandler(int nTrackId, ChunkHandler pfnHandler, void *pData) {
    mHandlers[nTrackId] = pfnHandler;
    mHandlerData[nTrackId] = pData;
}

} // namespace Rnd

// 0x00580178
void MovieAsyncCallback::Done(
    int nHandle, [[maybe_unused]] int nFile, void *pBuffer, int nLength, int nStatus) {
    for (auto it = g_pendingStreams.begin(); it != g_pendingStreams.end(); ++it) {
        Rnd::MovieStream *pStream = *it;
        if (pStream->mAsyncHandle != nHandle) {
            continue;
        }
        if (nStatus > 0) {
            Fatal("MOVIE CALLBACK CALLED WITH ERROR: %d, AsyncHandle: %d\n", nStatus, nHandle);
        } else {
            const int nError = pStream->ParseHeader(static_cast<char *>(pBuffer), nLength);
            if (nError != 0) {
                Fatal("MOVIE POST-LOAD ERROR: %d, AsyncHandle: %d\n", nError, nHandle);
            } else {
                pStream->mLoaded = 1;
            }
        }
        g_pendingStreams.erase(it);
        return;
    }
    Fatal("HEY - MOVIE LOAD CALLBACK CAN'T FIND HANDLE %d\n", nHandle);
}

// 0x005809f0
void MovieStreamingAsyncCallback::Done(int nHandle,
                                       [[maybe_unused]] int nFile,
                                       [[maybe_unused]] void *pBuffer,
                                       int nLength,
                                       int nStatus) {
    Rnd::MovieStream *pStream = g_pStreamingMovie;
    if (nStatus > 0) {
        Fatal(
            "MOVIE STREAMING CALLBACK CALLED WITH ERROR: %d, AsyncHandle: %d\n", nStatus, nHandle);
    } else {
        pStream->mFileOffset += nLength;
        pStream->mCircBuff->AdvanceWrite(nLength);
    }
    g_nStreamingReadPending = 0;
}
